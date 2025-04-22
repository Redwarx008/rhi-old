#pragma once

#include "rhi/rhi.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "../common/Ref.hpp"
#include "../common/RefCounted.h"
#include "../BufferBase.h"
#include <array>

namespace rhi::vulkan
{
	class Queue;
	class Buffer final :  public BufferBase
	{
	public:

		static Ref<Buffer> Create(DeviceBase* device, const BufferDesc& desc);
		~Buffer();
		// interface
		void* APIGetMappedPointer() override;
		// internal
		VkBuffer GetHandle() const;
		void TransitionOwnership(Queue* queue, Queue* receivingQueue);
		void TransitionUsageNow(Queue* queue, BufferUsage usage, ShaderStage stage = ShaderStage::None);
		void TrackUsageAndGetResourceBarrier(Queue* queue, BufferUsage usage, ShaderStage stage = ShaderStage::None);
		uint64_t GetAllocatedSize() const;
		ResourceType GetType() const override;
	private:
		explicit Buffer(DeviceBase* device, const BufferDesc& desc) noexcept;
		bool Initialize();
		void DestroyImpl() override;
		void MarkUsedInPendingCommandList(Queue* queue);
		void MapAsyncImpl(QueueBase* queue, MapMode mode) override;
		VmaAllocationInfo mAllocationInfo{};
		VmaAllocation mAllocation;
		VkBuffer mHandle = VK_NULL_HANDLE;
	};
}

