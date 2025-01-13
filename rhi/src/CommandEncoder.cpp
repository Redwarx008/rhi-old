#include "CommandEncoder.h"

#include "Utils.h"
namespace rhi
{
	CommandList::CommandList(std::vector<Command>&& commands) : mCommands(std::move(commands)) {}

	void CommandList::Wait(ICommandList* other)
	{
		mWaitCommandLists.push_back(static_cast<CommandList*>(other));
	}

	ICommandList* CommandEncoder::Finish()
	{
		Ref<CommandList> commands = AcquireRef(new CommandList(std::move(mCommands)));
		return commands.Detach();
	}
}