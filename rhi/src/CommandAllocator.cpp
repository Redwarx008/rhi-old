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

    CommandAllocator::CommandAllocator(CommandAllocator&& other)
        : mBlocks(std::move(other.mBlocks)), mLastAllocationSize(other.mLastAllocationSize)
    {
        other.mBlocks.clear();
        if (!other.IsEmpty())
        {
            mCurrentPtr = other.mCurrentPtr;
            mEndPtr = other.mEndPtr;
        }
        else {
            ResetPtr();
        }
        other.Reset();
    }
    CommandAllocator& CommandAllocator::operator=(CommandAllocator&& other)
    {
        Reset();
        if (!other.IsEmpty())
        {
            std::swap(mBlocks, other.mBlocks);
            mLastAllocationSize = other.mLastAllocationSize;
            mCurrentPtr = other.mCurrentPtr;
            mEndPtr = other.mEndPtr;
        }
        other.Reset();
        return *this;
    }

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
        mEndPtr = reinterpret_cast<char*>(&mPlaceholderSpace[1]); // just get address. no visit
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

    std::vector<Block>&& CommandAllocator::AcquireBlocks()
    {
        assert(mCurrentPtr != nullptr && mEndPtr != nullptr);
        assert(IsPtrAligned(mCurrentPtr, alignof(uint32_t)));
        assert(mCurrentPtr + sizeof(uint32_t) <= mEndPtr);
        *reinterpret_cast<uint32_t*>(mCurrentPtr) = detail::cEndOfBlock;

        mCurrentPtr = nullptr;
        mEndPtr = nullptr;
        return std::move(mBlocks);
    }


    CommandIterator::CommandIterator(CommandAllocator allocator) : mBlocks(allocator.AcquireBlocks())
    {
        Reset();
    }

    CommandIterator::CommandIterator()
    {
        Reset();
    }

    CommandIterator::~CommandIterator() = default;

    CommandIterator::CommandIterator(CommandIterator&& other)
    {
        if (!other.IsEmpty()) 
        {
            mBlocks = std::move(other.mBlocks);
            other.Reset();
        }
        Reset();
    }

    CommandIterator& CommandIterator::operator=(CommandIterator&& other)
    {
        assert(IsEmpty());
        if (!other.IsEmpty())
        {
            mBlocks = std::move(other.mBlocks);
            other.Reset();
        }
        Reset();
        return *this;
    }

    bool CommandIterator::NextCommandId(uint32_t* commandId)
    {
        char* idPtr = AlignPtr(mCurrentPtr, alignof(uint32_t));
        assert(idPtr == reinterpret_cast<char*>(&mEndOfBlock) ||
            idPtr + sizeof(uint32_t) <=
            mBlocks[mCurrentBlock].block.get() + mBlocks[mCurrentBlock].size);

        uint32_t id = *reinterpret_cast<uint32_t*>(idPtr);

        if (id != detail::cEndOfBlock)
        {
            mCurrentPtr = idPtr + sizeof(uint32_t);
            *commandId = id;
            return true;
        }
        return NextCommandIdInNewBlock(commandId);
    }

    bool CommandIterator::NextCommandIdInNewBlock(uint32_t* commandId)
    {
        mCurrentBlock++;
        if (mCurrentBlock >= mBlocks.size()) {
            Reset();
            *commandId = detail::cEndOfBlock;
            return false;
        }
        mCurrentPtr = AlignPtr(mBlocks[mCurrentBlock].block.get(), alignof(uint32_t));
        return NextCommandId(commandId);
    }

    void CommandIterator::Reset()
    {
        mCurrentBlock = 0;

        if (mBlocks.empty())
        {
            // This will case the first NextCommandId call to try to move to the next block and stop
            // the iteration immediately, without special casing the initialization.
            mCurrentPtr = reinterpret_cast<char*>(&mEndOfBlock);
        }
        else
        {
            mCurrentPtr = AlignPtr(mBlocks[0].block.get(), alignof(uint32_t));
        }
    }

    void CommandIterator::clear()
    {
        if (IsEmpty()) {
            return;
        }

        mCurrentPtr = reinterpret_cast<char*>(&mEndOfBlock);
        mBlocks.clear();
        Reset();
    }

    bool CommandIterator::IsEmpty() const
    {
        return mBlocks.empty();
    }

    void* CommandIterator::NextCommand(size_t commandSize, size_t commandAlignment)
    {
        char* commandPtr = AlignPtr(mCurrentPtr, commandAlignment);
        assert(commandPtr + sizeof(commandSize) <=
            mBlocks[mCurrentBlock].block.get() + mBlocks[mCurrentBlock].size);

        mCurrentPtr = commandPtr + commandSize;
        return commandPtr;
    }

    void* CommandIterator::NextData(size_t dataSize, size_t dataAlignment)
    {
        uint32_t id;
        bool hasId = NextCommandId(&id);
        assert(hasId);
        assert(id == detail::cAdditionalData);

        return NextCommand(dataSize, dataAlignment);
    }
}