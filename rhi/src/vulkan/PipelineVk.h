#pragma once

#include <vulkan/vulkan.h>

#include "rhi/rhi.h"
#include <vector>
#include <array>

namespace rhi::vulkan
{
	struct ContextVk;

	class GraphicsPipeline final : public IRenderPipeline
	{
	public:
		explicit GraphicsPipeline(const ContextVk& context)
			:m_Context(context){}
		~GraphicsPipeline();
		const RenderPipelineDesc& getDesc() const override  { return desc; }
		bool getPipelineCacheData(void* pData, size_t* pDataSize) const override;
		Object getNativeObject(NativeObjectType type) const override;

		RenderPipelineDesc desc;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkPipeline pipeline = VK_NULL_HANDLE;

		struct PushConstantInfo
		{
			PushConstantRange desc{};
			uint32_t offset = 0;
		};
		std::vector<PushConstantInfo> pushConstantInfos;
		std::array<std::vector<>>
	private:
		const ContextVk& m_Context;
	};

	class ComputePipeline final : public IComputePipeline
	{
	public:
		explicit ComputePipeline(const ContextVk& contex)
			:m_Context(contex) {}
		~ComputePipeline();
		bool getPipelineCacheData(void* pData, size_t* pDataSize) const override;
		Object getNativeObject(NativeObjectType type) const override;

		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkPipeline pipeline = VK_NULL_HANDLE;
		VkPipelineCache pipelineCache = VK_NULL_HANDLE;

		struct PushConstantInfo
		{
			PushConstantRange desc{};
			uint32_t offset = 0;
		};
		std::vector<PushConstantInfo> pushConstantInfos;
	private:
		const ContextVk& m_Context;
	};

	VkCullModeFlags convertCullMode(CullMode mode);
	VkPolygonMode convertPolygonMode(FillMode mode);
	VkBlendFactor convertBlendFactor(BlendFactor factor);
	VkBlendOp convertBlendOp(BlendOp op);
	VkColorComponentFlags convertColorMask(ColorMask mask);
	VkCompareOp convertCompareOp(CompareOp op);
	VkStencilOpState convertStencilOpState(StencilOpState stencilOpState);
	VkPrimitiveTopology PrimitiveTopologyConvert(PrimitiveType type);
	VkViewport convertViewport(const Viewport& viewport);
}