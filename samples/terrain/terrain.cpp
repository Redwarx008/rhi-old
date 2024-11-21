#include "terrain.h"

#include <fstream>
#include <algorithm>
#include <vector>
using namespace rhi;

static std::vector<uint32_t> loadShaderData(const char* filePath)
{
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);
	std::vector<uint32_t> buffer;
	if (!file.is_open()) {
		return buffer;
	}

	size_t fileSize = (size_t)file.tellg();
	buffer.resize(fileSize / sizeof(uint32_t));
	//spirv expects the buffer to be on uint32, so make sure to reserve an int vector big enough for the entire file

	//put file cursor at beginning
	file.seekg(0);

	//load the entire file into the buffer
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

	//now that the file is loaded into the buffer, we can close it
	file.close();

	return buffer;
}

void Terrain::init()
{
	m_Heightmap = loadHeightmap(m_RenderDevice, "heightmap.png", &m_RasterSizeX, &m_RasterSizeZ);
	assert(m_RasterSizeX >= 512 && m_RasterSizeZ >= 512);
	{
		SamplerDesc desc{};
		m_LinearSampler = m_RenderDevice->createSampler(desc);
	}

	uint32_t topNodeSize;
	// Determine the lod count
	{
		uint32_t minLength = (std::min)(m_RasterSizeX, m_RasterSizeZ);
		topNodeSize = std::pow(2, (int)std::log2(minLength) - 3);
		m_NodeLevelCount = std::log2(topNodeSize) - std::log2(m_BaseNodeSize) + 1;
	}

	uint32_t topNodeCountX = (uint32_t)std::ceil(m_RasterSizeX / (double)topNodeSize);
	uint32_t topNodeCountY = (uint32_t)std::ceil(m_RasterSizeZ / (double)topNodeSize);

	uint32_t baseNodeCountX = topNodeCountX * std::pow(2, m_NodeLevelCount - 1);
	uint32_t baseNodeCountY = topNodeCountY * std::pow(2, m_NodeLevelCount - 1);

	TextureDesc desc{};
	desc.width = baseNodeCountX;
	desc.height = baseNodeCountY;
	desc.dimension = TextureDimension::Texture2D;
	desc.format = Format::RGBA32_FLOAT;
	desc.mipLevels = m_NodeLevelCount;
	desc.usage = TextureUsage::UnorderedAccess | TextureUsage::ShaderResource;
	m_MinMaxHeightErrorMap = m_RenderDevice->createTexture(desc);

}

