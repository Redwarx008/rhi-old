#pragma once

#include <vk_mem_alloc.h>

namespace rhi::vulkan
{
	struct BufferAllocation
	{
		BufferAllocation(VkBuffer _buffer, VmaAllocation _allocation);
		VkBuffer buffer;
		VmaAllocation allocation;
	};

	struct ImageAllocation
	{
		ImageAllocation(VkImage _image, VmaAllocation _allocation);
		VkImage image;
		VmaAllocation allocation;
	};
}