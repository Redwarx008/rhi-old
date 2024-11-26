#include "terrain.h"

#include <fstream>
#include <algorithm>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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
	prepareData();
	initPipeline();
	m_CommandList = m_RenderDevice->createCommandList();
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
				ResourceSetLayoutBinding::StorageTexture(ShaderType::Compute, 1), //lower minMaxErrorMap mip
				ResourceSetLayoutBinding::StorageTexture(ShaderType::Compute, 2), //current minMaxErrorMap mip
				ResourceSetLayoutBinding::UniformBuffer(ShaderType::Compute, 3) // current Lod
			};

			ResourceSetBinding bindings[] =
			{
				ResourceSetBinding::TextureWithSampler(m_Heightmap->getDefaultView(), m_LinearSampler, 0),
				ResourceSetBinding::UniformBuffer(m_TerrainParamsBuffer, 3) // we will write minMaxErrorMapMips in runtime
			};

			m_BuildMinMaxMapErrorMapPass.resourceSetLayout = m_RenderDevice->createResourceSetLayout(layoutBindings, sizeof(layoutBindings) / sizeof(ResourceSetLayoutBinding));
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
		auto computeShaderData = loadShaderData("select_node.comp.spv");
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
				ResourceSetLayoutBinding::UniformBuffer(ShaderType::Compute, 4), // sceneData
				ResourceSetLayoutBinding::UniformBuffer(ShaderType::Compute, 5), // terrain parameters
				ResourceSetLayoutBinding::StorageBuffer(ShaderType::Compute, 6), // draw indirect args buffer
				ResourceSetLayoutBinding::StorageBuffer(ShaderType::Compute, 7) // dispatch indirect args buffer
			};

			ResourceSetBinding bindings[] =
			{
				ResourceSetBinding::TextureWithSampler(m_MinMaxHeightErrorMap->getDefaultView(), m_LinearSampler, 0),
				ResourceSetBinding::StorageBuffer(m_SelectNodesPass.nodeListA, 1),
				ResourceSetBinding::StorageBuffer(m_SelectNodesPass.nodeListB, 2),
				ResourceSetBinding::StorageBuffer(m_SelectNodesPass.finalNodeList, 3),
				ResourceSetBinding::UniformBuffer(m_SceneDataBuffer, 4),
				ResourceSetBinding::UniformBuffer(m_TerrainParamsBuffer, 5),
				ResourceSetBinding::StorageBuffer(m_DrawIndirectBuffer, 6),
				ResourceSetBinding::StorageBuffer(m_SelectNodesPass.nextDispatchArgs, 7)
			};

			m_SelectNodesPass.resourceSetLayout = m_RenderDevice->createResourceSetLayout(layoutBindings, sizeof(layoutBindings) / sizeof(ResourceSetLayoutBinding));
			m_SelectNodesPass.resourceSet = m_RenderDevice->createResourceSet(m_SelectNodesPass.resourceSetLayout, bindings, sizeof(bindings) / sizeof(ResourceSetBinding));
		}

		PushConstantDesc pushConstantDesc{};
		pushConstantDesc.stage = ShaderType::Compute;
		pushConstantDesc.size = sizeof(ChunkedLodParams);
		ComputePipelineCreateInfo pipelineCI{};
		pipelineCI.computeShader = computeShader;
		pipelineCI.resourceSetLayouts = &m_SelectNodesPass.resourceSetLayout;
		pipelineCI.resourceSetLayoutCount = 1;
		pipelineCI.pushConstantCount = 1;
		pipelineCI.pushConstantDescs = &pushConstantDesc;
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

		shaderData = loadShaderData("terrain.frag.spv");

		shaderCI.entry = "main";
		shaderCI.type = ShaderType::Fragment;
		IShader* fragmentShader = m_RenderDevice->createShader(shaderCI, shaderData.data(), shaderData.size() * sizeof(uint32_t));

		{
			ResourceSetLayoutBinding layoutBindings[] =
			{
				ResourceSetLayoutBinding::TextureWithSampler(ShaderType::Vertex | ShaderType::Fragment, 0), // heightmap
				ResourceSetLayoutBinding::StorageBuffer(ShaderType::Vertex, 1), // finalNodeList
				ResourceSetLayoutBinding::UniformBuffer(ShaderType::Vertex, 2), // sceneData
				ResourceSetLayoutBinding::UniformBuffer(ShaderType::Vertex, 3) // terrain parameters
			};

			ResourceSetBinding bindings[] =
			{
				ResourceSetBinding::TextureWithSampler(m_Heightmap->getDefaultView(), m_LinearSampler, 0),
				ResourceSetBinding::StorageBuffer(m_SelectNodesPass.finalNodeList, 1),
				ResourceSetBinding::UniformBuffer(m_SceneDataBuffer, 2),
				ResourceSetBinding::UniformBuffer(m_TerrainParamsBuffer, 3)
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
			{0, 1, Format::RG16_UINT}
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

	cmdList->beginDebugLabel("buildMinMaxErrorMap pass", { 0.0, 1.0, 0.0, 1.0 });

	uint32_t width = m_MinMaxHeightErrorMap->getDesc().width;
	uint32_t height = m_MinMaxHeightErrorMap->getDesc().height;

	for (uint32_t lodLevel = 0; lodLevel < m_NodeLevelCount; ++lodLevel)
	{
		ITextureView* currentMip = m_BuildMinMaxMapErrorMapPass.minMaxErrorMapMipViews[lodLevel];
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
			ResourceSetBinding::StorageTexture(lowerMip, 1),
			ResourceSetBinding::StorageTexture(currentMip, 2)
		};
		cmdList->setComputeState(compState);
		m_RenderDevice->updateResourceSet(m_BuildMinMaxMapErrorMapPass.resourceSet, bindings, sizeof(bindings) / sizeof(ResourceSetBinding));
		cmdList->commitResourceSet(m_BuildMinMaxMapErrorMapPass.resourceSet);
		cmdList->setPushConstant(ShaderType::Compute, &lodLevel);
		cmdList->dispatch(
			std::ceilf((width >> lodLevel) / 8.0),
			std::ceilf((height >> lodLevel) / 8.0),
			1);
	}
	cmdList->endDebugLabel();
}

