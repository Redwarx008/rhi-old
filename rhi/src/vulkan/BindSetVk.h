#pragma once

#include "rhi/RHIStruct.h"
#include "../BindSetBase.h"
#include "../common/Ref.hpp"
#include "DescriptorSetAllocation.h"
#include <array>

namespace rhi::vulkan
{
	class BindSet final : public BindSetBase
	{
	public:
		static Ref<BindSet> Create(Device* device, const BindSetDesc& desc);
		explicit BindSet(Device* device, const BindSetDesc& desc, DescriptorSetAllocation descriptorSetAllocation) noexcept;

		VkDescriptorSet GetHandle() const;
		void MarkUsedInQueue(QueueType queueType);
		bool IsUsedInQueue(QueueType queueType);
	private:
		~BindSet();
		void DestroyImpl() override;

		DescriptorSetAllocation mDescriptorSetAllocation;
		std::array<bool, 2> mUsedInQueues;
	};
}