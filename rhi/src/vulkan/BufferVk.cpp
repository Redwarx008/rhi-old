#include "BufferVk.h"

#include "DeviceVk.h"
#include "ErrorsVk.h"
#include "CommandListVk.h"
#include "../Utils.h"
#include "../Error.h"

#include <algorithm>

namespace rhi::vulkan
{
	VkBufferUsageFlags BufferUsageConvert(BufferUsage usage)
	{
		VkBufferUsageFlags flags = 0;

		if (HasFlag(usage, BufferUsage::Vertex))
		{
			flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		}
		if (HasFlag(usage, BufferUsage::Index))
		{
			flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		}
		if (HasFlag(usage, BufferUsage::Indirect))
		{
			flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
		}
		if (HasFlag(usage, BufferUsage::Uniform))
		{
			flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		}
		if (HasFlag(usage, BufferUsage::Storage))
		{
			flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		}
		if (HasFlag(usage, BufferUsage::CopySrc))
		{
			flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		}
		if (HasFlag(usage, BufferUsage::CopyDest))
		{
			flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		}
		return flags;
	}

	VkAccessFlagBits2 AccessFlagsConvert(BufferUsage usage) {
		VkAccessFlagBits2 flags = 0;

		if (HasFlag(usage, BufferUsage::MapRead)) 
		{
			flags |= VK_ACCESS_2_HOST_READ_BIT;
		}
		if (HasFlag(usage, BufferUsage::MapWrite))
		{
			flags |= VK_ACCESS_2_HOST_WRITE_BIT;
		}
		if (usage & (wgpu::BufferUsage::CopySrc | kInternalCopySrcBuffer)) {
			flags |= VK_ACCESS_TRANSFER_READ_BIT;
		}
		if (usage & wgpu::BufferUsage::CopyDst) {
			flags |= VK_ACCESS_TRANSFER_WRITE_BIT;
		}
		if (usage & wgpu::BufferUsage::Index) {
			flags |= VK_ACCESS_INDEX_READ_BIT;
		}
		if (usage & wgpu::BufferUsage::Vertex) {
			flags |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		}
		if (usage & wgpu::BufferUsage::Uniform) {
			flags |= VK_ACCESS_UNIFORM_READ_BIT;
		}
		if (usage & (wgpu::BufferUsage::Storage | kInternalStorageBuffer)) {
			flags |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		}
		if (usage & kReadOnlyStorageBuffer) {
			flags |= VK_ACCESS_SHADER_READ_BIT;
		}
		if (usage & kIndirectBufferForBackendResourceTracking) {
			flags |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
		}
		if (usage & wgpu::BufferUsage::QueryResolve) {
			flags |= VK_ACCESS_TRANSFER_WRITE_BIT;
		}

		return flags;
	}

	Ref<Buffer> Buffer::Create(Device* device, const BufferDesc& desc)
	{
		Ref<Buffer> buffer = AcquireRef(new Buffer(device, desc));
		if (!buffer->Initialize())
		{
			return nullptr;
		}
		return std::move(buffer);
	}

	Buffer::Buffer(Device* device, const BufferDesc& desc) :
		mSize(desc.size),
		mUsage(desc.usage),
		mInternalUsage(desc.usage),
		mDevice(device)
	{
		// mDevice->Track(this); ?
	}

	bool Buffer::Initialize()
	{
		// Vulkan requires the size to be non-zero.
		uint64_t toAllocatedSize = (std::max)(mSize, 4ull);

		ASSERT_MSG(toAllocatedSize & (3ull << 62ull),
			"Buffer size is HUGE and could cause overflows");

		VkBufferCreateInfo bufferCI{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferCI.size = toAllocatedSize;
		bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferCI.usage = BufferUsageConvert(mInternalUsage);

		VmaAllocationCreateInfo allocCI{};
		allocCI.usage = VMA_MEMORY_USAGE_AUTO;
		allocCI.priority = 1.0f;
		if ((mInternalUsage & BufferUsage::MapRead) != 0)
		{
			allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		}
		else if ((mInternalUsage & BufferUsage::MapWrite) != 0)
		{
			allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		}
		else
		{
			// device visible memory
			allocCI.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		}

		VkResult err = vmaCreateBuffer(mDevice->GetMemoryAllocator(), &bufferCI, &allocCI, &mHandle, &mAllocation, &mAllocationInfo);
		CHECK_VK_RESULT(err, "Could not create buffer");

		if (err != VK_SUCCESS)
		{
			return false;
		}
		return true;
	}

	void Buffer::MapAsync(MapMode mode, BufferMapCallback callback, void* userData)
	{

	}

	BufferUsage Buffer::GetUsage() const
	{
		return mUsage;
	}

	uint64_t Buffer::GetSize() const
	{
		return mSize;
	}

	uint64_t Buffer::GetAllocatedSize() const
	{
		return mAllocationInfo.size;
	}

	VkBuffer Buffer::GetHandle() const
	{
		return mHandle;
	}

	void Buffer::MarkUsedInPendingCommandList()
	{
		uint64_t serialID = mDevice->GetQueue(QueueType::Graphics);
		assert(serialID >= mLastUsageSerialID);
		mLastUsageSerialID = serialID;
	}

	void Buffer::TransitionUsageNow(CommandList* commandList, BufferUsage usage, ShaderStage stage)
	{
		TrackUsageAndGetResourceBarrier(commandList, usage, stage);
		commandList->EmitBufferBarriers(mDevice.Get());
	}

	void Buffer::TrackUsageAndGetResourceBarrier(CommandList* commandList, BufferUsage usage, ShaderStage shaderStage)
	{


		if (shaderStage == ShaderStage::None)
		{
			// If the buffer isn't used in any shader stages, ignore shader usages. Eg. ignore a uniform
			// buffer that isn't actually read in any shader.
			usage &= ~shaderBufferUsages;
		}

		const bool isMapUsage = (usage & mappableBufferUsages) != 0;
		if (!isMapUsage)
		{
			// Request non CPU usage, so assume the buffer will be used in pending commands.
			MarkUsedInPendingCommandList();
		}

		if (!isMapUsage && (mInternalUsage & mappableBufferUsages) != 0)
		{
			// The buffer is mappable and the requested usage is not map usage, we need to add it
			// into mappableBuffersForEagerTransition, so the buffer can be transitioned back to map
			// usages at end of the submit.
			commandList->GetMappableBuffersForTransition().insert(this);
		}

		const bool readOnly = IsSubset(usage, readOnlyBufferUsages);
		VkAccessFlags srcAccess = 0;
		VkPipelineStageFlags srcStage = 0;

		if (readOnly)
		{
			if ((shaderStage & ShaderStage::Fragment) != 0 &&
				(mReadShaderStages & ShaderStage::Vertex) != 0)
			{
				// There is an implicit vertex->fragment dependency, so if the vertex stage has already
				// waited, there is no need for fragment to wait. Add the fragment usage so we know to
				// wait for it before the next write.
				mReadShaderStages |= ShaderStage::Fragment;
			}

			if (IsSubset(usage, mReadUsage) && IsSubset(shaderStage, mReadShaderStages))
			{
				// This usage and shader stage has already waited for the last write.
				// No need for another barrier.
				return;
			}

			mReadUsage |= usage;
			mReadShaderStages |= shaderStage;

			if (mLastWriteUsage == BufferUsage::None)
			{
				// Read dependency with no prior writes. No barrier needed.
				return;
			}
			// Write -> read barrier.
			srcAccess = VulkanAccessFlags(mLastWriteUsage);
		}
	}
}