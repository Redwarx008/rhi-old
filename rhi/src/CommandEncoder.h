#pragma once

#include "rhi/rhi.h"
#include "Commands.h"
#include "Ref.hpp"
#include <vector>
namespace rhi
{
	class CommandList final : public ICommandList
	{
	public:
		explicit CommandList(std::vector<Command>&& commands);
		void Wait(ICommandList* other) override;
	private:
		std::vector<Ref<CommandList>> mWaitCommandLists;
		std::vector<Command> mCommands;
	};

	class CommandEncoder : public ICommandEncoder
	{
	public:
		ICommandList* Finish() override;
	private:
		std::vector<Command> mCommands{2048};
	};

	class TransferCommandEncoder final : public ITransferCommandEncoder
	{
	public:

	};
}