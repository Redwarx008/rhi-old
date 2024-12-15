#include "vk_command_queue.h"

#include "rhi/common/Error.h"
#include "vk_errors.h"
#include "vk_command_list.h"
#include "vk_render_device.h"

namespace rhi
{
	CommandQueue::CommandQueue(RenderDeviceVk* rd, const ContextVk& context)
		:m_RenderDevice(rd),
		m_Context(context)
	{
		// Setup the timeline semaphore
		VkSemaphoreTypeCreateInfo semaphoreTypeCI{};
		semaphoreTypeCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO_KHR;
		semaphoreTypeCI.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE_KHR;

		VkSemaphoreCreateInfo semaphoreCI{};
		semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreCI.pNext = &semaphoreTypeCI;

		VkResult err = vkCreateSemaphore(m_Context.device, &semaphoreCI, nullptr, &trackingSubmittedSemaphore);
		CHECK_VK_RESULT(err);
	}

	CommandQueue::~CommandQueue()
	{
		vkDestroySemaphore(m_Context.device, trackingSubmittedSemaphore, nullptr);
	}


	CommandListVk* CommandQueue::getValidCommandList()
	{
		CommandListVk* cmdList;
		if (m_CommandListPool.empty())
		{
			cmdList = new CommandListVk(m_RenderDevice);
			VkCommandPoolCreateInfo commandPoolCI{};
			commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			commandPoolCI.queueFamilyIndex = queueFamilyIndex;
			commandPoolCI.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

			VkResult err = vkCreateCommandPool(m_Context.device, &commandPoolCI, nullptr, &cmdList->commandPool);
			CHECK_VK_RESULT(err, "Could not create vkCommandPool");

			VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
			commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			commandBufferAllocateInfo.commandPool = cmdList->commandPool;
			commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			commandBufferAllocateInfo.commandBufferCount = 1;

			err = vkAllocateCommandBuffers(m_Context.device, &commandBufferAllocateInfo, &cmdList->commandBuffer);
			CHECK_VK_RESULT(err, "Could not create vkCommandBuffer");
			cmdList->queueType = type;
		}
		else
		{
			cmdList = m_CommandListPool.back();
			m_CommandListPool.pop_back();
		}
		m_ActiveCommandLists.push_back(cmdList);
		cmdList->allocateIndex = m_ActiveCommandLists.size() - 1;
		return cmdList;
	}

	void CommandQueue::addWaitSemaphore(VkSemaphore semaphore, uint64_t value)
	{
		m_WaitSemaphoresForSubmit.push_back(semaphore);
		m_WaitSemaphoreValuesForSubmit.push_back(value);
	}

	void CommandQueue::addSingalSemaphore(VkSemaphore semaphore, uint64_t value)
	{
		m_SingalSemaphoreForSubmit.push_back(semaphore);
		m_SingalSemaphoreValuesForSubmit.push_back(value);
	}

	uint64_t CommandQueue::submit(CommandListVk* commandList)
	{
		assert(commandList->queueType == type);

		++lastSubmitID;
		m_VkCmdBufSubmitInfos.resize(commandList->allocateIndex + 1);
		for (uint32_t i = 0; i < m_VkCmdBufSubmitInfos.size(); ++i)
		{
			m_ActiveCommandLists[i]->submitID = lastSubmitID;

			m_VkCmdBufSubmitInfos[i].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
			m_VkCmdBufSubmitInfos[i].commandBuffer = m_ActiveCommandLists[i]->commandBuffer;
		}

		VkSubmitInfo2 submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO_2 };

	}
}