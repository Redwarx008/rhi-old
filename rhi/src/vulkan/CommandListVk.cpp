#include "vk_command_list.h"

#include "vk_pipeline.h"
#include "vk_render_device.h"
#include "vk_resource.h"
#include "vk_errors.h"
#include "rhi/common/Error.h"

#include <array>
#include <optional>

namespace rhi
{
	CommandList::CommandList(Device* renderDevice, const ContextVk& context)
		:m_RenderDevice(renderDevice),
		m_Context(context),
		m_UploadAllocator(renderDevice)
	{
		// to do: delete it, if vulkan 1.4 is released.
		vkCmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(m_Context.device, "vkCmdPushDescriptorSetKHR");

		if (renderDevice->createInfo.enableDebugRuntime)
		{
			this->vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(m_Context.device, "vkCmdBeginDebugUtilsLabelEXT");
			this->vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(m_Context.device, "vkCmdEndDebugUtilsLabelEXT");
		}
	}

	CommandList::~CommandList()
	{

	}

	void CommandList::close()
	{
		endRendering();
		commitBarriers();
		vkEndCommandBuffer(commandBuffer);
	}

	void CommandList::waitCommandList(ICommandList* other)
	{
		auto otherCmdList = checked_cast<CommandList*>(other);
		if (queueType == otherCmdList->queueType)
		{
			return; // The order of commandlists on the same queue is implicitly guaranteed.
		}
		m_WaitCommandLists.push_back(other);
	}

	void CommandList::endRendering()
	{
		if (m_RenderingStarted)
		{
			vkCmdEndRendering(m_CurrentCmdBuf->vkCmdBuf);
			m_RenderingStarted = false;
		}
	}

	void CommandList::setResourceAutoTransition(bool enable)
	{
		m_EnableAutoTransition = enable;
	}

	inline static bool resourceStateHasWriteAccess(ResourceState state)
	{
		const ResourceState writeAccessStates =
			ResourceState::RenderTarget |
			ResourceState::DepthWrite |
			ResourceState::UnorderedAccess |
			ResourceState::CopyDest |
			ResourceState::ResolveDest;
		return (state & writeAccessStates) == state;
	}

	void CommandList::transitionFromSubmmitedState(ITexture* texture, ResourceState newState)
	{
		assert(texture);
		auto textureVk = checked_cast<TextureVk*>(texture);

		ResourceState oldState = textureVk->submittedState;

		// Always add barrier after writes.
		bool isAfterWrites = resourceStateHasWriteAccess(oldState);
		bool transitionNecessary = oldState != newState || isAfterWrites;

		if (transitionNecessary)
		{
			TextureBarrier& barrier = m_TextureBarriers.emplace_back();
			barrier.texture = textureVk;
			barrier.stateBefore = oldState;
			barrier.stateAfter = newState;
		}

		textureVk->setState(newState);
	}

	void CommandList::updateSubmittedState()
	{
		for (auto texture : m_TrackingSubmittedStates)
		{
			texture->submittedState = texture->getState();
		}
		m_TrackingSubmittedStates.clear();
	}

	void CommandList::transitionTextureState(ITexture* texture, ResourceState newState)
	{
		assert(texture);
		auto textureVk = checked_cast<TextureVk*>(texture);

		ResourceState oldState = textureVk->getState();

		// Always add barrier after writes.
		bool isAfterWrites = resourceStateHasWriteAccess(oldState);
		bool transitionNecessary = oldState != newState || isAfterWrites;

		if (transitionNecessary)
		{
			TextureBarrier& barrier = m_TextureBarriers.emplace_back();
			barrier.texture = textureVk;
			barrier.stateBefore = oldState;
			barrier.stateAfter = newState;
		}

		m_TrackingSubmittedStates.push_back(textureVk);
		textureVk->setState(newState);
	}

	void CommandList::transitionBufferState(IBuffer* buffer, ResourceState newState)
	{
		assert(buffer);
		auto bufferVk = checked_cast<BufferVk*>(buffer);

		ResourceState oldState = bufferVk->getState();

		//if (bufferVk.getDesc().access != BufferAccess::GpuOnly)
		//{
		//	// host visible buffers can't change state
		//	return;
		//}

		// Always add barrier after writes.
		bool isAfterWrites = resourceStateHasWriteAccess(oldState);
		bool transitionNecessary = oldState != newState || isAfterWrites;

		if (transitionNecessary)
		{
			// See if this buffer is already used for a different purpose in this batch.
			// If it is, combine the state bits.
			// Example: same buffer used as index and vertex buffer, or as SRV and indirect arguments.
			for (BufferBarrier& barrier : m_BufferBarriers)
			{
				if (barrier.buffer == bufferVk)
				{
					barrier.stateAfter = ResourceState(barrier.stateAfter | newState);
					bufferVk->setState(barrier.stateAfter);
					return;
				}
			}

			BufferBarrier& barrier = m_BufferBarriers.emplace_back();
			barrier.buffer = bufferVk;
			barrier.stateBefore = oldState;
			barrier.stateAfter = newState;
		}

		bufferVk->setState(newState);
	}

