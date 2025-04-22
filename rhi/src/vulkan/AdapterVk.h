#pragma once

#include "../AdapterBase.h"
#include <vulkan/vulkan.h>

namespace rhi::vulkan
{

	class Adapter final : public AdapterBase
	{
	public:
		explicit Adapter(InstanceBase* instance, VkPhysicalDevice vulkanPhysicalDevice);
		~Adapter() = default;
		// api
		DeviceBase* CreateDevice(const DeviceDesc& desc) override;
		// internal
		VkPhysicalDevice GetHandle() const;
	private:

		VkPhysicalDevice mVulkanPhysicalDevice = VK_NULL_HANDLE;
	};
}