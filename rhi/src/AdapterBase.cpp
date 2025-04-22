#include "AdapterBase.h"
#include "InstanceBase.h"
#include <cassert>

namespace rhi
{
	AdapterBase::AdapterBase(InstanceBase* instance) :
		mInstance(instance)
	{ }

	AdapterBase::~AdapterBase() {}

	AdapterInfo AdapterBase::GetInfo() const
	{
		AdapterInfo info{};
		info.apiVersion = mApiVersion;
		info.driverVersion = mDriverVersion;
		info.deviceID = mDeviceID;
		info.vendorID = mVendorID;
		info.adapterType = mAdapterType;
		info.deviceName = mDeviceName;
		return info;
	}

	Limits AdapterBase::GetLimits() const
	{
		return mLimits;
	}

	InstanceBase* AdapterBase::GetInstance() const
	{
		InstanceBase* instance = GetInstanceInternal();
		instance->AddRef();
		return instance;
	}

	InstanceBase* AdapterBase::GetInstanceInternal() const
	{
		InstanceBase* instance = mInstance.Get();
		assert(!instance);
		return instance;
	}
}