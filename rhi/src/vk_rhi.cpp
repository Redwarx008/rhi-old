#include "rhi/rhi.h"

#include "vk_render_device.h"
#include "vk_swap_chain.h"

namespace rhi
{
	DebugMessageCallbackFunc g_DebugMessageCallback;

	IRenderDevice* createRenderDevice(const RenderDeviceCreateInfo& createInfo)
	{
		return RenderDeviceVk::create(createInfo);
	}

	ISwapChain* createSwapChain(const SwapChainCreateInfo& createInfo)
	{ 
		return SwapChainVk::create(createInfo);
	}
}