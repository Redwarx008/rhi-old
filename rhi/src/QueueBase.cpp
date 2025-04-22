#include "QueueBase.h"
#include "DeviceBase.h"
#include "BufferBase.h"
#include "TextureBase.h"
#include "common/Utils.h"
#include "common/Error.h"

namespace rhi
{
	QueueBase::QueueBase(DeviceBase* device) :
		mDevice(device)
	{

	}

	QueueBase::~QueueBase() {}

	QueueType QueueBase::GetType() const
	{
		return mQueueType;
	}

	uint64_t QueueBase::GetLastSubmittedSerial() const
	{
		return mLastSubmittedSerial.load(std::memory_order_acquire);
	}

	uint64_t QueueBase::GetPendingSubmitSerial() const
	{
		return mLastSubmittedSerial.load(std::memory_order_acquire) + 1;
	}

	uint64_t QueueBase::GetCompletedSerial() const
	{
		return mCompletedSerial.load(std::memory_order_acquire);
	}

	void QueueBase::AssumeCommandsComplete()
	{
		uint64_t prev = mLastSubmittedSerial.fetch_add(1ull, std::memory_order_release);
		mCompletedSerial.store(prev + 1u, std::memory_order_release);
	}

	void QueueBase::TrackTask(std::unique_ptr<CallbackTask> callback, uint64_t serial)
	{
		if (serial < GetCompletedSerial())
		{
			mDevice->GetCallbackTaskManager().AddCallbackTask(std::move(callback));
		}
		else
		{
			mTasksInFlight.Push(serial, std::move(callback));
		}
	}

	void QueueBase::Tick()
	{
		uint64_t completedSerial = GetCompletedSerial();
		// Tasks' serials have passed. Move them to the callback task manager. They
		// are ready to be called.
		for (auto& task : mTasksInFlight.IterateUpTo(completedSerial))
		{
			mDevice->GetCallbackTaskManager().AddCallbackTask(std::move(task));
		}
		mTasksInFlight.ClearUpTo(completedSerial);

		mUploadAllocator->Deallocate(completedSerial);

		TickImpl(completedSerial);
	}

	uint64_t QueueBase::APISubmit(CommandListBase* const* commands, uint32_t commandListCount)
	{
		return SubmitImpl(commands, commandListCount);
		//Tick();
	}

	void QueueBase::CheckAndUpdateCompletedSerial()
	{
		uint64_t completedSerial = QueryCompletedSerial();
		uint64_t current = mCompletedSerial.load(std::memory_order_acquire);
		while (uint64_t(completedSerial) > current &&
			!mCompletedSerial.compare_exchange_weak(current, uint64_t(completedSerial),
				std::memory_order_acq_rel)) {}

	}

	bool QueueBase::HasScheduledCommands() const
	{
		return mLastSubmittedSerial.load(std::memory_order_acquire) >
			mCompletedSerial.load(std::memory_order_acquire);
	}

	bool QueueBase::NeedsTick() const
	{
		return HasScheduledCommands() || !mTasksInFlight.Empty();
	}

	void QueueBase::CopyFromStagingToBuffer(BufferBase* src, uint64_t srcOffset, BufferBase* dst, uint64_t dstOffset, uint64_t size)
	{
		CopyFromStagingToBufferImpl(src, srcOffset, dst, dstOffset, size);
		MarkRecordingContextIsUsed();
	}

	void QueueBase::APIWriteBuffer(BufferBase* buffer, const void* data, uint64_t dataSize, uint64_t offset)
	{
		ASSERT(HasFlag(buffer->APIGetUsage(), BufferUsage::CopyDst));
		INVALID_IF(dataSize > buffer->APIGetSize() - offset || offset > buffer->APIGetSize(),
			"Write range (bufferOffset: %u, size: %u) does not fit in %s size (%u).",
			offset, dataSize, buffer, buffer->APIGetSize());

		if (HasFlag(buffer->APIGetUsage(), BufferUsage::MapWrite | BufferUsage::MapRead))
		{
			ASSERT(buffer->APIGetMappedPointer() != nullptr);
			uint8_t* src = static_cast<uint8_t*>(buffer->APIGetMappedPointer()) + offset;
			memcpy(src, data, dataSize);
		}
		else
		{
			// For device visible buffer, we use stage buffer to upload.
			UploadAllocation allocation = mUploadAllocator->Allocate(dataSize, GetPendingSubmitSerial(), 4);
			ASSERT(allocation.mappedAddress != nullptr);
			memcpy(allocation.mappedAddress, data, dataSize);

			CopyFromStagingToBuffer(allocation.buffer, allocation.offset, buffer, offset, dataSize);
		}
	}

	uint64_t ComputeRequiredBytesInCopy(TextureFormat format, const Extent3D& size, uint32_t bytesPerRow, uint32_t rowsPerImage)
	{
		const FormatInfo& formatInfo = GetFormatInfo(format);
		uint32_t widthInBlocks = size.width / formatInfo.blockSize;
		uint32_t heightInBlocks = size.height / blockInfo.height;
	}

	void QueueBase::APIWriteTexture(const TextureSlice& dstTexture, const void* data, size_t dataSize, const TextureDataLayout& dataLayout)
	{
		ASSERT(HasFlag(dstTexture.texture->GetUsage(), TextureUsage::CopyDst));
		ASSERT(dataLayout.bytesPerRow != 0 && dataLayout.rowsPerImage != 0);
		INVALID_IF(dataLayout.offset > dataSize, "Data offset (%u) is greater than the data size (%u).", dataLayout.offset, dataSize);
		TextureFormat format = dstTexture.texture->GetFormat();
		ASSERT(dstTexture.size.width % GetFormatInfo(format).blockSize == 0);
		ASSERT(dstTexture.size.height % GetFormatInfo(format).blockSize == 0);

		uint32_t alignedBytesPerRow = dstTexture.size.width / GetFormatInfo(format).blockSize * GetFormatInfo(format).bytesPerBlock;
		uint32_t alignedRowsPerImage = dstTexture.size.height / GetFormatInfo(format).blockSize;

		uint32_t optimalBytesPerRowAlignment = mDevice->GetOptimalBytesPerRowAlignment();
		uint32_t optimallyAlignedBytesPerRow = AlignUp(alignedBytesPerRow, optimalBytesPerRowAlignment);

	}
}