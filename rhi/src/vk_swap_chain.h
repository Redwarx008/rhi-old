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


		ITextureView* getCurrentRenderTargetView() override;
		ITextureView* getDepthStencilView() override;
		Format getRenderTargetFormat() override { return m_SwapChainFormat; }
		Format getDepthStencilFormat() override { return m_DepthStencilFormat; }

	private:
		uint32_t m_SwapChainImageWidth = 0;
		uint32_t m_SwapChainImageHeight = 0;

		Format m_SwapChainFormat = Format::UNKNOWN;
		Format m_DepthStencilFormat = Format::UNKNOWN;

		bool m_VSyncEnabled = false;

		VkSurfaceKHR m_WindowSurface = VK_NULL_HANDLE;
		VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;

		uint32_t m_CurrentFrameInFlight = 0;
		uint32_t m_SwapChainImageIndex = UINT32_MAX;

		std::queue<uint64_t> m_LastSubmittedIDPerFrame;

		std::array<VkSemaphore, g_MaxConcurrentFrames + 1> m_ImageAvailableSemaphores{};
		std::array<VkSemaphore, g_MaxConcurrentFrames + 1> m_RenderCompleteSemaphores{};

		std::vector<std::unique_ptr<TextureVk>> m_ColorAttachments;
		std::unique_ptr<TextureVk> m_DepthStencilAttachments;
	};
}