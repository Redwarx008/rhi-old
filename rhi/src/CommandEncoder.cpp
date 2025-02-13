#include "CommandEncoder.h"

#include "Utils.h"
namespace rhi
{
	CommandList::CommandList(CommandEncoder* encoder)
		:mCommandIter(encoder->GetAllocator())
	{

	}

	Ref<CommandList> CommandList::Create(CommandEncoder* encoder)
	{
		Ref<CommandList> commands = AcquireRef(new CommandList(encoder));
		return commands;
	}

	void CommandList::Wait(ICommandList* other)
	{
		mWaitCommandLists.push_back(static_cast<CommandList*>(other));
	}

	ICommandList* CommandEncoder::Finish()
	{
		Ref<CommandList> commands = CommandList::Create(this);
		return commands.Detach();
	}
}