#pragma once

#include "rhi/RHIStruct.h"
#include "common/RefCounted.h"
#include "common/Ref.hpp"
#include "common/SerialMap.hpp"
#include "CallbackTaskManager.h"
#include "UploadAllocator.h"
#include <memory>
#include <atomic>

namespace rhi
{

	class CommandListBase;
	class CommandEncoder;
	class BufferBase;
	class QueueBase : public RefCounted
	{
	public:
		void APIWriteBuffer(BufferBase* buffer, const void* data, uint64_t dataSize, uint64_t offset);
		void APIWriteTexture(const TextureSlice& dstTexture, const void* data, size_t dataSize, const TextureDataLayout& dataLayout);
		void APIWaitQueue(QueueBase* queue, uint64_t submitID);
		uint64_t APISubmit(CommandListBase* const * commands, uint32_t commandListCount);

		void Tick();
		QueueType GetType() const;
		uint64_t GetLastSubmittedSerial() const;
		uint64_t GetPendingSubmitSerial() const;
		uint64_t GetCompletedSerial() const;
		void AssumeCommandsComplete();
		bool NeedsTick() const;
		bool HasScheduledCommands() const;
		void CheckAndUpdateCompletedSerial();
		void TrackTask(std::unique_ptr<CallbackTask>, uint64_t serial);
		void CopyFromStagingToBuffer(BufferBase* src, uint64_t srcOffset, BufferBase* dst, uint64_t dstOffset, uint64_t size);
	protected:
		explicit QueueBase(DeviceBase* device) noexcept;
		~QueueBase();
		virtual void TickImpl(uint64_t completedSerial) = 0;
		virtual uint64_t SubmitImpl(CommandListBase* const* commands, uint32_t commandListCount) = 0;
		virtual uint64_t QueryCompletedSerial() = 0;
		virtual void CopyFromStagingToBufferImpl(BufferBase* src, uint64_t srcOffset, BufferBase* dst, uint64_t destOffset, uint64_t size) = 0;
		virtual void MarkRecordingContextIsUsed() = 0;

		SerialMap<uint64_t, std::unique_ptr<CallbackTask>> mTasksInFlight;

		std::unique_ptr<UploadAllocator> mUploadAllocator;

		DeviceBase* mDevice;

		QueueType mQueueType;

		std::atomic<uint64_t> mCompletedSerial = 0;
		std::atomic<uint64_t> mLastSubmittedSerial = 0;
	};
}