#pragma once

#include "rhi/rhi.h"
#include "Commands.h"
#include "CommandAllocator.h"
#include "Ref.hpp"
#include <vector>
namespace rhi
{
	class CommandEncoder;
	class CommandList final : public ICommandList
	{
	public:
		Ref<CommandList> Create(CommandEncoder* encoder);
		void Wait(ICommandList* other) override;
	private:
		explicit CommandList(CommandEncoder* encoder);
		std::vector<Ref<CommandList>> mWaitCommandLists;
		CommandIterator mCommandIter;
	};

	class CommandEncoder : public ICommandEncoder
	{
	public:
		ICommandList* Finish() override;
		CommandAllocator* GetAllocator();
	private:
		CommandAllocator mCommandAllocator;
	};
}