#pragma once

#include "PipelineBase.h"
#include "rhi/RHIStruct.h"

namespace rhi
{
	class RenderPipelineBase : public PipelineBase
	{
	public:
		explicit RenderPipelineBase(DeviceBase* device, const RenderPipelineDesc& desc) noexcept;
		~RenderPipelineBase();
		ResourceType GetType() const override;
	protected:
		void ResolveVertexInputOffsetAndStride();

		std::vector<VertexInputAttribute> mVertexInputAttributes;
		std::vector<TextureFormat> mColorAttachmentFormats;
		TextureFormat mDepthStencilFormat = TextureFormat::Undefined;
		BlendState mBlendState;
		RasterState mRasterState;
		SampleState mSampleState;
		DepthStencilState mDepthStencilState;
		uint32_t mViewportCount = 1;
		uint32_t mPatchControlPoints = 0;
	};
}