void Terrain::initPipeline()
{
	// calculate minmaxerror map pipeline
	{
		auto computeShaderData = loadShaderData("build_minmaxerror_map.comp.spv");
		ShaderCreateInfo shaderCI{};
		shaderCI.entry = "main";
		shaderCI.type = ShaderType::Compute;
		IShader* computeShader = m_RenderDevice->createShader(shaderCI, computeShaderData.data(), computeShaderData.size() * sizeof(uint32_t));

		{
			ResourceSetLayoutBinding layoutBindings[] =
			{
				ResourceSetLayoutBinding::TextureWithSampler(ShaderType::Compute, 0), //heightmap
				ResourceSetLayoutBinding::SampledTexture(ShaderType::Compute, 1), //lower minMaxErrorMap mip
				ResourceSetLayoutBinding::StorageTexture(ShaderType::Compute, 2), //current minMaxErrorMap mip
				ResourceSetLayoutBinding::UniformBuffer(ShaderType::Compute, 3) // current Lod
			};

			ResourceSetBinding bindings[] =
			{
				ResourceSetBinding::TextureWithSampler(m_Heightmap->getDefaultView(), m_LinearSampler, 0),
				ResourceSetBinding::UniformBuffer(m_TerrainParamsBuffer, 3) // we will write minMaxErrorMapMips in runtime
			};

			m_BuildMinMaxMapErrorMapPass.resourceSetLayout = m_RenderDevice->createResourceSetLayout(layoutBindings, 2);
			m_BuildMinMaxMapErrorMapPass.resourceSet = m_RenderDevice->createResourceSet(m_BuildMinMaxMapErrorMapPass.resourceSetLayout, bindings, 2);
		
		}

		PushConstantDesc pushConstantDesc{};
		pushConstantDesc.stage = ShaderType::Compute;
		pushConstantDesc.size = sizeof(uint32_t); // current lod 

		ComputePipelineCreateInfo pipelineCI{};
		pipelineCI.computeShader = computeShader;
		pipelineCI.resourceSetLayouts = &m_BuildMinMaxMapErrorMapPass.resourceSetLayout;
		pipelineCI.resourceSetLayoutCount = 1;
		pipelineCI.pushConstantDescs = &pushConstantDesc;
		pipelineCI.pushConstantCount = 1;
		m_BuildMinMaxMapErrorMapPass.pipeline = m_RenderDevice->createComputePipeline(pipelineCI);
		delete computeShader;
	}

	// traverse quad tree and select node pipeline
	{
		auto computeShaderData = loadShaderData("terrain_node_select.comp.spv");
		ShaderCreateInfo shaderCI{};
		shaderCI.entry = "main";
		shaderCI.type = ShaderType::Compute;

		IShader* computeShader = m_RenderDevice->createShader(shaderCI, computeShaderData.data(), computeShaderData.size() * sizeof(uint32_t));

		{
			ResourceSetLayoutBinding layoutBindings[] =
			{
				ResourceSetLayoutBinding::TextureWithSampler(ShaderType::Compute, 0), // minMaxErrorMap
				ResourceSetLayoutBinding::StorageBuffer(ShaderType::Compute, 1), // consumeNodeList
				ResourceSetLayoutBinding::StorageBuffer(ShaderType::Compute, 2), // appendNodeList
				ResourceSetLayoutBinding::StorageBuffer(ShaderType::Compute, 3), // finalNodeList 
				ResourceSetLayoutBinding::UniformBuffer(ShaderType::Compute, 4) // sceneData
			};

			ResourceSetBinding bindings[] =
			{
				ResourceSetBinding::TextureWithSampler(m_MinMaxHeightErrorMap->getDefaultView(), m_LinearSampler, 0),
				ResourceSetBinding::StorageBuffer(m_SelectNodesPass.nodeListA, 1),
				ResourceSetBinding::StorageBuffer(m_SelectNodesPass.nodeListB, 2),
				ResourceSetBinding::StorageBuffer(m_SelectNodesPass.finalNodeList, 3),
				//ResourceSetItem::UniformBuffer(m_SceneDataBuffers[m_FrameInFlight], 7) // scene data will write in runtime
			};

			m_SelectNodesPass.resourceSetLayout = m_RenderDevice->createResourceSetLayout(layoutBindings, sizeof(layoutBindings) / sizeof(ResourceSetLayoutBinding));
			m_SelectNodesPass.resourceSet = m_RenderDevice->createResourceSet(m_SelectNodesPass.resourceSetLayout, bindings, sizeof(bindings) / sizeof(ResourceSetBinding));
		}

		ComputePipelineCreateInfo pipelineCI{};
		pipelineCI.computeShader = computeShader;
		pipelineCI.resourceSetLayouts = &m_SelectNodesPass.resourceSetLayout;
		pipelineCI.resourceSetLayoutCount = 1;
		m_SelectNodesPass.pipeline = m_RenderDevice->createComputePipeline(pipelineCI);

		delete computeShader;
	}

	// graphic pipeline
	{
		auto shaderData = loadShaderData("terrain.vert.spv");
		ShaderCreateInfo shaderCI{};
		shaderCI.entry = "main";
		shaderCI.type = ShaderType::Vertex;
		IShader* vertexShader = m_RenderDevice->createShader(shaderCI, shaderData.data(), shaderData.size() * sizeof(uint32_t));

		shaderData = loadShaderData("terrain.vert.spv");
		ShaderCreateInfo shaderCI{};
		shaderCI.entry = "main";
		shaderCI.type = ShaderType::Fragment;
		IShader* fragmentShader = m_RenderDevice->createShader(shaderCI, shaderData.data(), shaderData.size() * sizeof(uint32_t));

		{
			ResourceSetLayoutBinding layoutBindings[] =
			{
				ResourceSetLayoutBinding::TextureWithSampler(ShaderType::Vertex | ShaderType::Fragment, 0), // heightmap
				ResourceSetLayoutBinding::UniformBuffer(ShaderType::Vertex, 1), // finalNodeList
				ResourceSetLayoutBinding::UniformBuffer(ShaderType::Vertex, 2), // sceneData
			};

			ResourceSetBinding bindings[] =
			{
				ResourceSetBinding::TextureWithSampler(m_Heightmap->getDefaultView(), m_LinearSampler, 0),
				ResourceSetBinding::StorageBuffer(m_SelectNodesPass.finalNodeList, 1),
				//ResourceSetItem::UniformBuffer(m_SceneDataBuffers[m_FrameInFlight], 2, 0, offsetof(SceneData, frustumPlanes))
			};

			m_GraphicPass.resourceSetLayout = m_RenderDevice->createResourceSetLayout(layoutBindings, sizeof(layoutBindings) / sizeof(ResourceSetLayoutBinding));
			m_GraphicPass.resourceSet = m_RenderDevice->createResourceSet(m_GraphicPass.resourceSetLayout, bindings, sizeof(bindings) / sizeof(ResourceSetBinding));
		}

		// These match the following shader layout (see terrain.vert):
		//	layout (location = 0) in vec3 inPos;
		//	layout (location = 1) in vec2 uv;
		VertexInputAttribute vertexInputs[] =
		{
			{0, 0, Format::RGB32_FLOAT},
			{1, 1, Format::RG32_FLOAT}
		};

		GraphicsPipelineCreateInfo pipelineCI{};
		pipelineCI.primType = PrimitiveType::TriangleList;
		pipelineCI.vertexInputAttributes = vertexInputs;
		pipelineCI.vertexInputAttributeCount = sizeof(vertexInputs) / sizeof(VertexInputAttribute);
		pipelineCI.vertexShader = vertexShader;
		pipelineCI.fragmentShader = fragmentShader;
		pipelineCI.resourceSetLayouts = &m_GraphicPass.resourceSetLayout;
		pipelineCI.resourceSetLayoutCount = 1;
		pipelineCI.renderTargetFormatCount = 1;
		pipelineCI.renderTargetFormats[0] = m_SwapChain->getRenderTargetFormat();
		pipelineCI.depthStencilFormat = m_SwapChain->getDepthStencilFormat();
		pipelineCI.depthStencilState.depthTestEnable = true;
		pipelineCI.rasterState.cullMode = CullMode::back;
		m_GraphicPass.pipeline = m_RenderDevice->createGraphicsPipeline(pipelineCI);

		delete vertexShader;
		delete fragmentShader;
	}

}

