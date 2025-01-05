#include "UploadAllocator.h"

#include "rhi/common/Utils.h"

namespace rhi
{
	UploadAllocator::RingBuffer::RingBuffer(uint64_t maxSize) :mMaxBlockSize(maxSize) {}

	UploadAllocator::RingBuffer::~RingBuffer() = default;

	uint64_t UploadAllocator::RingBuffer::Allocate(uint64_t allocationSize, uint64_t serialID, uint64_t offsetAlignment)
	{
		if (mUsedSize >= mMaxBlockSize)
		{
			return cInvalidOffset;
		}

		const uint64_t remainingSize = (mMaxBlockSize - mUsedSize);
		if (allocationSize > remainingSize)
		{
			return cInvalidOffset;
		}

		uint64_t startOffset = cInvalidOffset;
		uint64_t currentRequestSize = 0;

		// Compute an alignment offset for the buffer if allocating at the end.
		const uint64_t alignmentOffset = AlignUp(mUsedEndOffset, offsetAlignment) - mUsedEndOffset;
		const uint64_t alignedUsedEndOffset = mUsedEndOffset + alignmentOffset;

		// Check if the buffer is NOT split (i.e sub-alloc on ends)
		if (mUsedStartOffset <= mUsedEndOffset)
		{
			// Order is important (try to sub-alloc at end first).
			// This is due to FIFO order where sub-allocs are inserted from left-to-right (when not
			// wrapped).
			if (alignedUsedEndOffset + allocationSize <= mMaxBlockSize)
			{
				startOffset = alignedUsedEndOffset;
				mUsedSize += allocationSize + alignmentOffset;
				currentRequestSize = allocationSize + alignmentOffset;
			}
			else if (allocationSize <= mUsedStartOffset) // Try to sub-alloc at front.
			{
				// Count the space at the end so that a subsequent
				// sub-alloc cannot fail when the buffer is full.
				const uint64_t requestSize = (mMaxBlockSize - mUsedEndOffset) + allocationSize;

				startOffset = 0;
				mUsedSize += requestSize;
				currentRequestSize = requestSize;
			}
		}
		else if (alignedUsedEndOffset + allocationSize <= mUsedStartOffset)
		{
			// Otherwise, buffer is split where sub-alloc must be in-between.
			startOffset = alignedUsedEndOffset;
			mUsedSize += allocationSize + alignmentOffset;
			currentRequestSize = allocationSize + alignmentOffset;
		}

		if (startOffset != cInvalidOffset)
		{
			mUsedEndOffset = startOffset + allocationSize;

			Request request;
			request.endOffset = mUsedEndOffset;
			request.size = currentRequestSize;

			mInflightRequests.push_back(std::move(request));
		}

		return startOffset;
	}

	void UploadAllocator::RingBuffer::Deallocate(uint64_t lastCompletedSerialID)
	{
		for (auto iter = mInflightRequests.begin(); iter != mInflightRequests.end(); ++iter)
		{
			if (iter->serialID <= lastCompletedSerialID)
			{
				mUsedStartOffset = iter->endOffset;
				mUsedSize -= iter->size;
			}
			else
			{
				mInflightRequests.erase(mInflightRequests.begin(), iter - 1);
				break;
			}
		}
	}
}