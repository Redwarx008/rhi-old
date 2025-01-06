#pragma once

#include "Ref.hpp"
#include <cstdint>
#include <limits>
#include <vector>

namespace rhi
{
	class IDevice;
	class IBuffer;

	struct UploadAllocation
	{
		IBuffer* buffer;
		uint64_t offset;
		void* mappedAddress;
	};

	class UploadAllocator
	{
	public:
		explicit UploadAllocator(IDevice* device);
		~UploadAllocator() = default;

		UploadAllocation Allocate(uint64_t allocationSize, uint64_t serialID, uint64_t offsetAlignment);
		void Deallocate(uint64_t lastCompletedSerialID);
	private:
		class RingBuffer
		{
		public:
			explicit RingBuffer(uint64_t maxSize);
			~RingBuffer();

			// Sub-allocate the ring-buffer by requesting a chunk of the specified size. 
			// return the starting offset. 
			uint64_t Allocate(uint64_t allocationSize, uint64_t serialID, uint64_t offsetAlignment = 1);
			void Deallocate(uint64_t lastCompletedSerialID);

			uint64_t GetSize() const;
			uint64_t GetUsedSize() const;
			bool Empty() const;

			static constexpr uint64_t cInvalidOffset = std::numeric_limits<uint64_t>::max();

			Ref<IBuffer> buffer;
		private:
			struct Request
			{
				uint64_t serialID;
				uint64_t endOffset;
				uint64_t size;
			};

			std::vector<Request> mInflightRequests;

			uint64_t mUsedEndOffset = 0;    // Tail of used sub-alloc requests (in bytes).
			uint64_t mUsedStartOffset = 0;  // Head of used sub-alloc requests (in bytes).
			uint64_t mMaxBlockSize = 0;     // Max size of the ring buffer (in bytes).
			uint64_t mUsedSize = 0;         // Size of the sub-alloc requests (in bytes) of the ring buffer.
		};

		static constexpr uint64_t cRingBufferSize = 4 * 1024 * 1024;

		IDevice* mDevice;
	};
}