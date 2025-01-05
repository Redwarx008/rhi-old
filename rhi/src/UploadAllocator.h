#pragma once

#include "rhi/rhi.h"
#include "Ref.hpp"
#include <cstdint>
#include <limits>
#include <vector>

namespace rhi
{
	class UploadAllocator
	{
	private:
		static constexpr uint64_t cRingBufferSize = 4 * 1024 * 1024;

		class RingBuffer
		{
			Ref<IBuffer> buffer;
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
	};
}