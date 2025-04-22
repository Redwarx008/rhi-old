#pragma once

#include "rhi/RHIStruct.h"
#include "ResourceBase.h"
#include "common/Constants.h"
#include "common/Ref.hpp"

#include <bitset>
#include <array>
namespace rhi
{
	class DeviceBase;
	class BindSetLayoutBase;

	class PipelineLayoutBase : public ResourceBase
	{
	public:
		BindSetLayoutBase* GetBindSetLayout(uint32_t bindSetIndex) const;
		const std::bitset<cMaxBindSets>& GetBindSetMask() const;
		ResourceType GetType() const override;
		const PushConstantRange& GetPushConstants() const;
	protected:
		explicit PipelineLayoutBase(DeviceBase* device, const PipelineLayoutDesc& desc) noexcept;
		~PipelineLayoutBase();
		std::bitset<cMaxBindSets> mBindSetMask = {};
		std::array<Ref<BindSetLayoutBase>, cMaxBindSets> mBindGroupLayouts = {};
		PushConstantRange mPushConstantRange;
	};
}