#include "RenderPipelinebase.h"
#include "DeviceBase.h"
#include "TextureBase.h"

namespace rhi
{
	RenderPipelineBase::RenderPipelineBase(DeviceBase* device, const RenderPipelineDesc& desc) :
		PipelineBase(device, desc),
		mRasterState(desc.rasterState),
		mBlendState(desc.blendState),
		mSampleState(desc.sampleState),
		mDepthStencilState(desc.depthStencilState),
		mViewportCount(desc.viewportCount),
		mPatchControlPoints(desc.patchControlPoints),
		mDepthStencilFormat(desc.depthStencilFormat)
	{
		for (uint32_t i = 0; i < desc.vertexAttributeCount; ++i)
		{
			mVertexInputAttributes.push_back(desc.vertexAttributes[i]);
		}

		for (uint32_t i = 0; i < desc.colorAttachmentCount; ++i)
		{
			mColorAttachmentFormats[i] = desc.colorAttachmentFormats[i];
		}

		ResolveVertexInputOffsetAndStride();
	}

	void RenderPipelineBase::ResolveVertexInputOffsetAndStride()
	{
				uint32_t bufferSlotUsed = 0;
		for (uint32_t i = 0; i < mVertexInputAttributes.size(); ++i)
		{
			bufferSlotUsed = (std::max)(bufferSlotUsed, mVertexInputAttributes[i].bindingBufferSlot + 1);
		}
		std::vector<uint32_t> autoSetStrides(bufferSlotUsed, 0);
		std::vector<uint32_t> originStrides(bufferSlotUsed, AUTO_COMPUTE);

		for (uint32_t i = 0; i < mVertexInputAttributes.size(); ++i)
		{
			uint32_t bufferSlot = mVertexInputAttributes[i].bindingBufferSlot;
			auto& currentAutoSetStride = autoSetStrides[bufferSlot];

			auto& formatInfo = GetFormatInfo(mVertexInputAttributes[i].format);

			if (mVertexInputAttributes[i].offsetInElement == AUTO_COMPUTE)
			{
				mVertexInputAttributes[i].offsetInElement = currentAutoSetStride;
			}

			if (mVertexInputAttributes[i].elementStride != AUTO_COMPUTE)
			{
				if (originStrides[bufferSlot] != AUTO_COMPUTE &&
					mVertexInputAttributes[i].elementStride != originStrides[bufferSlot])
				{
					LOG_ERROR("stride between elements is not consistent in the same buffer slot.");
				}
				originStrides[bufferSlot] = mVertexInputAttributes[i].elementStride;
			}

			currentAutoSetStride = (std::max)(currentAutoSetStride, mVertexInputAttributes[i].offsetInElement + formatInfo.bytesPerTexel);
		}

		for (uint32_t i = 0; i < mVertexInputAttributes.size(); ++i)
		{
			uint32_t bufferSlot = mVertexInputAttributes[i].bindingBufferSlot;
			if (originStrides[bufferSlot] != AUTO_COMPUTE && originStrides[bufferSlot] >= autoSetStrides[bufferSlot])
			{
				LOG_ERROR("stride is too small");
			}

			if (mVertexInputAttributes[i].elementStride == AUTO_COMPUTE)
			{
				mVertexInputAttributes[i].elementStride = autoSetStrides[bufferSlot];
			}
		}
	}

	ResourceType RenderPipelineBase::GetType() const
	{
		return ResourceType::RenderPipeline;
	}

	RenderPipelineBase::~RenderPipelineBase() {}
}