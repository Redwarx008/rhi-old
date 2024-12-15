#pragma once

#include "rhi/rhi.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

namespace rhi
{
	class RenderDeviceVk;
	class CommandListVk;
	struct ContextVk;

	class CommandQueue
	{
	public:
		CommandQueue(RenderDeviceVk* rd, const ContextVk& context);
		~CommandQueue();
		void addWaitSemaphore(VkSemaphore semaphore, uint64_t value = 0);
		void addSingalSemaphore(VkSemaphore semaphore, uint64_t value = 0);
		CommandListVk* getValidCommandList();
		uint64_t submit(CommandListVk* commandList); // submit this commandList and it's previous commandList.
		QueueType type = QueueType::Graphics;
		VkSemaphore trackingSubmittedSemaphore = VK_NULL_HANDLE;
		VkSemaphore swapChainImgAvailableSemaphore = VK_NULL_HANDLE;
		VkSemaphore renderCompleteSemaphore = VK_NULL_HANDLE;
		VkQueue queue = VK_NULL_HANDLE;
		uint64_t lastSubmitID = 0;
		uint32_t queueFamilyIndex = UINT32_MAX;
	private:
		RenderDeviceVk* m_RenderDevice;
		const ContextVk& m_Context;
		std::vector<CommandListVk*> m_ActiveCommandLists;
		std::vector<CommandListVk*> m_CommandListPool;

		std::vector<VkCommandBufferSubmitInfo> m_VkCmdBufSubmitInfos;

		std::vector<VkSemaphore> m_WaitSemaphoresForSubmit;
		std::vector<uint64_t> m_WaitSemaphoreValuesForSubmit;
		std::vector<VkSemaphore> m_SingalSemaphoreForSubmit;
		std::vector<uint64_t> m_SingalSemaphoreValuesForSubmit;
	};
}

