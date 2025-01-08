#include "CommandListVk.h"

#include "rhi/common/Error.h"
#include "ErrorsVk.h"
#include "CommandListVk.h"
#include "DeviceVk.h"

namespace rhi::vulkan
{
	CommandQueue::CommandQueue(Device* rd, const ContextVk& context)
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


	CommandList* CommandQueue::getValidCommandList()
	{
		CommandList* cmdList;
		if (m_CommandListPool.empty())
		{
			cmdList = new CommandList(m_RenderDevice);
			VkCommandPoolCreateInfo commandPoolCI{};
			commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			commandPoolCI.queueFamilyIndex = queueFamilyIndex;
			commandPoolCI.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

			VkResult err = vkCreateCommandPool(m_Context.device, &commandPoolCI, nullptr, &cmdList->mCommandPool);
			CHECK_VK_RESULT(err, "Could not create vkCommandPool");

			VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
			commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			commandBufferAllocateInfo.mCommandPool = cmdList->mCommandPool;
			commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			commandBufferAllocateInfo.commandBufferCount = 1;

			err = vkAllocateCommandBuffers(m_Context.device, &commandBufferAllocateInfo, &cmdList->mCommandBuffer);
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

	uint64_t CommandQueue::submit(CommandList* commandList)
	{
		assert(commandList->queueType == type);

		++lastSubmitID;
		m_VkCmdBufSubmitInfos.resize(commandList->allocateIndex + 1);
		for (uint32_t i = 0; i < m_VkCmdBufSubmitInfos.size(); ++i)
		{
			m_ActiveCommandLists[i]->submitID = lastSubmitID;

			m_VkCmdBufSubmitInfos[i].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
			m_VkCmdBufSubmitInfos[i].mCommandBuffer = m_ActiveCommandLists[i]->mCommandBuffer;
		}

		VkSubmitInfo2 submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO_2 };

	}

	uint64_t GetPendingCommandSerialID() const
	{
		return mLastSubmittedSerial.load(std::memory_order_acquire) + 1);
	}
}