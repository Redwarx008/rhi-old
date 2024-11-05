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

	class RenderDeviceVk;
	class CommandListVk;
	class SwapChainVk final : public ISwapChain
	{
	public:
		~SwapChainVk();
		static SwapChainVk* create(const SwapChainCreateInfo& swapChainCI);
		void beginFrame() override;
		void present() override;
		void resize() override;
		ITexture* getCurrentRenderTargetTexture() override;
		ITexture* getDepthStencilTexture() override;
		Format getRenderTargetFormat() override { return m_ColorFormat; }
		Format getDepthStencilFormat() override { return m_DepthStencilFormat; }
	private:
		SwapChainVk(RenderDeviceVk* renderDevice);
		void createSurface(void* platformWindow);
		void createVkSwapChain();
		void recreateSwapChain();
		uint32_t m_Width = 0;
		uint32_t m_Height = 0;

		Format m_ColorFormat {Format::UNKNOWN};
		Format m_DepthStencilFormat{ Format::UNKNOWN };

		bool m_VSyncEnabled = false;

		RenderDeviceVk* m_RenderDevice;
		VkSurfaceKHR m_WindowSurface = VK_NULL_HANDLE;
		VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;

		uint32_t m_CurrentFrameInFlight = 0;
		uint32_t m_SwapChainImageIndex = UINT32_MAX;

		std::queue<uint64_t> m_LastSubmittedIDPerFrame;

		std::array<VkSemaphore, g_MaxConcurrentFrames + 1> m_ImageAvailableSemaphores{};
		std::array<VkSemaphore, g_MaxConcurrentFrames + 1> m_RenderCompleteSemaphores{};

		std::unique_ptr<CommandListVk> m_CompleteRenderingCmdList;

		std::vector<std::unique_ptr<ITexture>> m_ColorAttachments;
		std::unique_ptr<ITexture> m_DepthStencilAttachments;
	};
}