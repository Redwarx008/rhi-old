#include "vk_pipeline.h"
#include "vk_errors.h"
#include "vk_resource.h"

#include <cassert>
namespace rhi
{
	VkCullModeFlags convertCullMode(CullMode mode)
	{
		switch (mode)
		{
		case rhi::CullMode::None:
			return VK_CULL_MODE_NONE;
		case rhi::CullMode::Front:
			return VK_CULL_MODE_FRONT_BIT;
		case rhi::CullMode::back:
			return VK_CULL_MODE_BACK_BIT;
		default:
			assert(!"invalid enum");
			return VK_CULL_MODE_BACK_BIT;
		}
	}

	VkPolygonMode convertPolygonMode(FillMode mode)
	{
		switch (mode)
		{
		case rhi::FillMode::Fill:
			return VK_POLYGON_MODE_FILL;
		case rhi::FillMode::Line:
			return VK_POLYGON_MODE_LINE;
		case rhi::FillMode::Point:
			return VK_POLYGON_MODE_POINT;
		default:
			assert(!"invalid enum");
			return VK_POLYGON_MODE_FILL;
		}
	}

	VkBlendFactor convertBlendFactor(BlendFactor factor)
	{
		static_assert(uint32_t(BlendFactor::One) == uint32_t(VK_BLEND_FACTOR_ONE));
		static_assert(uint32_t(BlendFactor::Zero) == uint32_t(VK_BLEND_FACTOR_ZERO));
		static_assert(uint32_t(BlendFactor::SrcColor) == uint32_t(VK_BLEND_FACTOR_SRC_COLOR));
		static_assert(uint32_t(BlendFactor::OneMinusSrcColor) == uint32_t(VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR));
		static_assert(uint32_t(BlendFactor::DstColor) == uint32_t(VK_BLEND_FACTOR_DST_COLOR));
		static_assert(uint32_t(BlendFactor::OneMinusDstColor) == uint32_t(VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR));
		static_assert(uint32_t(BlendFactor::SrcAlpha) == uint32_t(VK_BLEND_FACTOR_SRC_ALPHA));
		static_assert(uint32_t(BlendFactor::OneMinusSrcAlpha) == uint32_t(VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA));
		static_assert(uint32_t(BlendFactor::DstAlpha) == uint32_t(VK_BLEND_FACTOR_DST_ALPHA));
		static_assert(uint32_t(BlendFactor::OneMinusDstAlpha) == uint32_t(VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA));
		static_assert(uint32_t(BlendFactor::ConstantColor) == uint32_t(VK_BLEND_FACTOR_CONSTANT_COLOR));
		static_assert(uint32_t(BlendFactor::OneMinusConstantColor) == uint32_t(VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR));
		static_assert(uint32_t(BlendFactor::ConstantAlpha) == uint32_t(VK_BLEND_FACTOR_CONSTANT_ALPHA));
		static_assert(uint32_t(BlendFactor::OneMinusConstantAlpha) == uint32_t(VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA));
		static_assert(uint32_t(BlendFactor::SrcAlphaSaturate) == uint32_t(VK_BLEND_FACTOR_SRC_ALPHA_SATURATE));
		static_assert(uint32_t(BlendFactor::Src1Color) == uint32_t(VK_BLEND_FACTOR_SRC1_COLOR));
		static_assert(uint32_t(BlendFactor::OneMinusSrc1Color) == uint32_t(VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR));
		static_assert(uint32_t(BlendFactor::Src1Alpha) == uint32_t(VK_BLEND_FACTOR_SRC1_ALPHA));
		static_assert(uint32_t(BlendFactor::OneMinusSrc1Alpha) == uint32_t(VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA));

		return static_cast<VkBlendFactor>(uint32_t(factor));
	}

