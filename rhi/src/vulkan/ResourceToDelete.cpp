#include "ResourceToDelete.h"

namespace rhi::vulkan
{
	BufferAllocation::BufferAllocation(VkBuffer _buffer, VmaAllocation _allocation)
		:buffer(_buffer), allocation(_allocation) {}

	ImageAllocation::ImageAllocation(VkImage image, VmaAllocation allocation)
		:image(image), allocation(allocation) {}
}