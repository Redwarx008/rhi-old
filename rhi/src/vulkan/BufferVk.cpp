#include "BufferVk.h"

#include "DeviceVk.h"
#include "ErrorsVk.h"
#include "CommandListVk.h"
#include "../Utils.h"

namespace rhi::vulkan
{
	VkBufferUsageFlags BufferUsageConvert(BufferUsage usage)
	{
		VkBufferUsageFlags flags = 0;

		if ((usage & BufferUsage::Vertex) != 0)
		{
			flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		}
		if ((usage & BufferUsage::Index) != 0)
		{
			flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		}
		if ((usage & BufferUsage::Indirect) != 0)
		{
			flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
		}
		if ((usage & BufferUsage::Uniform) != 0)
		{
			flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		}
		if ((usage & BufferUsage::Storage) != 0)
		{
			flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
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
		VkBufferCreateInfo bufferCI{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferCI.size = mSize;
		bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		bufferCI.usage = BufferUsageConvert(mInternalUsage) | 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT;

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
		constexpr BufferUsage shaderBufferUsages =
			BufferUsage::Uniform | BufferUsage::Storage;
		constexpr BufferUsage mappableBufferUsages =
			BufferUsage::MapRead | BufferUsage::MapWrite;
		constexpr BufferUsage readOnlyBufferUsages =
			BufferUsage::MapRead | BufferUsage::CopySrc | BufferUsage::Index |
			BufferUsage::Vertex | BufferUsage::Uniform;

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

		}
	}
}