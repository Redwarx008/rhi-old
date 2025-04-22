#pragma once

#include "../ComputePipelineBase.h"
#include <vulkan/vulkan.h>

namespace rhi::vulkan
{
	class Device;
	class ComputePipeline final : public ComputePipelineBase
	{
	public:
		static Ref<ComputePipeline> Create(Device* device, const ComputePipelineDesc& desc);
		VkPipeline GetHandle() const;
	private:
		explicit ComputePipeline(Device* device, const ComputePipelineDesc& desc) noexcept;
		~ComputePipeline();
		bool Initialize();
		void DestroyImpl() override;

		VkPipeline mHandle = VK_NULL_HANDLE;
	};
}