	void CommandList::commitBarriers()
	{
		if (m_BufferBarriers.empty() && m_TextureBarriers.empty())
		{
			return;
		}
		endRendering();

		m_VkImageMemoryBarriers.resize(m_TextureBarriers.size());
		m_VkBufferMemoryBarriers.resize(m_BufferBarriers.size());

		for (int i = 0; i < m_TextureBarriers.size(); ++i)
		{
			const TextureBarrier& barrier = m_TextureBarriers[i];

			VkImageLayout oldLayout = resourceStateToVkImageLayout(barrier.stateBefore);
			VkImageLayout newLayout = resourceStateToVkImageLayout(barrier.stateAfter);
			assert(newLayout != VK_IMAGE_LAYOUT_UNDEFINED);

			VkPipelineStageFlags2 srcStage = resourceStatesToVkPipelineStageFlags2(barrier.stateBefore);
			VkPipelineStageFlags2 dstStage = resourceStatesToVkPipelineStageFlags2(barrier.stateAfter);

			VkAccessFlags2 srcAccessMask = resourceStatesToVkAccessFlags2(barrier.stateBefore);
			VkAccessFlags2 dstAccessMask = resourceStatesToVkAccessFlags2(barrier.stateAfter);

			VkImageSubresourceRange subresourceRange{};
			subresourceRange.aspectMask = getVkAspectMask(barrier.texture->format);
			subresourceRange.baseArrayLayer = 0;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.layerCount = barrier.texture->getDesc().arraySize;
			subresourceRange.levelCount = barrier.texture->getDesc().mipLevels;

			m_VkImageMemoryBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			m_VkImageMemoryBarriers[i].pNext = nullptr;
			m_VkImageMemoryBarriers[i].srcStageMask = srcStage;
			m_VkImageMemoryBarriers[i].srcAccessMask = srcAccessMask;
			m_VkImageMemoryBarriers[i].dstStageMask = dstStage;
			m_VkImageMemoryBarriers[i].dstAccessMask = dstAccessMask;
			m_VkImageMemoryBarriers[i].oldLayout = oldLayout;
			m_VkImageMemoryBarriers[i].newLayout = newLayout;
			m_VkImageMemoryBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			m_VkImageMemoryBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			m_VkImageMemoryBarriers[i].image = barrier.texture->image;
			m_VkImageMemoryBarriers[i].subresourceRange = subresourceRange;
		}

		for (int i = 0; i < m_BufferBarriers.size(); ++i)
		{
			const BufferBarrier& barrier = m_BufferBarriers[i];
			// for buffer, if srcStage is VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, no effect
			//if (barrier.stateBefore == ResourceState::Undefined)
			//{
			//	continue;
			//}

			VkPipelineStageFlags2 srcStage = resourceStatesToVkPipelineStageFlags2(barrier.stateBefore);
			VkPipelineStageFlags2 dstStage = resourceStatesToVkPipelineStageFlags2(barrier.stateAfter);

			VkAccessFlags2 srcAccessMask = resourceStatesToVkAccessFlags2(barrier.stateBefore);
			VkAccessFlags2 dstAccessMask = resourceStatesToVkAccessFlags2(barrier.stateAfter);

			m_VkBufferMemoryBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
			m_VkBufferMemoryBarriers[i].pNext = nullptr;
			m_VkBufferMemoryBarriers[i].srcStageMask = srcStage;
			m_VkBufferMemoryBarriers[i].srcAccessMask = srcAccessMask;
			m_VkBufferMemoryBarriers[i].dstStageMask = dstStage;
			m_VkBufferMemoryBarriers[i].dstAccessMask = dstAccessMask;
			m_VkBufferMemoryBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			m_VkBufferMemoryBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			m_VkBufferMemoryBarriers[i].buffer = barrier.buffer->buffer;
			m_VkBufferMemoryBarriers[i].size = barrier.buffer->getDesc().size;
			m_VkBufferMemoryBarriers[i].offset = 0;
		}

		VkDependencyInfo dependencyInfo{};
		dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependencyInfo.pNext = nullptr;
		dependencyInfo.dependencyFlags = 0;
		dependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(m_VkImageMemoryBarriers.size());
		dependencyInfo.pImageMemoryBarriers = m_VkImageMemoryBarriers.data();
		dependencyInfo.bufferMemoryBarrierCount = static_cast<uint32_t>(m_VkBufferMemoryBarriers.size());
		dependencyInfo.pBufferMemoryBarriers = m_VkBufferMemoryBarriers.data();

		vkCmdPipelineBarrier2(m_CurrentCmdBuf->vkCmdBuf, &dependencyInfo);

		m_BufferBarriers.clear();
		m_TextureBarriers.clear();
		m_VkBufferMemoryBarriers.clear();
		m_VkImageMemoryBarriers.clear();
	}

	void CommandList::setBufferBarrier(BufferVk* buffer, VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess)
	{
		assert(buffer);
		ASSERT_MSG(m_CurrentCmdBuf, "Must call CommandList::open() before this method.");

		VkBufferMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
		barrier.pNext = nullptr;
		barrier.srcStageMask = resourceStatesToVkPipelineStageFlags2(buffer->getState());
		barrier.srcAccessMask = resourceStatesToVkAccessFlags2(buffer->getState());
		barrier.dstStageMask = dstStage;
		barrier.dstAccessMask = dstAccess;
		barrier.buffer = buffer->buffer;
		barrier.size = buffer->getDesc().size;
		barrier.offset = 0;

		VkDependencyInfo dependencyInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
		dependencyInfo.pNext = nullptr;
		dependencyInfo.dependencyFlags = 0;
		dependencyInfo.bufferMemoryBarrierCount = 1;
		dependencyInfo.pBufferMemoryBarriers = &barrier;

		vkCmdPipelineBarrier2(m_CurrentCmdBuf->vkCmdBuf, &dependencyInfo);
	}