void Terrain::prepareData()
{
	// create heightmap
	{
		m_Heightmap = loadHeightmap(m_RenderDevice, "heightmap.png", &m_RasterSizeX, &m_RasterSizeZ);
		assert(m_RasterSizeX >= 512 && m_RasterSizeZ >= 512);
		m_TerrainParams.heightmapSize = glm::uvec2(m_RasterSizeX, m_RasterSizeZ);
		{
			SamplerDesc desc{};
			desc.maxAnisotropy = 0;
			m_LinearSampler = m_RenderDevice->createSampler(desc);
		}

		uint32_t topNodeSize;
		// Determine the lod count
		{
			uint32_t minLength = (std::min)(m_RasterSizeX, m_RasterSizeZ);
			topNodeSize = std::pow(2, (int)std::log2(minLength) - 3);
			m_NodeLevelCount = std::log2(topNodeSize) - std::log2(m_BaseNodeSize) + 1;
		}

		m_TopNodeCountX = (uint32_t)std::ceil(m_RasterSizeX / (double)topNodeSize);
		m_TopNodeCountY = (uint32_t)std::ceil(m_RasterSizeZ / (double)topNodeSize);

		uint32_t baseNodeCountX = m_TopNodeCountX * std::pow(2, m_NodeLevelCount - 1);
		uint32_t baseNodeCountY = m_TopNodeCountY * std::pow(2, m_NodeLevelCount - 1);

		TextureDesc desc{};
		desc.width = baseNodeCountX;
		desc.height = baseNodeCountY;
		desc.dimension = TextureDimension::Texture2D;
		desc.format = Format::RGBA32_FLOAT;
		desc.mipLevels = m_NodeLevelCount;
		desc.usage = TextureUsage::UnorderedAccess | TextureUsage::ShaderResource;
		m_MinMaxHeightErrorMap = m_RenderDevice->createTexture(desc);
	}

	// caculate chunked lod parameters
	{
		m_Fov = 60.0;
		m_ChunkedLodParams.tolerableError = 5.0;
		const double PI = std::atan(1.0) * 4;
		float tanHalfFov = std::tanf(0.5 * m_Fov * PI / 180.0);
		m_ChunkedLodParams.kFactor = m_WindowWidth / (2.0 * tanHalfFov);

		BufferDesc desc{};
		desc.size = sizeof(ChunkedLodParams);
		desc.access = BufferAccess::GpuOnly;
		desc.usage = BufferUsage::UniformBuffer;
		m_SelectNodesPass.ChunkedLodParametersBuffer = m_RenderDevice->createBuffer(desc, &m_ChunkedLodParams, sizeof(ChunkedLodParams));
	}

	// Create Terrain Parameters Uniform Buffer
	{
		BufferDesc desc{};
		desc.size = sizeof(TerrainParams);
		desc.access = BufferAccess::GpuOnly;
		desc.usage = BufferUsage::UniformBuffer;
		m_TerrainParamsBuffer = m_RenderDevice->createBuffer(desc, &m_TerrainParams, sizeof(TerrainParams));
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
		desc.access = BufferAccess::GpuOnly;
		desc.usage = BufferUsage::StorageBuffer;
		desc.size = sizeof(uint32_t) + sizeof(glm::uvec2) * maxNodePerSelect; // counter + [nodeX, nodeY]
		m_SelectNodesPass.nodeListA = m_RenderDevice->createBuffer(desc);
		m_SelectNodesPass.nodeListB = m_RenderDevice->createBuffer(desc);

		desc.size = sizeof(uint32_t) + sizeof(glm::vec4) * maxNodePerSelect; // counter + [nodeX, nodeY, LodLevel, morphValue]
		m_SelectNodesPass.finalNodeList = m_RenderDevice->createBuffer(desc);

		std::vector<uint32_t> topNodeData;
		topNodeData.push_back(m_TopNodeCountX * m_TopNodeCountY);
		for (int y = 0; y < m_TopNodeCountY; ++y)
		{
			for (int x = 0; x < m_TopNodeCountX; ++x)
			{
				topNodeData.push_back(x);
				topNodeData.push_back(y);
			}
		}
		desc.size = topNodeData.size();
		m_SelectNodesPass.topNodeList = m_RenderDevice->createBuffer(desc, topNodeData.data(), topNodeData.size());
	}

	// create indirct buffer
	{
		BufferDesc desc{};
		desc.size = sizeof(DrawIndexedIndirectCommand);
		desc.access = BufferAccess::GpuOnly;
		desc.usage = BufferUsage::StorageBuffer | BufferUsage::IndirectBuffer;

		m_DrawIndirectBuffer = m_RenderDevice->createBuffer(desc);

		desc.size = sizeof(DispatchIndirectCommand);
		m_SelectNodesPass.currentDispatchArgs = m_RenderDevice->createBuffer(desc);
		m_SelectNodesPass.nextDispatchArgs = m_RenderDevice->createBuffer(desc);
		glm::uvec3 data = glm::uvec3(m_TopNodeCountX, m_TopNodeCountY, 1);
		m_SelectNodesPass.topLodDispatchArgs = m_RenderDevice->createBuffer(desc, &data, sizeof(data));
	}

	// create scene data buffers
	{
		BufferDesc desc{};
		desc.size = sizeof(SceneData);
		desc.access = BufferAccess::GpuOnly;
		desc.usage = BufferUsage::UniformBuffer;
		m_SceneDataBuffer = m_RenderDevice->createBuffer(desc);
	}
	// create chunk mesh
	{
		m_GraphicPass.planeMesh = createPlaneMesh(m_RenderDevice, m_BaseNodeSize);
	}
}

