#include "vk_swap_chain.h"
#include "vk_render_device.h"
#include "vk_command_list.h"
#include "vk_resource.h"
#include "vk_errors.h"

#include <algorithm>
#include <memory>

namespace rhi
{
	SwapChainVk::SwapChainVk(RenderDeviceVk* renderDevice)
		:m_RenderDevice(renderDevice)
	{

	}

	SwapChainVk::~SwapChainVk()
	{
		ASSERT_MSG(m_RenderDevice, "RenderDevice must be destroyed after SwapChain");
		for (auto& semaphore : m_ImageAvailableSemaphores)
		{
			vkDestroySemaphore(m_RenderDevice->context.device, semaphore, nullptr);
		}

		for (auto& semaphore : m_RenderCompleteSemaphores)
		{
			vkDestroySemaphore(m_RenderDevice->context.device, semaphore, nullptr);
		}

		vkDestroySwapchainKHR(m_RenderDevice->context.device, m_SwapChain, nullptr);
		vkDestroySurfaceKHR(m_RenderDevice->context.instace, m_WindowSurface, nullptr);
		m_ColorAttachments.clear();
		m_DepthStencilAttachments = nullptr;
	}

	SwapChainVk* SwapChainVk::create(const SwapChainCreateInfo& swapChainCI)
	{
		assert(swapChainCI.renderDevice);
		SwapChainVk* swapChain = new SwapChainVk(static_cast<RenderDeviceVk*>(swapChainCI.renderDevice));
		swapChain->m_ColorFormat = swapChainCI.preferredColorFormat;
		swapChain->m_DepthStencilFormat = swapChainCI.preferredDepthStencilFormat;
		swapChain->m_Width = swapChainCI.initialWidth;
		swapChain->m_Height = swapChainCI.initialHeight;
		swapChain->m_VSyncEnabled = swapChainCI.enableVSync;

		swapChain->createSurface(swapChainCI.windowHandle);
		swapChain->createVkSwapChain();

		return swapChain;
	}

