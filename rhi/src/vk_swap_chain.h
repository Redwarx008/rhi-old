#pragma once

#include "rhi/rhi.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <queue>
#include <memory>

namespace rhi
{
	static constexpr uint32_t g_MaxConcurrentFrames = 2;

	class TextureVk;
	class RenderDeviceVk;
	class CommandListVk;
	class SwapChainVk
	{
	public:
		~SwapChainVk();
		SwapChainVk(RenderDeviceVk* renderDevice);


	};
}