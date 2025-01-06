#pragma once

#include "rhi/rhi.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <queue>
#include <memory>

#include "../Ref.hpp"

namespace rhi::vulkan
{
	static constexpr uint32_t g_MaxConcurrentFrames = 2;

	//template <typename T>
	//class Ref;
	class TextureVk;
	class Device;
	class CommandList;
	class SwapChain final : public ISwapChain
	{
	public:
		static Ref<SwapChain> Create(const SwapChainCreateInfo& swapChainCI);
		~SwapChain();
		SwapChain(Device* device);
		void recreateSwapChain() override;
		//ITextureView* getCurrentRenderTargetView() override;
		//ITextureView* getDepthStencilView() override;
		//Format getRenderTargetFormat() override { return m_SwapChainFormat; }
		//Format getDepthStencilFormat() override { return m_DepthStencilFormat; }

	private:
		void createSurface(void* platformWindow);
		void createSwapChainInternal();
		void destroySwapChain();

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

		Ref<Device> mDevice;
	};
}