	void CommandList::clearColorTexture(ITextureView* textureView, const ClearColor& color)
	{
		assert(textureView);
		ASSERT_MSG(m_CurrentCmdBuf, "Must call CommandList::open() before this method.");

		auto tv = checked_cast<TextureViewVk*>(textureView);
		auto texture = checked_cast<TextureVk*>(tv->getTexture());
		VkClearColorValue clearValue = convertVkClearColor(color, texture->m_Desc.format);

		// Check if the textureView is one of the currently bound renderTargetView
		int rendetTargetIndex = -1;
		for (uint32_t i = 0; i < m_LastGraphicsState.renderTargetCount; ++i)
		{
			if (m_LastGraphicsState.renderTargetViews[i] == textureView)
			{
				rendetTargetIndex = i;
			}
		}

		if (rendetTargetIndex != -1)
		{
			VkClearAttachment clearAttachment{};
			clearAttachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			clearAttachment.colorAttachment = static_cast<uint32_t>(rendetTargetIndex);
			clearAttachment.clearValue.color = clearValue;

			VkClearRect clearRect{};
			clearRect.rect.offset = { 0, 0 };
			clearRect.rect.extent = { texture->m_Desc.width,  texture->m_Desc.height };
			clearRect.baseArrayLayer = 0;
			clearRect.layerCount = texture->m_Desc.arraySize;

			vkCmdClearAttachments(m_CurrentCmdBuf->vkCmdBuf, 1, &clearAttachment, 1, &clearRect);
		}
		else
		{
			if (m_EnableAutoTransition)
			{
				transitionTextureState(tv->getTexture(), ResourceState::CopyDest);
			}
			commitBarriers();

			VkImageSubresourceRange imageSubresourceRange{};
			imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageSubresourceRange.baseArrayLayer = tv->m_Desc.baseArrayLayer;
			imageSubresourceRange.layerCount = tv->m_Desc.arrayLayerCount;
			imageSubresourceRange.baseMipLevel = tv->m_Desc.baseMipLevel;
			imageSubresourceRange.levelCount = tv->m_Desc.mipLevelCount;

			vkCmdClearColorImage(m_CurrentCmdBuf->vkCmdBuf, texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearValue, 1, &imageSubresourceRange);
		}
	}