void Terrain::draw()
{
	m_CommandList->open();
	buildMinMaxErrorMap(m_CommandList);

	// update scene data.
	m_CommandList->updateBuffer(m_SceneDataBuffer, &m_SceneData, sizeof(SceneData), 0);

	m_CommandList->clearBuffer(m_SelectNodesPass.nextDispatchArgs, 0);
	m_CommandList->clearBuffer(m_DrawIndirectBuffer, 0);
	m_CommandList->clearBuffer(m_SelectNodesPass.finalNodeList, 0);
	m_CommandList->clearBuffer(m_SelectNodesPass.nodeListA, 0);
	m_CommandList->clearBuffer(m_SelectNodesPass.nodeListB, 0);

	// Traversing the quadtree hierarchically
	IBuffer* consumeNodeList = m_SelectNodesPass.nodeListA;
	IBuffer* appendNodeList = m_SelectNodesPass.nodeListB;
	m_CommandList->copyBuffer(m_SelectNodesPass.topNodeList, 0, consumeNodeList, 0, m_SelectNodesPass.topNodeList->getDesc().size);
	m_CommandList->copyBuffer(m_SelectNodesPass.topLodDispatchArgs, 0, m_SelectNodesPass.currentDispatchArgs, 0,
		m_SelectNodesPass.topLodDispatchArgs->getDesc().size);

	ComputeState computeState{};
	computeState.pipeline = m_SelectNodesPass.pipeline;
	m_CommandList->setComputeState(computeState);

	for (int i = 0; i < m_NodeLevelCount; ++i)
	{
		m_ChunkedLodParams.currentLodLevel = m_NodeLevelCount - i - 1;

		ResourceSetBinding updateBindings[] =
		{
			ResourceSetBinding::StorageBuffer(consumeNodeList, 1),
			ResourceSetBinding::StorageBuffer(appendNodeList, 2)
		};
		m_RenderDevice->updateResourceSet(m_SelectNodesPass.resourceSet, updateBindings, 2);
		m_CommandList->commitResourceSet(m_SelectNodesPass.resourceSet);
		m_CommandList->setPushConstant(ShaderType::Compute, &m_ChunkedLodParams);
		m_CommandList->dispatchIndirect(m_SelectNodesPass.currentDispatchArgs, 0);
		m_CommandList->copyBuffer(m_SelectNodesPass.nextDispatchArgs, 0, m_SelectNodesPass.currentDispatchArgs, 0, sizeof(DispatchIndirectCommand));
		m_CommandList->clearBuffer(m_SelectNodesPass.nextDispatchArgs, 0);
		std::swap(consumeNodeList, appendNodeList);
	}

	// graphic pass
	GraphicsState graphicsState{};
	graphicsState.pipeline = m_GraphicPass.pipeline;
	graphicsState.viewports[0] = { (float)m_WindowWidth, (float)m_WindowHeight };
	graphicsState.viewportCount = 1;
	graphicsState.renderTargetViews[0] = m_SwapChain->getCurrentRenderTargetView();
	graphicsState.renderTargetCount = 1;
	graphicsState.depthStencilView = m_SwapChain->getDepthStencilView();
	graphicsState.vertexBuffers[0] = VertexBufferBinding().setBuffer(m_GraphicPass.planeMesh.vertexBuffer).setSlot(0).setOffset(0);
	graphicsState.vertexBufferCount = 1;
	graphicsState.indexBuffer = IndexBufferBinding().setBuffer(m_GraphicPass.planeMesh.indexBuffer).setFormat(Format::R32_UINT).setOffset(0);
	m_CommandList->setGraphicsState(graphicsState);
	m_CommandList->commitResourceSet(m_GraphicPass.resourceSet);
	m_CommandList->drawIndexedIndirect(m_DrawIndirectBuffer, 0, 1);
	m_CommandList->close();
	m_RenderDevice->executeCommandLists(&m_CommandList, 1);
}

