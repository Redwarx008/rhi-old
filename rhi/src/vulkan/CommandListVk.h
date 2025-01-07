#pragma once

#include "rhi/rhi.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>


namespace rhi::vulkan
{
	class Device;
	class TextureVk;
	class Buffer;
	struct ContextVk;
	struct TextureUpdateInfo;
	class CommandList;

	struct UploadAllocation
	{
		Buffer* buffer = nullptr;
		uint64_t offset = 0;
		void* mappedAdress = nullptr;
	};

	class CommandList final : public ICommandList
	{
	public:
		~CommandList();
		explicit CommandList(Device* renderDevice, const ContextVk& context);
		void open() override;
		void close() override;

		void waitCommandList(ICommandList* other) override;

		void setResourceAutoTransition(bool enable) override;
		void commitBarriers() override;
		void transitionTextureState(ITexture* texture, ResourceState newState) override;
		void transitionBufferState(IBuffer* buffer, ResourceState newState) override;
		void transitionResourceSet(IResourceSet* resourceSet) override;

		void clearColorTexture(ITextureView* textureView, const ClearColor& color) override;
		void clearDepthStencil(ITextureView* textureView, ClearDepthStencilFlag flag, float depthVal, uint8_t stencilVal) override;
		void clearBuffer(IBuffer* buffer, uint32_t value, uint64_t offset = 0, uint64_t size = ~0ull) override;
		void updateBuffer(IBuffer* buffer, const void* data, uint64_t dataSize, uint64_t offset) override;
		void copyBuffer(IBuffer* srcBuffer, uint64_t srcOffset, IBuffer* dstBuffer, uint64_t dstOffset, uint64_t dataSize) override;
		void* mapBuffer(IBuffer* buffer, MapMode usage) override;
		void updateTexture(ITexture* texture, const void* data, uint64_t dataSize, const TextureUpdateInfo& updateInfo) override;

		void commitShaderResources(IResourceSet* resourceSet, uint32_t dstSet = 0) override;

		void setPushConstant(ShaderType stages, const void* data) override;
		void setScissors(const Rect* scissors, uint32_t scissorCount) override;
		void setPipeline(const GraphicsState& state) override;

		void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
		void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) override;
		void drawIndirect(IBuffer* argsBuffer, uint64_t offset, uint32_t drawCount) override;
		void drawIndexedIndirect(IBuffer* argsBuffer, uint64_t offset, uint32_t drawCount) override;

		void setPipeline(const ComputeState& state) override;
		void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;
		void dispatchIndirect(IBuffer* argsBuffer, uint64_t offset) override;

		void beginDebugLabel(const char* labelName, Color color = Color()) override;
		void endDebugLabel() override;

		Object getNativeObject(NativeObjectType type) const override;

		void transitionFromSubmmitedState(ITexture* texture, ResourceState newState);
		void updateSubmittedState();
		bool hasPresentTexture() const;

		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
		VkCommandPool commandPool = VK_NULL_HANDLE;
		QueueType queueType = QueueType::Graphics;
		uint64_t submitID = 0;
		uint32_t allocateIndex = 0;
	private:
		CommandList() = delete;
		void transitionResourceSet(IResourceSet* set, ShaderType dstVisibleStages);
		void setBufferBarrier(Buffer* buffer, VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess);
		void endRendering();

		bool m_EnableAutoTransition = true;
		bool m_RenderingStarted = false;
		QueueType m_QueueType;

		enum class PipelineType
		{
			Unknown,
			Graphics,
			Compute,
		};
		PipelineType m_LastPipelineType = PipelineType::Unknown;
		GraphicsState m_LastGraphicsState;
		ComputeState m_LastComputeState;

		struct TextureBarrier
		{
			TextureVk* texture = nullptr;
			ResourceState stateBefore = ResourceState::Undefined;
			ResourceState stateAfter = ResourceState::Undefined;
		};

		struct BufferBarrier
		{
			Buffer* buffer = nullptr;
			ResourceState stateBefore = ResourceState::Undefined;
			ResourceState stateAfter = ResourceState::Undefined;
		};
		std::vector<TextureBarrier> m_TextureBarriers;
		std::vector<BufferBarrier> m_BufferBarriers;

		std::vector<VkImageMemoryBarrier2> m_VkImageMemoryBarriers;
		std::vector<VkBufferMemoryBarrier2> m_VkBufferMemoryBarriers;

		std::vector<ICommandList*> m_WaitCommandLists;

		Device* m_RenderDevice;
		const ContextVk& m_Context;

		// todo: delete it, if vulkan 1.4 is released.
		PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetKHR = nullptr;

		PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT = nullptr;
		PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT = nullptr;

	};
}