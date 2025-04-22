#include "ResourceToDelete.h"

namespace rhi::vulkan
{
	BufferAllocation::BufferAllocation(VkBuffer _buffer, VmaAllocation _allocation)
		:buffer(_buffer), allocation(_allocation) {}

	ImageAllocation::ImageAllocation(VkImage image, VmaAllocation allocation)
		:image(image), allocation(allocation) {}

	ConcurrentBufferAllocation::ConcurrentBufferAllocation(VkBuffer _buffer, VmaAllocation _allocation, uint32_t _queueCount)
		:buffer(_buffer), allocation(_allocation), refQueueCount(_queueCount) {}

	ConcurrentImageAllocation::ConcurrentImageAllocation(VkImage _image, VmaAllocation _allocation, uint32_t _queueCount)
		:image(image), allocation(allocation), refQueueCount(_queueCount) {}
}