void Terrain::KeyEvent(Key key, KeyState state)
{
	if (state == KeyState::Press)
	{
		switch (key)
		{
		case AppBase::Key::Esc:
			break;
		case AppBase::Key::W:
		case AppBase::Key::Up:
			m_Camera.keys.up = true;
			break;
		case AppBase::Key::A:
		case AppBase::Key::Left:
			m_Camera.keys.left = true;
			break;
		case AppBase::Key::S:
		case AppBase::Key::Down:
			m_Camera.keys.down = true;
			break;
		case AppBase::Key::D:
		case AppBase::Key::Right:
			m_Camera.keys.right = true;
			break;
		}
	}
	else if (state == KeyState::Release)
	{
		switch (key)
		{
		case AppBase::Key::Esc:
			break;
		case AppBase::Key::W:
		case AppBase::Key::Up:
			m_Camera.keys.up = false;
			break;
		case AppBase::Key::A:
		case AppBase::Key::Left:
			m_Camera.keys.left = false;
			break;
		case AppBase::Key::S:
		case AppBase::Key::Down:
			m_Camera.keys.down = false;
			break;
		case AppBase::Key::D:
		case AppBase::Key::Right:
			m_Camera.keys.right = false;
			break;
		}
	}
}


void Terrain::MouseEvent(float x, float y)
{
	int32_t dx = (int32_t)mouseState.position.x - x;
	int32_t dy = (int32_t)mouseState.position.y - y;

	if (mouseState.buttons.left) {
		m_Camera.rotate(glm::vec3(dy * m_Camera.rotationSpeed, -dx * m_Camera.rotationSpeed, 0.0f));
		//viewUpdated = true;
	}
	if (mouseState.buttons.right) {
		m_Camera.translate(glm::vec3(-0.0f, 0.0f, dy * .005f));
		//viewUpdated = true;
	}
	if (mouseState.buttons.middle) {
		m_Camera.translate(glm::vec3(-dx * 0.005f, -dy * 0.005f, 0.0f));
		//viewUpdated = true;
	}
	mouseState.position = glm::vec2((float)x, (float)y);
}

