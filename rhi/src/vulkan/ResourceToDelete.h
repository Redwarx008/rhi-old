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

	struct ConcurrentBufferAllocation
	{
		ConcurrentBufferAllocation(VkBuffer _buffer, VmaAllocation _allocation, uint32_t _queueCount = 0);
		VkBuffer buffer;
		VmaAllocation allocation;
		uint32_t refQueueCount;
	};

	struct ImageAllocation
	{
		ImageAllocation(VkImage _image, VmaAllocation _allocation);
		VkImage image;
		VmaAllocation allocation;
	};

	struct ConcurrentImageAllocation
	{
		ConcurrentImageAllocation(VkImage _image, VmaAllocation _allocation, uint32_t _queueCount = 0);
		VkImage image;
		VmaAllocation allocation;
		uint32_t refQueueCount;
	};
}