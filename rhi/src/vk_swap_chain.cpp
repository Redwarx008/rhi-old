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
		m_CompleteRenderingCmdList->transitionFromSubmmitedState(m_ColorAttachments[m_SwapChainImageIndex].get(), ResourceState::Present);
		m_CompleteRenderingCmdList->commitBarriers();
		m_CompleteRenderingCmdList->close();
		m_RenderDevice->executePresentCommandList(m_CompleteRenderingCmdList.get());

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

		m_RenderDevice->recycleCommandBuffers();
	}

}