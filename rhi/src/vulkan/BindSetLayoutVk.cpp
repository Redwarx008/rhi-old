#include "BindSetLayoutVk.h"

#include "../common/Utils.h"
#include "ErrorsVk.h"
#include "DeviceVk.h"
#include "BindSetVk.h"
#include "VulkanUtils.h"

#include <unordered_map>

namespace rhi::vulkan
{
	VkDescriptorType DescriptorTypeConvert(const BindSetLayoutEntry& entry)
	{
		BindingType bindType = entry.type;
		switch (bindType)
		{
		case rhi::BindingType::SampledTexture:
			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case rhi::BindingType::StorageTexture:
			return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		case rhi::BindingType::UniformBuffer:
		{
			if (entry.hasDynamicOffset)
			{
				return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			}
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		}
		case rhi::BindingType::StorageBuffer:
		{
			if (entry.hasDynamicOffset)
			{
				return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
			}
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		}	
		case rhi::BindingType::Sampler:
			return VK_DESCRIPTOR_TYPE_SAMPLER;
		case rhi::BindingType::CombinedTextureSampler:
			return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		case rhi::BindingType::None:
		default:
			assert(!"unreachable");
			break;
		}
	}

	BindSetLayout::BindSetLayout(DeviceBase* device, const BindSetLayoutDesc& desc):
		BindSetLayoutBase(device, desc)
	{

	}

	Ref<BindSetLayout> BindSetLayout::Create(DeviceBase* device, const BindSetLayoutDesc& desc)
	{
		Ref<BindSetLayout> bindSetLayout = AcquireRef(new BindSetLayout(device, desc));
		if (!bindSetLayout->Initialize(desc))
		{
			return nullptr;
		}
		return std::move(bindSetLayout);
	}

	BindSetLayout::~BindSetLayout() {}

	bool BindSetLayout::Initialize(const BindSetLayoutDesc& desc)
	{
		BindSetLayoutBase::Initialize(desc);

		std::vector<VkDescriptorSetLayoutBinding> vkBindings;
		vkBindings.reserve(mEntries.size());

		for (const BindSetLayoutEntry& entry : mEntries)
		{

			VkDescriptorSetLayoutBinding& vkBinding = vkBindings.emplace_back();
			vkBinding.binding = entry.binding;
			vkBinding.descriptorType = DescriptorTypeConvert(entry);
			vkBinding.descriptorCount = entry.arrayElementCount;
			vkBinding.stageFlags = ShaderStageFlagsConvert(entry.visibleStages);
			vkBinding.pImmutableSamplers = nullptr;
		}

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.bindingCount = vkBindings.size();
		createInfo.pBindings = vkBindings.data();

		Device* device = checked_cast<Device>(mDevice.Get());

		VkResult err = vkCreateDescriptorSetLayout(device->GetHandle(), &createInfo, nullptr, &mHandle);
		CHECK_VK_RESULT_FALSE(err, "CreateDescriptorSetLayout");

		// Compute the size of descriptor pools used for this layout.

		std::unordered_map<VkDescriptorType, uint32_t> descriptorCountPerType;

		for (const BindSetLayoutEntry& entry : mEntries)
		{
			VkDescriptorType vkType = DescriptorTypeConvert(entry);
			descriptorCountPerType[vkType] += 1;
		}

		mDescriptorSetAllocator = DescriptorSetAllocator::Create(device, std::move(descriptorCountPerType));

		SetDebugName(device, mHandle, "BindSetLayout", GetName());

		return true;
	}

	void BindSetLayout::DestroyImpl()
	{
		Device* device = checked_cast<Device>(mDevice.Get());

		if (mHandle != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(device->GetHandle(), mHandle, nullptr);
			mHandle = VK_NULL_HANDLE;
		}

		mDescriptorSetAllocator = nullptr;
	}

	VkDescriptorSetLayout BindSetLayout::GetHandle() const
	{
		return mHandle;
	}

	Ref<BindSet> BindSetLayout::AllocateBindSet(const BindSetDesc& desc)
	{
		DescriptorSetAllocation descriptorSetAllocation = mDescriptorSetAllocator->Allocate(this);

		return AcquireRef(new BindSet(checked_cast<Device*>(mDevice.Get()), desc, descriptorSetAllocation));
	}

	void BindSetLayout::DeallocateBindSet(BindSet* bindSet,
		DescriptorSetAllocation* descriptorSetAllocation)
	{
		mDescriptorSetAllocator->Deallocate(descriptorSetAllocation, bindSet->IsUsedInQueue(QueueType::Graphics), bindSet->IsUsedInQueue(QueueType::Compute));
	}
}