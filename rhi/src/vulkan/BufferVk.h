#pragma once

#include "rhi/rhi.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace rhi::vulkan
{
	class Buffer : public IBuffer
	{
	public:
		explicit Buffer(BufferDesc desc, const VmaAllocator& allocator)
			m_Allocator(allocator)
		{
		}

		~Buffer();

		VkBuffer buffer = VK_NULL_HANDLE;
		VmaAllocationInfo allocaionInfo{};
	private:
		const BufferUsage mUsage = BufferUsage::None;
		const uint64_t mSize = 0;
		const VmaAllocator& m_Allocator;
	};
}