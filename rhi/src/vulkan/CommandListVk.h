#pragma once

#include "../common/Ref.hpp"
#include "../CommandListBase.h"

namespace rhi
{
	class BeginRenderPassCmd;
}

namespace rhi::vulkan
{
	class CommandRecordContext;
	class Queue;
	class Device;

	class CommandList final : public CommandListBase
	{
	public:
		static Ref<CommandList> Create(Device* device, CommandEncoder* encoder);
		void RecordCommands(Queue* queue);
	private:
		explicit CommandList(Device* device, CommandEncoder* encoder) noexcept;
		void RecordRenderPass(Queue* queue, BeginRenderPassCmd* renderPassCmd);
	};
}