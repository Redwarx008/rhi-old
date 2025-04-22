#pragma once

#include "rhi/RHIStruct.h"
#include "ResourceBase.h"
#include "common/Ref.hpp"
#include <vector>

namespace rhi
{
	class DeviceBase;
	class BindSetLayoutBase;

	class BindSetBase : public ResourceBase
	{
	public:
		ResourceType GetType() const override;
		BindSetLayoutBase* GetLayout();
		const std::vector<BindSetEntry>& GetBindingEntries() const;
	protected:
		explicit BindSetBase(DeviceBase* device, const BindSetDesc& desc) noexcept;
		~BindSetBase();

	private:
		Ref<BindSetLayoutBase> mLayout;
		std::vector<BindSetEntry> mEntries;
	};
}