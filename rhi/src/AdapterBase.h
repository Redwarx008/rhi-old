#pragma once

#include "rhi/RHI.h"
#include <string>
#include "common/Ref.hpp"
#include "common/RefCounted.h"


namespace rhi
{
	class InstanceBase;
	class DeviceBase;
	class AdapterBase : public RefCounted
	{
	public:
		//api
		virtual DeviceBase* CreateDevice(const DeviceDesc& desc) = 0;
		AdapterInfo GetInfo() const;
		Limits GetLimits() const;
		InstanceBase* GetInstance() const;
		//internal
		InstanceBase* GetInstanceInternal() const;
	protected:
		AdapterBase(InstanceBase* instance);
		~AdapterBase();

		Ref<InstanceBase> mInstance;

		uint32_t mApiVersion;
		uint32_t mDriverVersion;
		uint32_t mVendorID;
		uint32_t mDeviceID;
		AdapterType mAdapterType;
		std::string mDeviceName;
		Limits mLimits;
	};
}