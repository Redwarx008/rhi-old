#pragma once
#include <rhi/rhi.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "app_base.h"
#include "frustum.h"

#include <array>
#include <vector>

inline rhi::ITexture* loadHeightmap(rhi::IRenderDevice* rd, const char* filename, uint32_t* width, uint32_t* height)
{
	rhi::ITexture* tex = nullptr;
	bool is16Bit = stbi_is_16_bit(filename);
	if (!is16Bit)
	{
		return tex;
	}
	int texWidth, texHeight, texChannels;
	auto pixels = stbi_load_16(filename, &texWidth, &texHeight, &texChannels, 1);
	
	uint64_t imageSize = texWidth * texHeight * sizeof(uint16_t);
	if (!pixels)
	{
		return tex;
	}
	rhi::TextureDesc desc{};
	desc.width = texWidth;
	desc.height = texHeight;
	desc.format = rhi::Format::R16_UINT;
	desc.dimension = rhi::TextureDimension::Texture2D;
	desc.usage = rhi::TextureUsage::ShaderResource;
	tex = rd->createTexture(desc);
	if (!tex)
	{
		return nullptr;
	}
	auto cmdList = rd->createCommandList();
	rhi::TextureUpdateInfo updateInfo{};
	updateInfo.srcRowPitch = texWidth * sizeof(uint16_t);
	updateInfo.srcDepthPitch = texWidth * texHeight * sizeof(uint16_t);
	updateInfo.dstRegion = { 0, 0, desc.width, desc.height };
	cmdList->updateTexture(tex, pixels, imageSize, updateInfo);
	uint64_t excuteID = rd->executeCommandLists(&cmdList, 1);
	rd->waitForExecution(excuteID);
	delete cmdList;

	*width = texWidth;
	*height = texHeight;
	return tex;
}

class Terrain : public AppBase
{
public:
	void init() override;
private:
	void buildMinMaxErrorMap(rhi::ICommandList* cmdList);
	void prepareData();
	void initPipeline();

	struct SceneData
	{
		glm::mat4 projection;
		glm::mat4 modelview;
		glm::vec4 cameraPos;
		glm::vec4 frustumPlanes[6];
	}m_SceneData;

	struct TerrainParameters
	{
		float heightScale = 51.0;
		uint32_t baseChunkSize = 16;
	}m_TerrainParameters;


	Frustum m_Frustum;

	uint32_t m_RasterSizeX;
	uint32_t m_RasterSizeZ;
	uint32_t m_NodeLevelCount;
	uint32_t m_BaseNodeSize = 16;
	struct
	{
		rhi::IComputePipeline* pipeline;

		rhi::IResourceSetLayout* resourceSetLayout;
		rhi::IResourceSet* resourceSet;
		
		std::vector<rhi::ITextureView*> minMaxErrorMapMipViews;
	} m_BuildMinMaxMapErrorMapPass;

	struct
	{
		rhi::IComputePipeline* pipeline;

		rhi::IResourceSetLayout* resourceSetLayout;
		rhi::IResourceSet* resourceSet;

		rhi::IBuffer* nodeListA;
		rhi::IBuffer* nodeListB;
		rhi::IBuffer* finalNodeList;

	}m_SelectNodesPass;


	struct
	{
		rhi::IGraphicsPipeline* pipeline;

		rhi::IResourceSetLayout* resourceSetLayout;
		rhi::IResourceSet* resourceSet;

		rhi::IBuffer* m_VertexBuffer;
		rhi::IBuffer* m_IndexBuffer;
		rhi::IBuffer* m_IndirectBuffer;

	}m_GraphicPass;


	rhi::ITexture* m_Heightmap;
	rhi::ISampler* m_LinearSampler;
	rhi::ITexture* m_MinMaxHeightErrorMap; // R for min height, G for max height, B for GeometricError
	rhi::IBuffer* m_TerrainParamsBuffer;

	std::array<rhi::IBuffer*, 2> m_SceneDataBuffers;
	uint32_t m_FrameInFlight = 0;
};

