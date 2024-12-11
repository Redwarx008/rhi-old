#pragma once

#include "rhi/rhi.h"
#if defined RHI_ENABLE_THREAD_RECORDING
#include <mutex>
#endif
#include <vk_mem_alloc.h>
#include "vk_resource.h"

namespace rhi
{
	class CommandBuffer;

	class RenderDeviceVk final : public IRenderDevice
	{
	public:
		~RenderDeviceVk();
		// Internal methods
		static RenderDeviceVk* create(const RenderDeviceCreateInfo& desc);
		CommandBuffer* getOrCreateCommandBuffer();
		void setSwapChainImageAvailableSeamaphore(const VkSemaphore& semaphore);
		void setRenderCompleteSemaphore(const VkSemaphore& semaphore);
		TextureVk* createRenderTarget(const TextureDesc& desc, VkImage image);
		void recycleCommandBuffers();
		void executePresentCommandList(ICommandList* cmdList);

		RenderDeviceCreateInfo createInfo{};
		ContextVk context{};
		VkQueue queue{ VK_NULL_HANDLE };
		VkPhysicalDeviceProperties physicalDeviceProperties{};
		uint32_t queueFamilyIndex = UINT32_MAX;
		uint32_t maxPushDescriptors = 0;
		uint64_t lastSubmittedID = 0;

		// Interface implementation
		void waitIdle() override;

		ICommandList* beginCommandList(QueueType queueType = QueueType::Graphics) override;

		ITexture* createTexture(const TextureDesc& desc) override;
		IBuffer* createBuffer(const BufferDesc& desc) override;
		IBuffer* createBuffer(const BufferDesc& desc, const void* data, size_t dataSize) override;
		IShader* createShader(const ShaderCreateInfo& shaderCI, const uint32_t* pCode, size_t codeSize) override;
		ISampler* createSampler(const SamplerDesc& desc) override;

		IResourceSetLayout* createResourceSetLayout(const ResourceSetLayoutBinding* bindings, uint32_t bindingCount) override;
		IResourceSet* createResourceSet(const IResourceSetLayout* layout, const ResourceSetBinding* bindings, uint32_t bindingCount) override;

		IGraphicsPipeline* createGraphicsPipeline(const GraphicsPipelineCreateInfo& pipelineCI) override;
		IComputePipeline* createComputePipeline(const ComputePipelineCreateInfo& pipelineCI) override;

		void* mapBuffer(IBuffer* buffer) override;

		uint64_t executeCommandLists(ICommandList** cmdLists, size_t numCmdLists) override;
		void waitForExecution(uint64_t executeID, uint64_t timeout = UINT64_MAX) override;

		void updateResourceSet(IResourceSet* set, const ResourceSetBinding* bindings, uint32_t bindingCount) override;

	private:
		RenderDeviceVk() = default;
		bool createInstance(bool enableDebugRuntime);
		bool pickPhysicalDevice();
		bool createDevice(const RenderDeviceCreateInfo& desc);
		void destroyDebugUtilsMessenger();
#if defined RHI_ENABLE_THREAD_RECORDING
		std::mutex m_Mutex;
#endif
		VmaAllocator m_Allocator{VK_NULL_HANDLE};

		VkDebugUtilsMessengerEXT m_DebugUtilsMessenger{ VK_NULL_HANDLE };

		VkSemaphore m_SwapChainImgAvailableSemaphore{ VK_NULL_HANDLE };

		VkSemaphore m_RenderCompleteSemaphore{ VK_NULL_HANDLE };



		std::vector<VkCommandBufferSubmitInfo> m_CmdBufSubmitInfos;
		std::vector<CommandBuffer*> m_CommandBufferInFlight;
		std::vector<CommandBuffer*> m_CommandBufferPool;
		std::vector<CommandBuffer*> m_AllCommandBuffers; // to release CommandBuffers
	};
}

