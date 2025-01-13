#include "CommandAllocator.h"
#include "Utils.h"

#include <cassert>

namespace rhi
{
    CommandAllocator::CommandAllocator()
    {
        ResetPtr();
    }

    CommandAllocator::~CommandAllocator() = default;

	char* CommandAllocator::Allocate(uint32_t commandId, size_t commandSize, size_t commandAlignment)
	{
        assert(mCurrentPtr != nullptr);
        assert(mEndPtr != nullptr);
        assert(commandId != detail::cEndOfBlock);

        // It should always be possible to allocate one id, for kEndOfBlock tagging,
        assert(IsPtrAligned(mCurrentPtr, alignof(uint32_t)));
        assert(mEndPtr >= mCurrentPtr);
        assert(static_cast<size_t>(mEndPtr - mCurrentPtr) >= sizeof(uint32_t));

        // The memory after the ID will contain the following:
        //   - the current ID
        //   - padding to align the command, maximum kMaxSupportedAlignment
        //   - the command of size commandSize
        //   - padding to align the next ID, maximum alignof(uint32_t)
        //   - the next ID of size sizeof(uint32_t)

        // This can't overflow because by construction mCurrentPtr always has space for the next
        // ID.
        size_t remainingSize = static_cast<size_t>(mEndPtr - mCurrentPtr);

        // The good case were we have enough space for the command data and upper bound of the
        // extra required space.
        if ((remainingSize >= cWorstCaseAdditionalSize) &&
            (remainingSize - cWorstCaseAdditionalSize >= commandSize))
        {
            uint32_t* idAlloc = reinterpret_cast<uint32_t*>(mCurrentPtr);
            *idAlloc = commandId;

            char* commandAlloc = AlignPtr(mCurrentPtr + sizeof(uint32_t), commandAlignment);
            mCurrentPtr = AlignPtr(commandAlloc + commandSize, alignof(uint32_t));

            return commandAlloc;
        }
        return AllocateInNewBlock(commandId, commandSize, commandAlignment);
	}

    char* CommandAllocator::AllocateInNewBlock(uint32_t commandId, size_t commandSize, size_t commandAlignment)
    {
        // When there is not enough space, we signal the kEndOfBlock, so that the iterator knows
        // to move to the next one. kEndOfBlock on the last block means the end of the commands.
        uint32_t* idAlloc = reinterpret_cast<uint32_t*>(mCurrentPtr);
        *idAlloc = detail::cEndOfBlock;

        // We'll request a block that can contain at least the command ID, the command and an
        // additional ID to contain the kEndOfBlock tag.
        size_t requestedBlockSize = commandSize + cWorstCaseAdditionalSize;

        // The computation of the request could overflow.
        if (requestedBlockSize <= commandSize)
        {
            return nullptr;
        }

        if (!GetNewBlock(requestedBlockSize))
        {
            return nullptr;
        }
        return Allocate(commandId, commandSize, commandAlignment);
    }

    bool CommandAllocator::GetNewBlock(size_t minimumSize)
    {
        // Allocate blocks doubling sizes each time, to a maximum of 16k (or at least minimumSize).
        mLastAllocationSize = std::max(minimumSize, std::min(mLastAllocationSize * 2, size_t(16384)));

        auto block = std::unique_ptr<char[]>(new (std::nothrow) char[mLastAllocationSize]);
        if (block == nullptr)
        {
            return false;
        }

        mCurrentPtr = AlignPtr(block.get(), alignof(uint32_t));
        mEndPtr = block.get() + mLastAllocationSize;
        mBlocks.push_back({ mLastAllocationSize, std::move(block) });
        return true;
    }

    void CommandAllocator::ResetPtr()
    {
        mCurrentPtr = reinterpret_cast<char*>(&mPlaceholderSpace[0]);
        mEndPtr = reinterpret_cast<char*>(&mPlaceholderSpace[1]);
    }

    bool CommandAllocator::Reset()
    {
        ResetPtr();
        mBlocks.clear();
        mLastAllocationSize = cDefaultBaseAllocationSize;
    }

    bool CommandAllocator::IsEmpty() const
    {
        return mCurrentPtr == reinterpret_cast<const char*>(&mPlaceholderSpace[0]);
    }
}