	void CommandList::clearDepthStencil(ITextureView* textureView, ClearDepthStencilFlag flag, float depthVal, uint8_t stencilVal)
	{
		assert(textureView);
		ASSERT_MSG(m_CurrentCmdBuf, "Must call CommandList::open() before this method.");

		auto tv = checked_cast<TextureViewVk*>(textureView);
		auto texture = checked_cast<TextureVk*>(tv->getTexture());

		if (textureView == m_LastGraphicsState.depthStencilView)
		{
			VkClearAttachment clearAttachment{};
			if ((flag & ClearDepthStencilFlag::Depth) != 0)
			{
				clearAttachment.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
			}
			if ((flag & ClearDepthStencilFlag::Stencil) != 0)
			{
				clearAttachment.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}

			clearAttachment.colorAttachment = VK_ATTACHMENT_UNUSED;
			clearAttachment.clearValue.depthStencil.depth = depthVal;
			clearAttachment.clearValue.depthStencil.stencil = stencilVal;

			VkClearRect clearRect{};
			clearRect.rect.offset = { 0, 0 };
			clearRect.rect.extent = { texture->m_Desc.width,  texture->m_Desc.height };
			clearRect.baseArrayLayer = 0;
			clearRect.layerCount = texture->m_Desc.arraySize;

			vkCmdClearAttachments(m_CurrentCmdBuf->vkCmdBuf, 1, &clearAttachment, 1, &clearRect);
		}
		else
		{
			if (m_EnableAutoTransition)
			{
				transitionTextureState(texture, ResourceState::CopyDest);
			}
			commitBarriers();

			VkImageSubresourceRange imageSubresourceRange{};
			if ((flag & ClearDepthStencilFlag::Depth) != 0)
			{
				imageSubresourceRange.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
			}
			if ((flag & ClearDepthStencilFlag::Stencil) != 0)
			{
				imageSubresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
			imageSubresourceRange.baseArrayLayer = tv->m_Desc.baseArrayLayer;
			imageSubresourceRange.layerCount = tv->m_Desc.arrayLayerCount;
			imageSubresourceRange.baseMipLevel = tv->m_Desc.baseMipLevel;
			imageSubresourceRange.levelCount = tv->m_Desc.mipLevelCount;

			VkClearDepthStencilValue clearValue;
			clearValue.depth = depthVal;
			clearValue.stencil = stencilVal;

			vkCmdClearDepthStencilImage(m_CurrentCmdBuf->vkCmdBuf, texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearValue, 1, &imageSubresourceRange);
		}
	}

	void CommandList::clearBuffer(IBuffer* buffer, uint32_t value, uint64_t offset, uint64_t size)
	{
		assert(buffer);
		ASSERT_MSG(m_CurrentCmdBuf, "Must call CommandList::open() before this method.");

		auto buf = checked_cast<BufferVk*>(buffer);

		if (m_EnableAutoTransition)
		{
			transitionBufferState(buffer, ResourceState::CopyDest);
		}
		commitBarriers();

		vkCmdFillBuffer(m_CurrentCmdBuf->vkCmdBuf, buf->buffer, offset, size, value);
	}

	void CommandList::copyBuffer(IBuffer* srcBuffer, uint64_t srcOffset, IBuffer* dstBuffer, uint64_t dstOffset, uint64_t dataSize)
	{
		assert(srcBuffer && dstBuffer);
		ASSERT_MSG(m_CurrentCmdBuf, "Must call CommandList::open() before this method.");

		auto srcBuf = checked_cast<BufferVk*>(srcBuffer);
		auto dstBuf = checked_cast<BufferVk*>(dstBuffer);

		assert(srcOffset + dataSize <= srcBuf->getDesc().size);
		assert(dstOffset + dataSize <= dstBuf->getDesc().size);

		if (m_EnableAutoTransition)
		{
			transitionBufferState(srcBuffer, ResourceState::CopySource);
			transitionBufferState(dstBuffer, ResourceState::CopyDest);
		}
		commitBarriers();

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = srcOffset;
		copyRegion.dstOffset = dstOffset;
		copyRegion.size = dataSize;
		vkCmdCopyBuffer(m_CurrentCmdBuf->vkCmdBuf, srcBuf->buffer, dstBuf->buffer, 1, &copyRegion);
	}

	void CommandList::updateBuffer(IBuffer* buffer, const void* data, uint64_t dataSize, uint64_t offset)
	{
		assert(buffer);
		ASSERT_MSG(m_CurrentCmdBuf, "Must call CommandList::open() before this method.");

		auto buf = checked_cast<BufferVk*>(buffer);

		if (buf->getDesc().access != BufferAccess::GpuOnly)
		{
			LOG_ERROR("this method only works with Buffer that tagged with BufferAccess::GpuOnly");
			return;
		}

		// vkCmdUpdateBuffer requires that the data size is smaller than or equal to 64 kB,
		// and offset is a multiple of 4.
		const uint64_t vkCmdUpdateBufferLimit = 65536;
		if ((offset & 3) == 0 && (dataSize & 3) == 0 && dataSize < vkCmdUpdateBufferLimit)
		{

			if (m_EnableAutoTransition)
			{
				transitionBufferState(buf, ResourceState::CopyDest);
			}
			commitBarriers();

			// Round up the write size to a multiple of 4
			const uint64_t sizeToWrite = (dataSize + 3) & ~uint64_t(3);
			vkCmdUpdateBuffer(m_CurrentCmdBuf->vkCmdBuf, buf->buffer, offset, sizeToWrite, data);
		}
		else
		{
			BufferDesc stageBufferDesc;
			stageBufferDesc.access = BufferAccess::CpuWrite;
			stageBufferDesc.usage = BufferUsage::None;
			stageBufferDesc.size = dataSize;
			auto& stageBuffer = m_CurrentCmdBuf->referencedInternalStageBuffer.emplace_back();
			stageBuffer = std::unique_ptr<BufferVk>(checked_cast<BufferVk*>(m_RenderDevice.createBuffer(stageBufferDesc, data, dataSize)));
			copyBuffer(stageBuffer.get(), 0, buf, 0, dataSize);
		}
	}

	void* CommandList::mapBuffer(IBuffer* buffer, MapMode usage)
	{
		assert(buffer);
		ASSERT_MSG(m_CurrentCmdBuf, "Must call CommandList::open() before this method.");

		auto buf = checked_cast<BufferVk*>(buffer);

		ASSERT_MSG(buf->getDesc().access != BufferAccess::CpuRead,
			"Only tagged with BufferAccess::CpuRead buffer can be mapped.");

		if (usage == MapMode::Read)
		{
			setBufferBarrier(buf, VK_PIPELINE_STAGE_2_HOST_BIT, VK_ACCESS_2_HOST_READ_BIT);
		}
		// do noting, Submission guarantees the host write being complete.

		return buf->allocaionInfo.pMappedData;
	}

	void CommandList::updateTexture(ITexture* texture, const void* data, uint64_t dataSize, const TextureUpdateInfo& updateInfo)
	{
		assert(texture);
		ASSERT_MSG(m_CurrentCmdBuf, "Must call CommandList::open() before this method.");

		auto tex = checked_cast<TextureVk*>(texture);

		TextureCopyInfo copyInfo = getTextureCopyInfo(tex->getDesc().format, updateInfo.dstRegion,
			(uint32_t)m_RenderDevice.physicalDeviceProperties.limits.optimalBufferCopyRowPitchAlignment);

		ASSERT_MSG(updateInfo.dstRegion.maxX <= tex->getDesc().width << updateInfo.mipLevel &&
			updateInfo.dstRegion.maxY <= tex->getDesc().height << updateInfo.mipLevel &&
			updateInfo.dstRegion.maxZ <= tex->getDesc().depth << updateInfo.mipLevel, "dest region is out of bound for this miplevel.");

		ASSERT_MSG(updateInfo.srcRowPitch <= copyInfo.rowBytesCount, "src row pitch is blow the dst region row pitch.");
		ASSERT_MSG(dataSize >= copyInfo.regionBytesCount, "Not enough data was provided to update to the dst region.");

		BufferDesc stageBufferDesc;
		stageBufferDesc.access = BufferAccess::CpuWrite;
		stageBufferDesc.usage = BufferUsage::None;
		stageBufferDesc.size = copyInfo.regionBytesCount;
		auto& stageBuffer = m_CurrentCmdBuf->referencedInternalStageBuffer.emplace_back();
		stageBuffer = std::unique_ptr<BufferVk>(checked_cast<BufferVk*>(m_RenderDevice.createBuffer(stageBufferDesc, data, copyInfo.regionBytesCount)));

		uint32_t regionDepth = updateInfo.dstRegion.getDepth();

		for (uint32_t z = 0; z < regionDepth; ++z)
		{
			const uint8_t* srcPtr = reinterpret_cast<const uint8_t*>(data) + updateInfo.srcDepthPitch * z;
			const uint8_t* dstPtr = reinterpret_cast<const uint8_t*>(stageBuffer->allocaionInfo.pMappedData) + copyInfo.depthStride * z;
			for (uint32_t y = 0; y < copyInfo.rowCount; ++y)
			{
				memcpy(stageBuffer->allocaionInfo.pMappedData, srcPtr, copyInfo.rowBytesCount);
				srcPtr += updateInfo.srcRowPitch;
				dstPtr += copyInfo.rowStride;
			}
		}

		VkBufferImageCopy bufferCopyRegion = {};
		bufferCopyRegion.imageSubresource.aspectMask = getVkAspectMask(tex->format);
		bufferCopyRegion.imageSubresource.baseArrayLayer = updateInfo.arrayLayer;
		bufferCopyRegion.imageSubresource.layerCount = 1;
		bufferCopyRegion.imageSubresource.mipLevel = updateInfo.mipLevel;
		bufferCopyRegion.imageOffset = { static_cast<int32_t>(updateInfo.dstRegion.minX), static_cast<int32_t>(updateInfo.dstRegion.minY), static_cast<int32_t>(updateInfo.dstRegion.minZ) };
		bufferCopyRegion.imageExtent = { updateInfo.dstRegion.getWidth(), updateInfo.dstRegion.getHeight(), updateInfo.dstRegion.getDepth() };

		if (m_EnableAutoTransition)
		{
			transitionTextureState(tex, ResourceState::CopyDest);
		}
		commitBarriers();

		vkCmdCopyBufferToImage(m_CurrentCmdBuf->vkCmdBuf, stageBuffer->buffer, tex->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);
	}

	void CommandList::transitionResourceSet(IResourceSet* set, ShaderType dstVisibleStages)
	{
		assert(set);
		auto resourceSet = checked_cast<ResourceSetVk*>(set);

		for (auto& itemWithVisibleStages : resourceSet->resourcesNeedStateTransition)
		{
			if ((itemWithVisibleStages.visibleStages & dstVisibleStages) == 0)
			{
				continue;
			}

			switch (itemWithVisibleStages.binding.type)
			{
			case ShaderResourceType::TextureWithSampler:
			case ShaderResourceType::SampledTexture:
			{
				assert(itemWithVisibleStages.binding.textureView);
				auto textureView = checked_cast<TextureViewVk*>(itemWithVisibleStages.binding.textureView);
				transitionTextureState(textureView->getTexture(), ResourceState::ShaderResource);
				break;
			}
			case ShaderResourceType::StorageTexture:
			{
				assert(itemWithVisibleStages.binding.textureView);
				auto textureView = checked_cast<TextureViewVk*>(itemWithVisibleStages.binding.textureView);
				transitionTextureState(textureView->getTexture(), ResourceState::UnorderedAccess);
				break;
			}
			case ShaderResourceType::UniformBuffer:
			{
				assert(itemWithVisibleStages.binding.buffer);
				auto buffer = checked_cast<BufferVk*>(itemWithVisibleStages.binding.buffer);
				transitionBufferState(buffer, ResourceState::ShaderResource);
				break;
			}
			case ShaderResourceType::StorageBuffer:
			{
				assert(itemWithVisibleStages.binding.buffer);
				auto buffer = checked_cast<BufferVk*>(itemWithVisibleStages.binding.buffer);
				transitionBufferState(buffer, ResourceState::UnorderedAccess);
				break;
			}
			default:
				break;
			}
		}
	}

	void CommandList::transitionResourceSet(IResourceSet* resourceSet)
	{
		assert(resourceSet);

		switch (m_LastPipelineType)
		{
		case rhi::CommandList::PipelineType::Graphics:
		{
			constexpr ShaderType graphicsStages = ShaderType::Vertex | ShaderType::Fragment |
				ShaderType::Geometry | ShaderType::TessellationControl | ShaderType::TessellationEvaluation;
			transitionResourceSet(resourceSet, graphicsStages);
			break;
		}
		case rhi::CommandList::PipelineType::Compute:
			transitionResourceSet(resourceSet, ShaderType::Compute);
			break;
		case rhi::CommandList::PipelineType::Unknown:
		default:
			LOG_ERROR("Must set pipelineState before transition resourceSet.");
			break;
		}
	}

	void CommandList::commitShaderResources(IResourceSet* resourceSet, uint32_t dstSet)
	{
		assert(resourceSet);
		ASSERT_MSG(m_CurrentCmdBuf, "Must call CommandList::open() before this method.");

		auto set = checked_cast<ResourceSetVk*>(resourceSet);

		// reference host visible buffer

		switch (m_LastPipelineType)
		{
		case rhi::CommandList::PipelineType::Graphics:
		{
			if (m_EnableAutoTransition)
			{
				constexpr ShaderType graphicsStages = ShaderType::Vertex | ShaderType::Fragment |
					ShaderType::Geometry | ShaderType::TessellationControl | ShaderType::TessellationEvaluation;
				transitionResourceSet(resourceSet, graphicsStages);
			}

			auto pipeline = checked_cast<GraphicsPipelineVk*>(m_LastGraphicsState.pipeline);

			vkCmdPushDescriptorSetKHR(m_CurrentCmdBuf->vkCmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipelineLayout, dstSet, set->writeDescriptorSets.size(), set->writeDescriptorSets.data());

			break;
		}
		case rhi::CommandList::PipelineType::Compute:
		{
			if (m_EnableAutoTransition)
			{
				transitionResourceSet(resourceSet, ShaderType::Compute);
			}

			auto pipeline = checked_cast<ComputePipelineVk*>(m_LastComputeState.pipeline);

			vkCmdPushDescriptorSetKHR(m_CurrentCmdBuf->vkCmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->pipelineLayout, dstSet, set->writeDescriptorSets.size(), set->writeDescriptorSets.data());

			break;
		}
		default:
			LOG_ERROR("Must set pipelineState before transition resourceSet.");
			break;
		}

		commitBarriers();
	}

	static inline void fillVkRenderingInfo(const GraphicsState& state, VkRenderingInfo& renderingInfo,
		std::array<VkRenderingAttachmentInfo, g_MaxColorAttachments>& colorAttachments, 
		VkRenderingAttachmentInfo& depthStencilAttachment)
	{
		if (state.renderTargetCount > 0)
		{
			auto rtv = checked_cast<TextureViewVk*>(state.renderTargetViews[0]);
			auto texture = checked_cast<TextureVk*>(rtv->getTexture());
			renderingInfo.renderArea.extent = { texture->getDesc().width , texture->getDesc().height };
		}
		else
		{
			auto dsv = checked_cast<TextureViewVk*>(state.depthStencilView);
			auto texture = checked_cast<TextureVk*>(dsv->getTexture());
			renderingInfo.renderArea.extent = { texture->getDesc().width , texture->getDesc().height };
		}
		renderingInfo.layerCount = 1;

		for (uint32_t i = 0; i < state.renderTargetCount; ++i)
		{
			auto rtv = checked_cast<TextureViewVk*>(state.renderTargetViews[i]);
			auto& colorAttachment = colorAttachments[i];
			colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			colorAttachment.pNext = nullptr;
			colorAttachment.imageView = rtv->imageView;
			colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorAttachment.loadOp = state.clearRenderTarget ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.clearValue = { 0.0f, 0.0f, 0.f, 0.0f };
		}

		depthStencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		depthStencilAttachment.pNext = nullptr;
		depthStencilAttachment.imageView = checked_cast<TextureViewVk*>(state.depthStencilView)->imageView;
		depthStencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthStencilAttachment.clearValue.depthStencil = { 1.0f,  0 };

		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
		renderingInfo.pNext = nullptr;
		renderingInfo.colorAttachmentCount = state.renderTargetCount;
		renderingInfo.pColorAttachments = colorAttachments.data();
		renderingInfo.pDepthAttachment = &depthStencilAttachment;
		renderingInfo.pStencilAttachment = &depthStencilAttachment;
		renderingInfo.viewMask = 0;
	}

	void CommandList::setPipeline(const GraphicsState& state)
	{
		ASSERT_MSG(m_CurrentCmdBuf, "Must call CommandList::open() before this method.");

		// vertex buffer
		for (uint32_t i = 0; i < state.vertexBufferCount; ++i)
		{
			assert(state.vertexBuffers[i].buffer != nullptr);
			auto buffer = checked_cast<BufferVk*>(state.vertexBuffers[i].buffer);
			if (m_EnableAutoTransition)
			{
				transitionBufferState(buffer, ResourceState::VertexBuffer);
			}
		}

		// index buffer
		if (state.indexBuffer.buffer)
		{
			assert(state.indexBuffer.buffer != nullptr);
			auto buffer = checked_cast<BufferVk*>(state.indexBuffer.buffer);
			if (m_EnableAutoTransition)
			{
				transitionBufferState(buffer, ResourceState::IndexBuffer);
			}
		}

		assert(state.renderTargetCount > 0 || state.depthStencilView != nullptr);
		for (uint32_t i = 0; i < state.renderTargetCount; ++i)
		{
			assert(state.renderTargetViews[i] != nullptr);
			auto rtv = checked_cast<TextureViewVk*>(state.renderTargetViews[i]);
			transitionTextureState(rtv->getTexture(), ResourceState::RenderTarget);
		}
		if (state.depthStencilView)
		{
			auto dsv = checked_cast<TextureViewVk*>(state.depthStencilView);
			transitionTextureState(dsv->getTexture(), ResourceState::DepthWrite);
		}
		// Because once the graphics state is set, the render target is always changed. 
		// and Barrier must not be placed within a render section started with vkCmdBeginRendering
		endRendering();
		commitBarriers();

		std::array<VkRenderingAttachmentInfo, g_MaxColorAttachments> colorAttachments{};
		VkRenderingAttachmentInfo depthAttachment{};

		if (arraysAreDifferent(state.vertexBuffers, state.vertexBufferCount,
			m_LastGraphicsState.vertexBuffers, m_LastGraphicsState.vertexBufferCount))
		{
			VkBuffer buffers[g_MaxVertexInputBindings]{};
			VkDeviceSize bufferOffsets[g_MaxVertexInputBindings]{};
			uint32_t maxBindingSlot = 0;
			for (uint32_t i = 0; i < state.vertexBufferCount; ++i)
			{
				const VertexBufferBinding& binding = state.vertexBuffers[i];
				buffers[binding.bindingSlot] = checked_cast<BufferVk*>(binding.buffer)->buffer;
				bufferOffsets[binding.bindingSlot] = binding.offset;
				maxBindingSlot = (std::max)(maxBindingSlot, binding.bindingSlot);
			}
			vkCmdBindVertexBuffers(m_CurrentCmdBuf->vkCmdBuf, 0, maxBindingSlot + 1, buffers, bufferOffsets);
		}

		if (state.indexBuffer.buffer && state.indexBuffer != m_LastGraphicsState.indexBuffer)
		{
			auto* buffer = checked_cast<BufferVk*>(state.indexBuffer.buffer);
			vkCmdBindIndexBuffer(m_CurrentCmdBuf->vkCmdBuf, buffer->buffer, state.indexBuffer.offset,
				state.indexBuffer.format == Format::R16_UINT ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
		}
		// set viewports and scissors 
		ASSERT_MSG(state.viewportCount == checked_cast<GraphicsPipelineVk*>(state.pipeline)->getDesc().viewportCount,
			"The number of viewports provided does not match the number specified when the pipeline was created.");
		if (arraysAreDifferent(state.viewports, state.viewportCount,
			m_LastGraphicsState.viewports, m_LastGraphicsState.viewportCount))
		{
			VkViewport viewports[g_MaxViewPorts]{};
			for (uint32_t i = 0; i < state.viewportCount; ++i)
			{
				viewports[i] = convertViewport(state.viewports[i]);
			}
			vkCmdSetViewport(m_CurrentCmdBuf->vkCmdBuf, 0, state.viewportCount, viewports);

			// If no scissor is provided, it will be set to the corresponding viewport size.
			VkRect2D scissors[g_MaxViewPorts]{};
			uint32_t scissorsCount = state.viewportCount;
			for (uint32_t i = 0; i < scissorsCount; ++i)
			{
				scissors[i].offset = { (int)state.viewports[i].minX, (int)state.viewports[i].minY };
				scissors[i].extent = { static_cast<uint32_t>(std::abs(state.viewports[i].getWidth())), static_cast<uint32_t>(std::abs(state.viewports[i].getHeight())) };
			}

			vkCmdSetScissor(m_CurrentCmdBuf->vkCmdBuf, 0, scissorsCount, scissors);
		}

		assert(state.pipeline != nullptr);
		GraphicsPipelineVk* pipeline = checked_cast<GraphicsPipelineVk*>(state.pipeline);

		if (state.pipeline != m_LastGraphicsState.pipeline)
		{
			vkCmdBindPipeline(m_CurrentCmdBuf->vkCmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
		}

		m_LastPipelineType = PipelineType::Graphics;
		m_LastGraphicsState = state;
	}

	void CommandList::setScissors(const Rect* scissors, uint32_t scissorCount)
	{
		ASSERT_MSG(m_CurrentCmdBuf, "Must call CommandList::open() before this method.");
		ASSERT_MSG(m_LastGraphicsState.pipeline, "set GraphicsState before set scissors");
		ASSERT_MSG(scissorCount == checked_cast<GraphicsPipelineVk*>(m_LastGraphicsState.pipeline)->getDesc().viewportCount,
			"The number of scissors must be the same as the number of viewports.");

		VkRect2D rects[g_MaxViewPorts]{};
		for (uint32_t i = 0; i < scissorCount; ++i)
		{
			rects[i].offset = { scissors[i].minX, scissors[i].minY };
			rects[i].extent = { static_cast<uint32_t>(std::abs(scissors[i].getWidth())), static_cast<uint32_t>(std::abs(scissors[i].getHeight())) };
		}

		vkCmdSetScissor(m_CurrentCmdBuf->vkCmdBuf, 0, scissorCount, rects);
	}

	void CommandList::setPushConstant(ShaderType stages, const void* data)
	{
		ASSERT_MSG(m_CurrentCmdBuf, "Must call CommandList::open() before this method.");
		ASSERT_MSG(m_LastGraphicsState.pipeline || m_LastComputeState.pipeline, "Must set PipelineState before push constant.");

		if (m_LastGraphicsState.pipeline)
		{
			auto pipeline = checked_cast<GraphicsPipelineVk*>(m_LastGraphicsState.pipeline);
			for (auto& info : pipeline->pushConstantInfos)
			{
				if (stages == info.desc.stage)
				{
					vkCmdPushConstants(m_CurrentCmdBuf->vkCmdBuf, pipeline->pipelineLayout,
						shaderTypeToVkShaderStageFlagBits(stages), info.offset, info.desc.size, data);
					break;
				}
			}
		}
		else if (m_LastComputeState.pipeline)
		{
			auto pipeline = checked_cast<ComputePipelineVk*>(m_LastComputeState.pipeline);
			for (auto& info : pipeline->pushConstantInfos)
			{
				if (stages == info.desc.stage)
				{
					vkCmdPushConstants(m_CurrentCmdBuf->vkCmdBuf, pipeline->pipelineLayout,
						shaderTypeToVkShaderStageFlagBits(stages), info.offset, info.desc.size, data);
					break;
				}
			}
		}
	}

	void CommandList::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
	{
		ASSERT_MSG(m_CurrentCmdBuf, "Must call CommandList::open() before this method.");

		if (!m_RenderingStarted)
		{
			std::array<VkRenderingAttachmentInfo, g_MaxColorAttachments> colorAttachments{};
			VkRenderingAttachmentInfo depthAttachment{};

			VkRenderingInfo renderingInfo{};
			fillVkRenderingInfo(m_LastGraphicsState, renderingInfo, colorAttachments, depthAttachment);
			vkCmdBeginRendering(m_CurrentCmdBuf->vkCmdBuf, &renderingInfo);
			m_RenderingStarted = true;
		}

		vkCmdDraw(m_CurrentCmdBuf->vkCmdBuf, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void CommandList::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
	{
		ASSERT_MSG(m_CurrentCmdBuf, "Must call CommandList::open() before this method.");

		if (!m_RenderingStarted)
		{
			std::array<VkRenderingAttachmentInfo, g_MaxColorAttachments> colorAttachments{};
			VkRenderingAttachmentInfo depthAttachment{};

			VkRenderingInfo renderingInfo{};
			fillVkRenderingInfo(m_LastGraphicsState, renderingInfo, colorAttachments, depthAttachment);
			vkCmdBeginRendering(m_CurrentCmdBuf->vkCmdBuf, &renderingInfo);
			m_RenderingStarted = true;
		}

		vkCmdDrawIndexed(m_CurrentCmdBuf->vkCmdBuf, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void CommandList::drawIndirect(IBuffer* argsBuffer, uint64_t offset, uint32_t drawCount)
	{
		assert(argsBuffer);
		ASSERT_MSG(m_CurrentCmdBuf, "Must call CommandList::open() before this method.");

		auto indirectBuffer = checked_cast<BufferVk*>(argsBuffer);
		if (m_EnableAutoTransition)
		{
			transitionBufferState(indirectBuffer, ResourceState::IndirectBuffer);
		}
		commitBarriers();

		if (!m_RenderingStarted)
		{
			std::array<VkRenderingAttachmentInfo, g_MaxColorAttachments> colorAttachments{};
			VkRenderingAttachmentInfo depthAttachment{};

			VkRenderingInfo renderingInfo{};
			fillVkRenderingInfo(m_LastGraphicsState, renderingInfo, colorAttachments, depthAttachment);
			vkCmdBeginRendering(m_CurrentCmdBuf->vkCmdBuf, &renderingInfo);
			m_RenderingStarted = true;
		}

		vkCmdDrawIndirect(m_CurrentCmdBuf->vkCmdBuf, indirectBuffer->buffer, offset, drawCount, sizeof(DrawIndirectCommand));
	}

	void CommandList::drawIndexedIndirect(IBuffer* argsBuffer, uint64_t offset, uint32_t drawCount)
	{
		assert(argsBuffer);
		ASSERT_MSG(m_CurrentCmdBuf, "Must call CommandList::open() before this method.");

		auto indirectBuffer = checked_cast<BufferVk*>(argsBuffer);
		if (m_EnableAutoTransition)
		{
			transitionBufferState(indirectBuffer, ResourceState::IndirectBuffer);
		}
		commitBarriers();

		if (!m_RenderingStarted)
		{
			std::array<VkRenderingAttachmentInfo, g_MaxColorAttachments> colorAttachments{};
			VkRenderingAttachmentInfo depthAttachment{};

			VkRenderingInfo renderingInfo{};
			fillVkRenderingInfo(m_LastGraphicsState, renderingInfo, colorAttachments, depthAttachment);
			vkCmdBeginRendering(m_CurrentCmdBuf->vkCmdBuf, &renderingInfo);
			m_RenderingStarted = true;
		}

		vkCmdDrawIndexedIndirect(m_CurrentCmdBuf->vkCmdBuf, indirectBuffer->buffer, offset, drawCount, sizeof(DrawIndexedIndirectCommand));
	}

	void CommandList::setPipeline(const ComputeState& state)
	{
		ASSERT_MSG(m_CurrentCmdBuf, "Must call CommandList::open() before this method.");

		endRendering();

		auto pipeline = checked_cast<ComputePipelineVk*>(state.pipeline);

		if (state.pipeline != m_LastComputeState.pipeline)
		{
			vkCmdBindPipeline(m_CurrentCmdBuf->vkCmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->pipeline);
		}

		m_LastPipelineType = PipelineType::Compute;
		m_LastComputeState = state;
	}

	void CommandList::dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{
		ASSERT_MSG(m_CurrentCmdBuf, "Must call CommandList::open() before this method.");

		vkCmdDispatch(m_CurrentCmdBuf->vkCmdBuf, groupCountX, groupCountY, groupCountZ);
	}

	void CommandList::dispatchIndirect(IBuffer* argsBuffer, uint64_t offset)
	{
		assert(argsBuffer);
		ASSERT_MSG(m_CurrentCmdBuf, "Must call CommandList::open() before this method.");

		auto indirectBuffer = checked_cast<BufferVk*>(argsBuffer);
		if (m_EnableAutoTransition)
		{
			transitionBufferState(indirectBuffer, ResourceState::IndirectBuffer);
		}
		commitBarriers();

		vkCmdDispatchIndirect(m_CurrentCmdBuf->vkCmdBuf, indirectBuffer->buffer, offset);
	}

	void CommandList::beginDebugLabel(const char* labelName, Color color)
	{
		assert(labelName);
		ASSERT_MSG(m_CurrentCmdBuf, "Must call CommandList::open() before this method.");

		VkDebugUtilsLabelEXT debugUtilsLabel{};
		debugUtilsLabel.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
		debugUtilsLabel.pLabelName = labelName;
		memcpy(debugUtilsLabel.color, &color, sizeof(debugUtilsLabel.color));

		if (this->vkCmdBeginDebugUtilsLabelEXT)
		{
			this->vkCmdBeginDebugUtilsLabelEXT(m_CurrentCmdBuf->vkCmdBuf, &debugUtilsLabel);
		}
	}

	void CommandList::endDebugLabel()
	{
		ASSERT_MSG(m_CurrentCmdBuf, "Must call CommandList::open() before this method.");

		if (this->vkCmdEndDebugUtilsLabelEXT)
		{
			this->vkCmdEndDebugUtilsLabelEXT(m_CurrentCmdBuf->vkCmdBuf);
		}
	}

	Object CommandList::getNativeObject(NativeObjectType type) const
	{
		if (type == NativeObjectType::VK_CommandBuffer)
		{
			return static_cast<Object>(m_CurrentCmdBuf->vkCmdBuf);
		}
		return nullptr;
	}

	bool CommandList::hasPresentTexture()
	{
		
	}

	UploadAllocation CommandList::UploadAllocator::allocate(uint64_t dataSize, uint32_t alignment)
	{
		UploadAllocation allocation;
		UploadPage pageToRetire;
		if (m_CurrentPage.valid())
		{
			uint64_t alignedOffset = AlignUp(m_CurrentPage.offset, alignment);
			uint64_t endPoint = alignedOffset + dataSize;
			if (m_CurrentPage.buffer->m_Desc.size >= endPoint)
			{
				m_CurrentPage.offset = endPoint;
				m_CurrentPage.inUse = true;

				allocation.buffer = m_CurrentPage.buffer;
				allocation.offset = alignedOffset;
				allocation.mappedAdress = static_cast<uint8_t*>(allocation.buffer->allocaionInfo.pMappedData) + alignedOffset;
				return allocation;
			}

			// if there is not enough space
			pageToRetire = m_CurrentPage;
		}

		for (auto it = m_UploadPagePool.begin(); it != m_UploadPagePool.end(); ++it)
		{
			UploadPage& page = *it;
			if (!page.inUse && page.buffer->m_Desc.size >= dataSize)
			{
				m_CurrentPage = page;
				std::swap(page, *m_UploadPagePool.end());
				m_UploadPagePool.pop_back();
				break;
			}
		}

		if (pageToRetire.valid())
		{
			m_UploadPagePool.push_back(pageToRetire);
		}

		// If we can't find a suitable one in the end, create one.
		if (!m_CurrentPage.valid())
		{
			m_CurrentPage = createNewPage(dataSize);
		}

		m_CurrentPage.offset = dataSize;
		allocation.buffer = m_CurrentPage.buffer;
		allocation.offset = 0;
		allocation.mappedAdress = m_CurrentPage.buffer->allocaionInfo.pMappedData;
		return allocation;
	}

	CommandList::UploadAllocator::UploadPage CommandList::UploadAllocator::createNewPage(uint64_t size)
	{
		UploadPage newPage{};
		BufferDesc desc{};
		desc.access = BufferAccess::CpuWrite;
		desc.size = AlignUp((std::max)(size, c_DefaultPageSize), c_SizeAlignment);
		newPage.buffer = checked_cast<BufferVk*>(m_RenderDevice->createBuffer(desc));
		return newPage;
	}
}