	void SwapChainVk::createSurface(void* platformWindow)
	{
		VkResult err = VK_SUCCESS;

		VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceCreateInfo.hinstance = (HINSTANCE)GetModuleHandle(nullptr);
		surfaceCreateInfo.hwnd = (HWND)platformWindow;
		err = vkCreateWin32SurfaceKHR(m_RenderDevice->context.instace, &surfaceCreateInfo, nullptr, &m_WindowSurface);

		CHECK_VK_RESULT(err, "Could not create surface!");

		VkBool32 isGraphicsSupportPresent;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_RenderDevice->context.physicalDevice, m_RenderDevice->queueFamilyIndex, m_WindowSurface, &isGraphicsSupportPresent);
		if (!isGraphicsSupportPresent)
		{
			LOG_ERROR("Could not support present!");
			return;
		}
	}

	void SwapChainVk::createVkSwapChain()
	{
		// Get list of supported surface formats
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_RenderDevice->context.physicalDevice, m_WindowSurface, &formatCount, NULL);
		assert(formatCount > 0);

		std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_RenderDevice->context.physicalDevice, m_WindowSurface, &formatCount, surfaceFormats.data());

		VkSurfaceFormatKHR selectedFormat = surfaceFormats[0];
		std::vector<VkFormat> preferredImageFormats = {
			rhi::formatToVkFormat(m_ColorFormat),
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_FORMAT_B8G8R8A8_UNORM,
			VK_FORMAT_R8G8B8A8_UNORM,
		};

		for (auto& availableFormat : surfaceFormats)
		{
			for (uint32_t i = 0; i < preferredImageFormats.size(); ++i)
			{
				if (preferredImageFormats[i] == availableFormat.format)
				{
					selectedFormat = availableFormat;
					break;
				}
			}
		}

		if (selectedFormat.format != rhi::formatToVkFormat(m_ColorFormat))
		{
			std::stringstream ss;
			ss << "Requested color format is not supported and will be replaced by " << getFormatInfo(vkFormatToFormat(selectedFormat.format)).name;
			logMsg(MessageSeverity::Warning, __FUNCTION__, __LINE__, ss.str().c_str());
			m_ColorFormat = vkFormatToFormat(selectedFormat.format);
		}

		// Store the current swap chain handle so we can use it later on to ease up recreation
		VkSwapchainKHR oldSwapchain = m_SwapChain;

		// Get physical device surface properties and formats
		VkSurfaceCapabilitiesKHR surfCaps;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_RenderDevice->context.physicalDevice, m_WindowSurface, &surfCaps);

		// Get available present modes
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_RenderDevice->context.physicalDevice, m_WindowSurface, &presentModeCount, NULL);
		assert(presentModeCount > 0);

		std::vector<VkPresentModeKHR> presentModes(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_RenderDevice->context.physicalDevice, m_WindowSurface, &presentModeCount, presentModes.data());

		VkExtent2D swapchainExtent = {};
		// If width (and height) equals the special value 0xFFFFFFFF, the size of the surface will be set by the swapchain
		if (surfCaps.currentExtent.width == (uint32_t)-1)
		{
			// If the surface size is undefined, the size is set to
			// the size of the images requested.
			swapchainExtent.width = std::clamp(m_Width, surfCaps.minImageExtent.width, surfCaps.maxImageExtent.width);
			swapchainExtent.height = std::clamp(m_Height, surfCaps.minImageExtent.height, surfCaps.maxImageExtent.height);
		}
		else
		{
			// If the surface size is defined, the swap chain size must match
			swapchainExtent = surfCaps.currentExtent;
		}

		// Select a present mode for the swapchain

		// The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
		// This mode waits for the vertical blank ("v-sync")
		VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

		// If v-sync is not requested, try to find a mailbox mode
		// It's the lowest latency non-tearing present mode available
		if (!m_VSyncEnabled)
		{
			for (size_t i = 0; i < presentModeCount; i++)
			{
				if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
					break;
				}
				if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
				{
					swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
				}
			}
		}

		// Determine the number of images
		uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
		if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount))
		{
			desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
		}

		// Find the transformation of the surface
		VkSurfaceTransformFlagsKHR preTransform;
		if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
		{
			// We prefer a non-rotated transform
			preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		}
		else
		{
			preTransform = surfCaps.currentTransform;
		}

		// Find a supported composite alpha format (not all devices support alpha opaque)
		VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		// Simply select the first composite alpha format available
		std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
		};
		for (auto& compositeAlphaFlag : compositeAlphaFlags) {
			if (surfCaps.supportedCompositeAlpha & compositeAlphaFlag) {
				compositeAlpha = compositeAlphaFlag;
				break;
			};
		}

		VkSwapchainCreateInfoKHR swapchainCI = {};
		swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCI.surface = m_WindowSurface;
		swapchainCI.minImageCount = desiredNumberOfSwapchainImages;
		swapchainCI.imageFormat = selectedFormat.format;
		swapchainCI.imageColorSpace = selectedFormat.colorSpace;
		swapchainCI.imageExtent = { swapchainExtent.width, swapchainExtent.height };
		swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		swapchainCI.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
		swapchainCI.imageArrayLayers = 1;
		swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCI.queueFamilyIndexCount = 0;
		swapchainCI.presentMode = swapchainPresentMode;
		// Setting oldSwapChain to the saved handle of the previous swapchain aids in resource reuse and makes sure that we can still present already acquired images
		swapchainCI.oldSwapchain = oldSwapchain;
		// Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
		swapchainCI.clipped = VK_TRUE;
		swapchainCI.compositeAlpha = compositeAlpha;

		// Enable transfer source on swap chain images if supported
		if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
			swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}

		// Enable transfer destination on swap chain images if supported
		if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
			swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}

		VkResult err = vkCreateSwapchainKHR(m_RenderDevice->context.device, &swapchainCI, nullptr, &m_SwapChain);

		CHECK_VK_RESULT(err, "Could not create swapchain!");

		// If an existing swap chain is re-created, destroy the old swap chain
		// This also cleans up all the presentable images
		if (oldSwapchain != VK_NULL_HANDLE)
		{
			m_ColorAttachments.clear();
			vkDestroySwapchainKHR(m_RenderDevice->context.device, oldSwapchain, nullptr);
			oldSwapchain = VK_NULL_HANDLE;
		}

		uint32_t imageCount;
		vkGetSwapchainImagesKHR(m_RenderDevice->context.device, m_SwapChain, &imageCount, NULL);
		std::vector<VkImage> images{ imageCount };
		err = vkGetSwapchainImagesKHR(m_RenderDevice->context.device, m_SwapChain, &imageCount, images.data());
		CHECK_VK_RESULT(err, "Failed to get swap chain images");

		m_ColorAttachments.resize(imageCount);

		TextureDesc colorTextureDesc;
		colorTextureDesc.dimension = TextureDimension::Texture2D;
		colorTextureDesc.width = swapchainExtent.width;
		colorTextureDesc.height = swapchainExtent.height;
		colorTextureDesc.format = m_ColorFormat;
		colorTextureDesc.usage = TextureUsage::RenderTarget;
		for (int i = 0; i < m_ColorAttachments.size(); ++i)
		{
			TextureVk* colorTex = m_RenderDevice->createTextureWithExistImage(colorTextureDesc, images[i]);
			m_ColorAttachments[i] = std::unique_ptr<ITexture>(colorTex);
		}

		if (m_DepthStencilFormat != Format::UNKNOWN)
		{
			TextureDesc depthStencilDesc;
			depthStencilDesc.width = swapchainExtent.width;
			depthStencilDesc.height = swapchainExtent.height;
			depthStencilDesc.format = m_DepthStencilFormat;
			depthStencilDesc.usage = TextureUsage::DepthStencil;
			depthStencilDesc.dimension = TextureDimension::Texture2D;
			TextureVk* depthStencilTex = static_cast<TextureVk*>(m_RenderDevice->createTexture(depthStencilDesc));
			m_DepthStencilAttachments = std::unique_ptr<ITexture>(depthStencilTex);
		}

		// The semaphores need to be rebuilt
		for (auto& semaphore : m_ImageAvailableSemaphores)
		{
			if (semaphore != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(m_RenderDevice->context.device, semaphore, nullptr);
			}
		}

		for (auto& semaphore : m_RenderCompleteSemaphores)
		{
			if (semaphore != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(m_RenderDevice->context.device, semaphore, nullptr);
			}
		}

		VkSemaphoreCreateInfo semaphoreCI{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		for (auto& semaphore : m_ImageAvailableSemaphores)
		{
			vkCreateSemaphore(m_RenderDevice->context.device, &semaphoreCI, nullptr, &semaphore);
		}
		
		for (auto& semaphore : m_RenderCompleteSemaphores)
		{
			vkCreateSemaphore(m_RenderDevice->context.device, &semaphoreCI, nullptr, &semaphore);
		}

		m_CompleteRenderingCmdList = std::unique_ptr<CommandListVk>(checked_cast<CommandListVk*>(m_RenderDevice->createCommandList()));
	}

	void SwapChainVk::recreateSwapChain()
	{
		assert(m_RenderDevice);
		m_RenderDevice->waitIdle();
		createVkSwapChain();
	}

	void SwapChainVk::beginFrame()
	{
		const VkSemaphore& semaphore = m_ImageAvailableSemaphores[m_CurrentFrameInFlight];

		VkResult err = vkAcquireNextImageKHR(m_RenderDevice->context.device,
			m_SwapChain, UINT64_MAX, semaphore, nullptr, &m_SwapChainImageIndex);
		if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
		{
			recreateSwapChain();
			err = vkAcquireNextImageKHR(m_RenderDevice->context.device,
				m_SwapChain, UINT64_MAX, semaphore, nullptr, &m_SwapChainImageIndex);
		}
		CHECK_VK_RESULT(err, "Failed to acquire next swap chain image");
		m_RenderDevice->setSwapChainImageAvailableSeamaphore(semaphore);
	}

	void SwapChainVk::present()
	{
		const VkSemaphore& semaphore = m_RenderCompleteSemaphores[m_CurrentFrameInFlight];
		m_RenderDevice->setRenderCompleteSemaphore(semaphore);
		// to ensure that all commandBuffers on this queue have been executed to completion and transition the colorattchment layout.
		m_CompleteRenderingCmdList->open();
		m_CompleteRenderingCmdList->transitionTextureState(*getCurrentRenderTargetTexture(), ResourceState::Present);
		m_CompleteRenderingCmdList->commitBarriers();
		m_CompleteRenderingCmdList->close();
		ICommandList* cmdLists[] = { m_CompleteRenderingCmdList.get() };
		m_RenderDevice->executeCommandLists(cmdLists, 1);

		VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &semaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_SwapChain;
		presentInfo.pImageIndices = &m_SwapChainImageIndex;

		VkResult err = vkQueuePresentKHR(m_RenderDevice->queue, &presentInfo);
		if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
		{
			recreateSwapChain();
		}
		else
		{
			CHECK_VK_RESULT(err, "Failed to present swap chain image");
		}

		m_CurrentFrameInFlight = (m_CurrentFrameInFlight + 1) % m_ColorAttachments.size();

		while (m_LastSubmittedIDPerFrame.size() >= g_MaxConcurrentFrames)
		{
			uint64_t submitID = m_LastSubmittedIDPerFrame.front();
			m_LastSubmittedIDPerFrame.pop();
			m_RenderDevice->waitForExecution(submitID, UINT64_MAX);
		}

		m_LastSubmittedIDPerFrame.push(m_RenderDevice->lastSubmittedID);

		m_RenderDevice->RecycleCommandBuffers();
	}

	ITexture* SwapChainVk::getCurrentRenderTargetTexture()
	{
		return m_ColorAttachments[m_SwapChainImageIndex].get();
	}

	ITexture* SwapChainVk::getDepthStencilTexture()
	{
		return m_DepthStencilAttachments.get();
	}

	void SwapChainVk::resize()
	{
		recreateSwapChain();
	}
}