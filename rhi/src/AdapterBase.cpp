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

	InstanceBase* AdapterBase::APIGetInstance() const
	{
		Ref<InstanceBase> instance = mInstance;
		return instance.Detach();
	}

	Ref<InstanceBase> AdapterBase::GetInstance() const
	{
		return mInstance;
	}
}