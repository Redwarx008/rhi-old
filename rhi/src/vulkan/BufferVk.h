#pragma once

#include "rhi/rhi.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "../Ref.hpp"

namespace rhi::vulkan
{
	class Device;
	class CommandList;
	class Buffer final : public IBuffer
	{
	public:
		static Ref<Buffer> Create(Device* device, const BufferDesc& desc);
		~Buffer();
		VkBuffer GetHandle() const;
		void TransitionUsageNow(CommandList* commandList, BufferUsage usage, ShaderStage stage);
		void TrackUsageAndGetResourceBarrier(CommandList* commandList, BufferUsage usage, ShaderStage stage);

		BufferUsage GetUsage() const override;
		uint64_t GetSize() const override;
		void MapAsync(MapMode mode, BufferMapCallback callback, void* userData) override;
	private:
		explicit Buffer(Device* device, const BufferDesc& desc);
		bool Initialize();
		void MarkUsedInPendingCommandList();
		const BufferUsage mUsage = BufferUsage::None;
		const BufferUsage mInternalUsage = BufferUsage::None;
		const uint64_t mSize = 0;
		
		Ref<Device> mDevice;
		VmaAllocationInfo mAllocationInfo{};
		VmaAllocation mAllocation;
		VkBuffer mHandle = VK_NULL_HANDLE;
		QueueType mLastUsedQueue = QueueType::Graphics;
		uint64_t mLastUsageSerialID = 0;

		// Track which usage was the last to write to the buffer.
		BufferUsage mLastWriteUsage = BufferUsage::None;
		ShaderStage mLastWriteShaderStage = ShaderStage::None;

		// Track which usages have read the buffer since the last write.
		BufferUsage mReadUsage = BufferUsage::None;
		ShaderStage mReadShaderStages = ShaderStage::None;
	};
}