#pragma once

#include "common/RefCounted.h"
#include "common/Ref.hpp"
#include "PassResourceUsage.h"
#include "CommandAllocator.h"

namespace rhi
{
	class CommandEncoder;
	class CommandListBase : public RefCounted
	{
	public:
		const CommandListResourceUsage& GetResourceUsages() const;
	protected:
		explicit CommandListBase(DeviceBase* device, CommandEncoder* encoder);
		~CommandListBase();
		DeviceBase* mDevice;
		CommandIterator mCommandIter;
		CommandListResourceUsage mResourceUsages;
	};
}