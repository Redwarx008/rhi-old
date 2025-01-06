#pragma once

#include "rhi/rhi.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>


namespace rhi::vulkan
{
	class Device;
	class CommandList;
	struct ContextVk;

	class CommandQueue
	{
	public:
		CommandQueue(Device* rd, const ContextVk& context);
		~CommandQueue();
		void addWaitSemaphore(VkSemaphore semaphore, uint64_t value = 0);
		void addSingalSemaphore(VkSemaphore semaphore, uint64_t value = 0);
		CommandList* getValidCommandList();
		uint64_t submit(CommandList* commandList); // submit this commandList and it's previous commandList.
		QueueType type = QueueType::Graphics;
		VkSemaphore trackingSubmittedSemaphore = VK_NULL_HANDLE;
		VkSemaphore swapChainImgAvailableSemaphore = VK_NULL_HANDLE;
		VkSemaphore renderCompleteSemaphore = VK_NULL_HANDLE;
		VkQueue queue = VK_NULL_HANDLE;
		uint64_t lastSubmitID = 0;
		uint32_t queueFamilyIndex = UINT32_MAX;
	private:
		Device* m_RenderDevice;
		const ContextVk& m_Context;
		std::vector<CommandList*> m_ActiveCommandLists;
		std::vector<CommandList*> m_CommandListPool;

		std::vector<VkCommandBufferSubmitInfo> m_VkCmdBufSubmitInfos;

		std::vector<VkSemaphore> m_WaitSemaphoresForSubmit;
		std::vector<uint64_t> m_WaitSemaphoreValuesForSubmit;
		std::vector<VkSemaphore> m_SingalSemaphoreForSubmit;
		std::vector<uint64_t> m_SingalSemaphoreValuesForSubmit;


	};
}

