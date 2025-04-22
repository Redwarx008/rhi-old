#pragma once

#include "CommandRecordContextVk.h"
#include "VkResourceDeleter.h"
#include "../QueueBase.h"
#include "../common/RefCounted.h"
#include "../common/Ref.hpp"
#include "../common/MutexProtected.hpp"
#include "../common/SerialQueue.hpp"

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>


namespace rhi::vulkan
{
	class Device;
	class DescriptorSetAllocator;

	class Queue final : public QueueBase
	{
	public:
		static Ref<Queue> Create(Device* device, uint32_t family);
		~Queue();
		// internal 
		CommandRecordContext* GetPendingRecordingContext();
		MutexProtected<VkResourceDeleter>& GetDeleter();
		Device* GetDevice() const;
		uint32_t GetQueueFamilyIndex() const;
		void EnqueueDeferredDeallocation(DescriptorSetAllocator* allocator);
		void SubmitPendingCommands();
	private:
		explicit Queue(Device* device, uint32_t family) noexcept;
		void Initialize();
		void TickImpl(uint64_t completedSerial) override;
		uint64_t SubmitImpl(CommandListBase* const* commands, uint32_t commandListCount) override;
		uint64_t QueryCompletedSerial() override;
		void MarkRecordingContextIsUsed() override;
		void CopyFromStagingToBufferImpl(BufferBase* src, uint64_t srcOffset, BufferBase* dst, uint64_t destOffset, uint64_t size) override;
		void RecycleCompletedCommandBuffer(uint64_t completedSerial);
		void SetTrackingSubmitSemaphore();
		CommandPoolAndBuffer GetOrCreateCommandPoolAndBuffer();
		void NextRecordingContext();
		Device* mDevice;

		uint32_t mQueueFamilyIndex;
		VkQueue mHandle;

		VkSemaphore mTrackingSubmitSemaphore;

		CommandRecordContext mRecordContext;

		SerialQueue<uint64_t, CommandPoolAndBuffer> mCommandBufferInFlight;

		std::vector<CommandPoolAndBuffer> mUnusedCommandBuffer;

		MutexProtected<VkResourceDeleter> mDeleter;
		MutexProtected<SerialQueue<uint64_t, Ref<DescriptorSetAllocator>>> mDescriptorAllocatorsPendingDeallocation;
	};
}

