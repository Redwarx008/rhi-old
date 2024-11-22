#pragma once

#include "rhi/rhi.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>


namespace rhi
{
	class RenderDeviceVk;
	class TextureVk;
	class BufferVk;
	struct ContextVk;
	struct TextureUpdateInfo;

	class CommandBuffer
	{
	public:
		explicit CommandBuffer(const ContextVk& context)
			:m_Context(context)
		{}
		~CommandBuffer();
		VkCommandBuffer vkCmdBuf{ VK_NULL_HANDLE };
		VkCommandPool vkCmdPool{ VK_NULL_HANDLE };

		std::vector<std::unique_ptr<BufferVk>> referencedInternalStageBuffer;
		uint64_t submitID = 0;
	private:
		const ContextVk& m_Context;
	};

	class CommandListVk final : public ICommandList
	{
	public:
		~CommandListVk();
		explicit CommandListVk(RenderDeviceVk& renderDevice);
		void open() override;
		void close() override;

		void setResourceAutoTransition(bool enable) override;
		void commitBarriers() override;
		void transitionTextureState(ITexture* texture, ResourceState newState) override;
		void transitionBufferState(IBuffer* buffer, ResourceState newState) override;
		void transitionResourceSet(IResourceSet* resourceSet) override;

		void clearColorTexture(ITextureView* textureView, const ClearColor& color) override;
		void clearDepthStencil(ITextureView* textureView, ClearDepthStencilFlag flag, float depthVal, uint8_t stencilVal) override;
		void clearBuffer(IBuffer* buffer, uint32_t value) override;
		void updateBuffer(IBuffer* buffer, const void* data, uint64_t dataSize, uint64_t offset) override;
		void copyBuffer(IBuffer* srcBuffer, uint64_t srcOffset, IBuffer* dstBuffer, uint64_t dstOffset, uint64_t dataSize) override;
		void* mapBuffer(IBuffer* buffer, MapBufferUsage usage) override;
		void updateTexture(ITexture* texture, const void* data, uint64_t dataSize, const TextureUpdateInfo& updateInfo) override;

		void commitResourceSet(IResourceSet* resourceSet, uint32_t dstSet = 0) override;

		void setPushConstant(ShaderType stages, const void* data) override;
		void setScissors(const Rect* scissors, uint32_t scissorCount) override;
		void setGraphicsState(const GraphicsState& state) override;

		void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
		void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) override;
		void drawIndirect(uint64_t offset, uint32_t drawCount) override;
		void drawIndexedIndirect(uint64_t offset, uint32_t drawCount) override;

		void setComputeState(const ComputeState& state) override;
		void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;
		void dispatchIndirect(uint64_t offset) override;

		Object getNativeObject(NativeObjectType type) const override;

		void transitionFromSubmmitedState(ITexture* texture, ResourceState newState);
		void updateSubmittedState();
		bool hasSetGraphicPipeline() const { return m_LastGraphicsState.pipeline != nullptr; }
		CommandBuffer* getCommandBuffer() const { return m_CurrentCmdBuf; }
	private:
		CommandListVk() = delete;
		void transitionResourceSet(IResourceSet* set, ShaderType dstVisibleStages);
		void setBufferBarrier(BufferVk* buffer, VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess);
		void endRendering();

		bool m_EnableAutoTransition = true;
		bool m_RenderingStarted = false;

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
			BufferVk* buffer = nullptr;
			ResourceState stateBefore = ResourceState::Undefined;
			ResourceState stateAfter = ResourceState::Undefined;
		};
		std::vector<TextureBarrier> m_TextureBarriers;
		std::vector<BufferBarrier> m_BufferBarriers;

		std::vector<TextureVk*> m_TrackingSubmittedStates;

		std::vector<VkImageMemoryBarrier2> m_VkImageMemoryBarriers;
		std::vector<VkBufferMemoryBarrier2> m_VkBufferMemoryBarriers;

		CommandBuffer* m_CurrentCmdBuf = nullptr;

		RenderDeviceVk& m_RenderDevice;

		// todo: delete it, if vulkan 1.4 is released.
		PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetKHR = nullptr;
	};
}