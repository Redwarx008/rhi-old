#pragma once

#include "rhi/rhi.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "../Ref.hpp"

namespace rhi::vulkan
{
	class Device;
	class Buffer final : public IBuffer
	{
	public:
		static Ref<Buffer> Create(Device* device, const BufferDesc& desc);
		BufferUsage GetUsage() const override;
		uint64_t GetSize() const override;
		void MapAsync(MapMode mode, BufferMapCallback callback, void* userData) override;
		
		~Buffer();



	private:
		explicit Buffer(Device* device, const BufferDesc& desc);
		bool Initialize();
		const BufferUsage mUsage = BufferUsage::None;
		const uint64_t mSize = 0;
		
		Ref<Device> mDevice;
		VmaAllocationInfo mAllocaionInfo{};
		VkBuffer mHandle = VK_NULL_HANDLE;
		QueueType mLastUsedQueue = QueueType::Graphics;
	};
}