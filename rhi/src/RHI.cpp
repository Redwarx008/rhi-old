#include "rhi/rhi.h"

#include "vulkan/DeviceVk.h"
#include "vulkan/SwapChainVk.h"

namespace rhi
{
	DebugMessageCallbackFunc g_DebugMessageCallback;

	IDevice* createDevice(const DeviceCreateInfo& createInfo)
	{
		return rhi::vulkan::Device::create(createInfo);
	}

	ISwapChain* createSwapChain(const SwapChainCreateInfo& createInfo)
	{ 
		return rhi::vulkan::SwapChain::create(createInfo);
	}
}