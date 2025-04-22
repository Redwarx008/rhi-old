#pragma once

#include "rhi/RHIStruct.h"
#include "PipelineBase.h"

namespace rhi
{
	class ComputePipelineBase : public PipelineBase
	{
	public:
		explicit ComputePipelineBase(DeviceBase* device, const ComputePipelineDesc& desc);
		~ComputePipelineBase();
		ResourceType GetType() const override;
	};
}