	VkBlendOp convertBlendOp(BlendOp op)
	{
		switch (op)
		{
		case rhi::BlendOp::Add:
			return VK_BLEND_OP_ADD;
		case rhi::BlendOp::Subrtact:
			return VK_BLEND_OP_SUBTRACT;
		case rhi::BlendOp::ReverseSubtract:
			return VK_BLEND_OP_REVERSE_SUBTRACT;
		case rhi::BlendOp::Min:
			return VK_BLEND_OP_MIN;
		case rhi::BlendOp::Max:
			return VK_BLEND_OP_MAX;
		default:
			assert(!"invalid enum");
			return VK_BLEND_OP_ADD;
		}
	}

	VkColorComponentFlags convertColorMask(ColorMask mask)
	{
		static_assert(uint32_t(ColorMask::Red) == uint32_t(VK_COLOR_COMPONENT_R_BIT));
		static_assert(uint32_t(ColorMask::Green) == uint32_t(VK_COLOR_COMPONENT_G_BIT));
		static_assert(uint32_t(ColorMask::Blue) == uint32_t(VK_COLOR_COMPONENT_B_BIT));
		static_assert(uint32_t(ColorMask::Alpha) == uint32_t(VK_COLOR_COMPONENT_A_BIT));
		static_assert(uint32_t(ColorMask::All) == uint32_t(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT));

		return static_cast<VkColorComponentFlags>(uint32_t(mask));
	}

	VkCompareOp convertCompareOp(CompareOp op)
	{
		static_assert(uint32_t(CompareOp::Never) == uint32_t(VK_COMPARE_OP_NEVER));
		static_assert(uint32_t(CompareOp::Less) == uint32_t(VK_COMPARE_OP_LESS));
		static_assert(uint32_t(CompareOp::Equal) == uint32_t(VK_COMPARE_OP_EQUAL));
		static_assert(uint32_t(CompareOp::LessOrEqual) == uint32_t(VK_COMPARE_OP_LESS_OR_EQUAL));
		static_assert(uint32_t(CompareOp::Greater) == uint32_t(VK_COMPARE_OP_GREATER));
		static_assert(uint32_t(CompareOp::NotEqual) == uint32_t(VK_COMPARE_OP_NOT_EQUAL));
		static_assert(uint32_t(CompareOp::GreaterOrEqual) == uint32_t(VK_COMPARE_OP_GREATER_OR_EQUAL));
		static_assert(uint32_t(CompareOp::Always) == uint32_t(VK_COMPARE_OP_ALWAYS));

		return static_cast<VkCompareOp>(uint32_t(op));
	}

	static VkStencilOp convertStencilOp(StencilOp op)
	{
		static_assert(uint32_t(StencilOp::Keep) == uint32_t(VK_STENCIL_OP_KEEP));
		static_assert(uint32_t(StencilOp::Zero) == uint32_t(VK_STENCIL_OP_ZERO));
		static_assert(uint32_t(StencilOp::Replace) == uint32_t(VK_STENCIL_OP_REPLACE));
		static_assert(uint32_t(StencilOp::IncrementAndClamp) == uint32_t(VK_STENCIL_OP_INCREMENT_AND_CLAMP));
		static_assert(uint32_t(StencilOp::DecrementAndClamp) == uint32_t(VK_STENCIL_OP_DECREMENT_AND_CLAMP));
		static_assert(uint32_t(StencilOp::Invert) == uint32_t(VK_STENCIL_OP_INVERT));
		static_assert(uint32_t(StencilOp::IncrementAndWrap) == uint32_t(VK_STENCIL_OP_INCREMENT_AND_WRAP));
		static_assert(uint32_t(StencilOp::DecrementAndWrap) == uint32_t(VK_STENCIL_OP_DECREMENT_AND_WRAP));

		return static_cast<VkStencilOp>(uint32_t(op));
	}

