#include "ComputePipelineBase.h"

namespace rhi
{
	ComputePipelineBase::ComputePipelineBase(DeviceBase* device, const ComputePipelineDesc& desc) :
		PipelineBase(device, desc)
	{

	}

	ComputePipelineBase::~ComputePipelineBase() {}

	ResourceType ComputePipelineBase::GetType() const
	{
		return ResourceType::ComputePipeline;
	}
}