void Terrain::buildMinMaxErrorMap(ICommandList* cmdList)
{
	ComputeState compState{};
	compState.pipeline = m_BuildMinMaxMapErrorMapPass.pipeline;
	compState.resourceSets[0] = m_BuildMinMaxMapErrorMapPass.resourceSet;
	compState.resourceSetCount = 1;

	for (uint32_t lodLevel = 0; lodLevel < m_NodeLevelCount; ++lodLevel)
	{
		ITextureView* currentMip = m_BuildMinMaxMapErrorMapPass.minMaxErrorMapMipViews[0];
		ITextureView* lowerMip;
		if (lodLevel == 0)
		{
			lowerMip = currentMip;
		}
		else
		{
			lowerMip = m_BuildMinMaxMapErrorMapPass.minMaxErrorMapMipViews[lodLevel - 1];
		}

		ResourceSetBinding bindings[] =
		{
			ResourceSetBinding::SampledTexture(lowerMip, 1),
			ResourceSetBinding::StorageTexture(currentMip, 2)
		};
		m_RenderDevice->updateResourceSet(m_BuildMinMaxMapErrorMapPass.resourceSet, bindings, sizeof(bindings) / sizeof(ResourceSetBinding));
		cmdList->setComputeState(compState);
		cmdList->commitResourceSet(m_BuildMinMaxMapErrorMapPass.resourceSet);
		cmdList->setPushConstant(ShaderType::Compute, &lodLevel);
		cmdList->dispatch(
			std::ceilf(currentMip->getTexture()->getDesc().width / 8.0),
			std::ceilf(currentMip->getTexture()->getDesc().height / 8.0),
			1);
	}
}

void Terrain::prepareData()
{
	// Create Terrain Parameters Uniform Buffer
	{
		BufferDesc desc{};
		desc.size = sizeof(uint32_t);
		desc.access = BufferAccess::GpuOnly;
		desc.usage = BufferUsage::UniformBuffer;
		m_TerrainParamsBuffer = m_RenderDevice->createBuffer(desc, &m_TerrainParameters, sizeof(TerrainParameters));
	}

	// create minMaxErrorMap views per mip
	{
		m_BuildMinMaxMapErrorMapPass.minMaxErrorMapMipViews.resize(m_NodeLevelCount);
		TextureViewDesc viewDesc{};
		viewDesc.dimension = TextureDimension::Texture2D;
		for (uint32_t mip = 0; mip < m_NodeLevelCount; ++mip)
		{
			viewDesc.baseMipLevel = mip;
			viewDesc.mipLevelCount = 1;

			m_BuildMinMaxMapErrorMapPass.minMaxErrorMapMipViews[mip] = m_MinMaxHeightErrorMap->createView(viewDesc);
		}
	}
	// create node lists
	{
		uint32_t maxNodePerSelect = m_MinMaxHeightErrorMap->getDesc().width * m_MinMaxHeightErrorMap->getDesc().height;
		BufferDesc desc{};
		desc.size = sizeof(uint32_t) + sizeof(glm::uvec2) * 2 * maxNodePerSelect; // counter + [nodeX, nodeY]
		desc.access = BufferAccess::GpuOnly;
		desc.usage = BufferUsage::StorageBuffer;

		m_SelectNodesPass.nodeListA = m_RenderDevice->createBuffer(desc);
		m_SelectNodesPass.nodeListB = m_RenderDevice->createBuffer(desc);
		desc.size = sizeof(uint32_t) + sizeof(glm::uvec3) * maxNodePerSelect; // counter + [nodeX, nodeY, LodLevel]
		m_SelectNodesPass.finalNodeList = m_RenderDevice->createBuffer(desc);
	}

	// create scene data buffers
	for (uint32_t i = 0; i < 2; ++i)
	{
		BufferDesc desc{};
		desc.size = sizeof(SceneData);
		desc.access = BufferAccess::GpuOnly;
		desc.usage = BufferUsage::StorageBuffer;
		m_SceneDataBuffers[i] = m_RenderDevice->createBuffer(desc);
	}
}
