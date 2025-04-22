#pragma once

#include <vulkan/vulkan.h>

namespace rhi::vulkan
{
	struct DescriptorSetAllocation
	{
		VkDescriptorSet set;
		uint32_t poolIndex;
		uint32_t setIndex;
	};
}