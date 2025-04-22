#include "rhi/RHI.h"

#include "vulkan/DeviceVk.h"
#include "vulkan/SwapChainVk.h"

namespace rhi
{
	LoggingCallback gDebugMessageCallback;

	IDevice* createDevice(const DeviceDesc& createInfo)
	{
		return rhi::vulkan::Device::create(createInfo);
	}

	IInstance* CreateInstance(const InstanceDesc& desc)
	{

	}

	ISwapChain* CreateSwapChain(const SwapChainDesc& createInfo)
	{ 
		return rhi::vulkan::SwapChain::create(createInfo);
	}
}