#include "PassEncoder.h"
#include "PipelineBase.h"
#include "PipelineLayoutBase.h"
#include "Commands.h"
#include "common/Error.h"

namespace rhi
{
	PassEncoder::PassEncoder(CommandEncoder* encoder, EncodingContext& encodingContext) :
		mCommandEncoder(encoder),
		mEncodingContext(encodingContext)
	{

	}

	PassEncoder::~PassEncoder() {}

	void PassEncoder::RecordSetBindSet(BindSetBase* set, uint32_t setIndex, uint32_t dynamicOffsetCount = 0, const uint32_t* dynamicOffsets = nullptr)
	{
		INVALID_IF(mLastPipeline == nullptr, "Must set pipeline before set BindSet.");
		CommandAllocator& allocator = mEncodingContext.GetCommandAllocator();
		SetBindSetCmd* cmd = allocator.Allocate<SetBindSetCmd>(Command::SetBindSet);
		cmd->set = set;
		cmd->setIndex = setIndex;
		cmd->dynamicOffsetCount = dynamicOffsetCount;
		if (dynamicOffsetCount > 0)
		{
			uint32_t* offsets = allocator.AllocateData<uint32_t>(dynamicOffsetCount);
			memcpy(offsets, dynamicOffsets, dynamicOffsetCount * sizeof(uint32_t));
		}
	}

	void PassEncoder::APISetPushConstant(const void* data, uint32_t size)
	{
		INVALID_IF(mLastPipeline == nullptr, "Must set pipeline before set pushConstant.");
		ASSERT(data != nullptr);
		ASSERT(size <= mLastPipeline->GetLayout()->GetPushConstants().size);
		INVALID_IF(size % 4 != 0, "PushConstant size (%u) is not  a multiple of 4.", size);

		CommandAllocator& allocator = mEncodingContext.GetCommandAllocator();
		SetPushConstantCmd* cmd = allocator.Allocate<SetPushConstantCmd>(Command::SetPushConstant);
		cmd->size = mLastPipeline->GetLayout()->GetPushConstants().size;
		
		uint8_t* pData = allocator.AllocateData<uint8_t>(size);
		memcpy(pData, data, size);
	}

	void PassEncoder::APIBeginDebugLabel(std::string_view label, Color color = Color())
	{
		CommandAllocator& allocator = mEncodingContext.GetCommandAllocator();
		BeginDebugLabelCmd* cmd = allocator.Allocate<BeginDebugLabelCmd>(Command::BeginDebugLabel);
		EnsureValidString(allocator, label, &cmd->labelLength);
		cmd->color = color;
		++mDebugLabelCount;
	}

	void PassEncoder::APIEndDebugLabel()
	{
		INVALID_IF(mDebugLabelCount == 0, "EndDebugLabel called when no DebugLabels are begin.");

		CommandAllocator& allocator = mEncodingContext.GetCommandAllocator();
		allocator.Allocate<EndDebugLabelCmd>(Command::EndDebugLabel);
		--mDebugLabelCount;
	}
}