	VkStencilOpState convertStencilOpState(StencilOpState stencilOpState)
	{
		VkStencilOpState res{};
		res.failOp = convertStencilOp(stencilOpState.failOp);
		res.passOp = convertStencilOp(stencilOpState.passOp);
		res.depthFailOp = convertStencilOp(stencilOpState.depthFailOp);
		res.compareOp = convertCompareOp(stencilOpState.compareOp);
		res.compareMask = stencilOpState.compareMak;
		res.writeMask = stencilOpState.writeMask;
		res.reference = stencilOpState.referenceValue;
		return res;
	}

	VkPrimitiveTopology convertPrimitiveTopology(PrimitiveType type)
	{
		switch (type)
		{
		case rhi::PrimitiveType::PointList:
			return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case rhi::PrimitiveType::LineList:
			return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case rhi::PrimitiveType::LineStrip:
			return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case rhi::PrimitiveType::TriangleList:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case rhi::PrimitiveType::TriangleStrip:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		case rhi::PrimitiveType::TriangleFan:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
		case rhi::PrimitiveType::PatchList:
			return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
		default:
			assert(!"invalid enum");
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		}
	}

	VkViewport convertViewport(const Viewport& viewport)
	{
		return VkViewport{ viewport.minX, viewport.minY, viewport.maxX - viewport.minX, viewport.maxY - viewport.minY, viewport.minZ, viewport.maxZ };
	}

	GraphicsPipelineVk::~GraphicsPipelineVk()
	{
		if (pipelineCache != VK_NULL_HANDLE)
		{
			vkDestroyPipelineCache(m_Context.device, pipelineCache, nullptr);
		}
		vkDestroyPipelineLayout(m_Context.device, pipelineLayout, nullptr);
		vkDestroyPipeline(m_Context.device, pipeline, nullptr);
	}

	Object GraphicsPipelineVk::getNativeObject(NativeObjectType type) const
	{
		switch (type)
		{
		case NativeObjectType::VK_Pipeline:
			return static_cast<Object>(pipeline);
		case NativeObjectType::VK_PipelineCache:
			return static_cast<Object>(pipelineCache);
		default:
			return nullptr;
		}
	}

	bool GraphicsPipelineVk::getPipelineCacheData(void* pData, size_t* pDataSize) const
	{
		assert(pDataSize != nullptr);
		VkResult err = vkGetPipelineCacheData(m_Context.device, pipelineCache, pDataSize, nullptr);
		CHECK_VK_RESULT(err);
		if (pData == nullptr)
		{
			return err != VK_SUCCESS;
		}
		err = vkGetPipelineCacheData(m_Context.device, pipelineCache, pDataSize, pData);
		CHECK_VK_RESULT(err);
		return err != VK_SUCCESS;
	}

	ComputePipelineVk::~ComputePipelineVk()
	{
		if (pipelineCache != VK_NULL_HANDLE)
		{
			vkDestroyPipelineCache(m_Context.device, pipelineCache, nullptr);
		}
		vkDestroyPipelineLayout(m_Context.device, pipelineLayout, nullptr);
		vkDestroyPipeline(m_Context.device, pipeline, nullptr);
	}

	Object ComputePipelineVk::getNativeObject(NativeObjectType type) const
	{
		switch (type)
		{
		case NativeObjectType::VK_Pipeline:
			return static_cast<Object>(pipeline);
		case NativeObjectType::VK_PipelineCache:
			return static_cast<Object>(pipelineCache);
		default:
			return nullptr;
		}
	}

	bool ComputePipelineVk::getPipelineCacheData(void* pData, size_t* pDataSize) const
	{
		assert(pDataSize != nullptr);
		VkResult err = vkGetPipelineCacheData(m_Context.device, pipelineCache, pDataSize, nullptr);
		CHECK_VK_RESULT(err);
		if (pData == nullptr)
		{
			return err != VK_SUCCESS;
		}
		err = vkGetPipelineCacheData(m_Context.device, pipelineCache, pDataSize, pData);
		CHECK_VK_RESULT(err);
		return err != VK_SUCCESS;
	}
}