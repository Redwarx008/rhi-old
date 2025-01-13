#include "BufferVk.h"

#include "DeviceVk.h"
#include "ErrorsVk.h"
#include "CommandListVk.h"
#include "../Utils.h"
#include "../Error.h"

#include <algorithm>

namespace rhi::vulkan
{
	constexpr BufferUsage cShaderBufferUsages =
		BufferUsage::Uniform | BufferUsage::Storage;
	constexpr BufferUsage cMappableBufferUsages =
		BufferUsage::MapRead | BufferUsage::MapWrite;
	constexpr BufferUsage cReadOnlyBufferUsages =
		BufferUsage::MapRead | BufferUsage::CopySrc | BufferUsage::Index |
		BufferUsage::Vertex | BufferUsage::Uniform;

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
		if (HasFlag(usage, BufferUsage::CopyDst))
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
		if (HasFlag(usage, BufferUsage::CopySrc))
		{
			flags |= VK_ACCESS_2_TRANSFER_READ_BIT;
		}
		if (HasFlag(usage, BufferUsage::CopyDst))
		{
			flags |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
		}
		if (HasFlag(usage, BufferUsage::Index))
		{
			flags |= VK_ACCESS_2_INDEX_READ_BIT;
		}
		if (HasFlag(usage, BufferUsage::Vertex))
		{
			flags |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
		}
		if (HasFlag(usage, BufferUsage::Uniform))
		{
			flags |= VK_ACCESS_2_UNIFORM_READ_BIT;
		}
		if (HasFlag(usage, BufferUsage::Storage))
		{
			flags |= VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
		}
		if (HasFlag(usage, BufferUsage::QueryResolve))
		{
			flags |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
		}

		return flags;
	}

	VkPipelineStageFlags2 PipelineStageConvert(BufferUsage usage, ShaderStage shaderStage) {
		VkPipelineStageFlags2 flags = 0;

		if (HasFlag(usage, cMappableBufferUsages))
		{
			flags |= VK_PIPELINE_STAGE_2_HOST_BIT;
		}
		if (HasFlag(usage, (BufferUsage::CopySrc | BufferUsage::CopyDst)))
		{
			flags |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		}
		if (HasFlag(usage, (BufferUsage::Index | BufferUsage::Vertex)))
		{
			flags |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
		}
		if (HasFlag(usage, cShaderBufferUsages))
		{
			if (HasFlag(shaderStage, ShaderStage::Vertex))
			{
				flags |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
			}
			if (HasFlag(shaderStage, ShaderStage::Fragment))
			{
				flags |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			}
			if (HasFlag(shaderStage, ShaderStage::Compute))
			{
				flags |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			}
		}
		if (HasFlag(usage, BufferUsage::QueryResolve))
		{
			flags |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;
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

		ASSERT_MSG(!(toAllocatedSize & (3ull << 62ull)),
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
			// No CPU access
			constexpr uint64_t bigSize = 4ull * 1024ull * 1024ull;
			if (toAllocatedSize >= bigSize)
			{
				allocCI.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
			}
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
			usage &= ~cShaderBufferUsages;
		}

		const bool isMapUsage = HasFlag(usage, cMappableBufferUsages);
		if (!isMapUsage)
		{
			// Request non CPU usage, so assume the buffer will be used in pending commands.
			MarkUsedInPendingCommandList();
		}

		if (!isMapUsage && HasFlag(mInternalUsage, cMappableBufferUsages))
		{
			// The buffer is mappable and the requested usage is not map usage, we need to add it
			// into mappableBuffersForEagerTransition, so the buffer can be transitioned back to map
			// usages at end of the submit.
			commandList->GetMappableBuffersForTransition().insert(this);
		}

		const bool readOnly = IsSubset(usage, cReadOnlyBufferUsages);
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
			srcAccess = AccessFlagsConvert(mLastWriteUsage);
			srcStage = PipelineStageConvert(mLastWriteUsage, mLastWriteShaderStage);
		}
		else
		{
			bool skipBarrier = false;

			// vkQueueSubmit does an implicit domain and visibility operation. For HOST_COHERENT
			// memory, we can ignore read (host)->write barriers. However, we can't necessarily
			// skip the barrier if mReadUsage == MapRead, as we could still need a barrier for
			// the last write. Instead, pretend the last host read didn't happen.
			mReadUsage &= ~BufferUsage::MapRead;

			if ((mLastWriteUsage == BufferUsage::None && mReadUsage == BufferUsage::None) ||
				IsSubset(usage | mLastWriteUsage | mReadUsage, cMappableBufferUsages)) 
			{
				// The buffer has never been used before, or the dependency is map->map. We don't need a
				// barrier.
				skipBarrier = true;
			}
			else if (mReadUsage == BufferUsage::None)
			{
				// No reads since the last write.
				// Write -> write barrier.
				srcAccess = AccessFlagsConvert(mLastWriteUsage);
				srcStage = PipelineStageConvert(mLastWriteUsage, mLastWriteShaderStage);
			}
			else
			{
				// Read -> write barrier.
				srcAccess = AccessFlagsConvert(mReadUsage);
				srcStage = PipelineStageConvert(mReadUsage, mReadShaderStages);
			}

			mLastWriteUsage = usage;
			mLastWriteShaderStage = shaderStage;

			mReadUsage = BufferUsage::None;
			mReadShaderStages = ShaderStage::None;

			if (skipBarrier)
			{
				return;
			}
		}
		if (isMapUsage) {
			// CPU usage, but a pipeline barrier is needed, so mark the buffer as used within the
			// pending commands.
			MarkUsedInPendingCommandList();
		}

		commandList->AddBufferBarrier(srcAccess, AccessFlagsConvert(usage),
			srcStage, PipelineStageConvert(usage, shaderStage));
	}
}