Terrain::~Terrain()
{
	delete m_CommandList;
	delete m_BuildMinMaxMapErrorMapPass.pipeline;
	delete m_BuildMinMaxMapErrorMapPass.resourceSetLayout;
	delete m_BuildMinMaxMapErrorMapPass.resourceSet;
	for (auto view : m_BuildMinMaxMapErrorMapPass.minMaxErrorMapMipViews)
	{
		delete view;
	}

	delete m_SelectNodesPass.pipeline;
	delete m_SelectNodesPass.topNodeList;
	delete m_SelectNodesPass.nodeListA;
	delete m_SelectNodesPass.nodeListB;
	delete m_SelectNodesPass.finalNodeList;
	delete m_SelectNodesPass.topLodDispatchArgs;
	delete m_SelectNodesPass.currentDispatchArgs;
	delete m_SelectNodesPass.nextDispatchArgs;
	delete m_SelectNodesPass.ChunkedLodParametersBuffer;
	delete m_SelectNodesPass.resourceSetLayout;
	delete m_SelectNodesPass.resourceSet;

	delete m_GraphicPass.pipeline;
	delete m_GraphicPass.resourceSetLayout;
	delete m_GraphicPass.resourceSet;
	delete m_GraphicPass.planeMesh.indexBuffer;
	delete m_GraphicPass.planeMesh.vertexBuffer;

	delete m_Heightmap;
	delete m_LinearSampler;
	delete m_MinMaxHeightErrorMap;
	delete m_TerrainParamsBuffer;
	delete m_DrawIndirectBuffer;
	delete m_SceneDataBuffer;
}