#include "BindSetLayoutBase.h"

#include "common/Error.h"
#include "common/Constants.h"
#include "DeviceBase.h"
#include <algorithm>

namespace rhi
{

	BindSetLayoutBase::BindSetLayoutBase(DeviceBase* device, const BindSetLayoutDesc& desc) :
		ResourceBase(device, desc.name)
	{
	}

	BindSetLayoutBase::~BindSetLayoutBase() {}

	void BindSetLayoutBase::Initialize(const BindSetLayoutDesc& desc)
	{
		ResourceBase::Initialize();

		mEntries.reserve(desc.entryCount);
		for (uint32_t i = 0; i < desc.entryCount; ++i)
		{
			mEntries.push_back(desc.entries[i]);
		}

		std::sort(mEntries.begin(), mEntries.end(), [](const BindSetLayoutEntry& a, const BindSetLayoutEntry& b)
			{
				return a.binding < b.binding;
			});

		ASSERT(mEntries.cend()->binding < cMaxBindingsPerBindSet);
		mBindingIndexToInfoMap.reserve(mEntries.cend()->binding);

		for (const auto& entry : mEntries)
		{
			mBindingIndexToInfoMap[entry.binding].type = entry.type;
			mBindingIndexToInfoMap[entry.binding].visibility = entry.visibleStages;
		}
	}

	ResourceType BindSetLayoutBase::GetType() const
	{
		return ResourceType::BindSetLayout;
	}

	BindingType BindSetLayoutBase::GetBindingType(uint32_t binding) const
	{
		ASSERT(binding < mBindingIndexToInfoMap.size());
		return mBindingIndexToInfoMap[binding].type;
	}

	ShaderStage BindSetLayoutBase::GetVisibility(uint32_t binding) const
	{
		ASSERT(binding < mBindingIndexToInfoMap.size());
		return mBindingIndexToInfoMap[binding].visibility;
	}
}