#pragma once

#include <memory>
#include <vector>
#include <limits>

namespace rhi
{
	namespace detail
	{
		constexpr uint32_t cEndOfBlock = std::numeric_limits<uint32_t>::max();
		constexpr uint32_t cAdditionalData = std::numeric_limits<uint32_t>::max() - 1;
	}

	struct Block
	{
		size_t size;
		std::unique_ptr<char[]> block;
	};

	class CommandAllocator
	{
	public:
		CommandAllocator();
		~CommandAllocator();
		bool Reset();
		bool IsEmpty() const;
	private:
		static constexpr uint32_t cMaxSupportedAlignment = 8;
		// To avoid checking for overflows at every step of the computations we compute an upper
		// bound of the space that will be needed in addition to the command data.
		static constexpr uint32_t cWorstCaseAdditionalSize =
			sizeof(uint32_t) + cMaxSupportedAlignment + alignof(uint32_t);

		static constexpr uint32_t cDefaultBaseAllocationSize = 2048;

		char* Allocate(uint32_t commandId, size_t commandSize, size_t commandAlignment);
		char* AllocateInNewBlock(uint32_t commandId, size_t commandSize, size_t commandAlignment);
		bool GetNewBlock(size_t minimumSize);
		void ResetPtr();
		std::vector<Block> mBlocks;
		// Data used for the block range at initialization so that the first call to Allocate sees
		// there is not enough space and calls GetNewBlock. This avoids having to special case the
		// initialization in Allocate.
		uint32_t mPlaceholderSpace[2] = { 0, 0 };
		size_t mLastAllocationSize = cDefaultBaseAllocationSize;
		char* mCurrentPtr = nullptr;
		char* mEndPtr = nullptr;
	};
}