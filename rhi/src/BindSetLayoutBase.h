#pragma once

#include "ResourceBase.h"
#include "rhi/RHIStruct.h"
#include <vector>

namespace rhi
{
	class DeviceBase;
	class BindSetLayoutBase : public ResourceBase
	{
	public:
		ResourceType GetType() const override;
		BindingType GetBindingType(uint32_t binding) const;
		ShaderStage GetVisibility(uint32_t binding) const;
	protected:
		explicit BindSetLayoutBase(DeviceBase* device, const BindSetLayoutDesc& desc) noexcept;
		~BindSetLayoutBase();
		void Initialize(const BindSetLayoutDesc& desc);

		struct BindingInfo
		{
			BindingType type;
			ShaderStage visibility;
		};
		std::vector<BindSetLayoutEntry> mEntries;
		std::vector<BindingInfo> mBindingIndexToInfoMap;
	};
}