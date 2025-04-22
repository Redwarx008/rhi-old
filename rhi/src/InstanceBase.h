#pragma once

#include "rhi/RHIStruct.h"
#include "common/RefCounted.h"

namespace rhi
{
	class AdapterBase;

	class InstanceBase : public RefCounted
	{
	public:
		//api
		virtual void EnumerateAdapters(AdapterBase** const adapters, uint32_t* adapterCount) = 0;
		// internal
		bool IsDebugLayerEnabled() const;
	protected:
		InstanceBase() = default;
		~InstanceBase() = default;

		bool mDebugLayerEnabled;
	};
}