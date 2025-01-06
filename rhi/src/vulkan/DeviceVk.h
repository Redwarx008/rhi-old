#pragma once

#include "rhi/rhi.h"
#if defined RHI_ENABLE_THREAD_RECORDING
#include <mutex>
#endif
#include <vk_mem_alloc.h>

#include <array>

namespace rhi::vulkan
{
	class CommandBuffer;
	class CommandQueue;

	//template <typename T>
	//class Ref;


	class Device final : public IDevice
	{
	public:
		~Device();
		// Internal methodsd
		static Device* create(const DeviceCreateInfo& desc);

		Ref<ITexture> createTextureImpl(const TextureDesc& desc);
		Ref<IBuffer> createBufferImpl(const BufferDesc& desc);
		Ref<IBuffer> createBufferImpl(const BufferDesc& desc, const void* data, size_t dataSize);
		Ref<IShader> createShaderImpl(const ShaderCreateInfo& shaderCI, const uint32_t* pCode, size_t codeSize);
		Ref<ISampler> createSamplerImpl(const SamplerDesc& desc);

		void setSwapChainImageAvailableSeamaphore(const VkSemaphore& semaphore);
		void setRenderCompleteSemaphore(const VkSemaphore& semaphore);
		void recycleCommandBuffers();
		void executePresentCommandList(ICommandList* cmdList);

		DeviceCreateInfo createInfo{};
		VkPhysicalDeviceProperties physicalDeviceProperties{};
		uint32_t maxPushDescriptors = 0;

		// Interface implementation


		void createSwapChain(const SwapChainCreateInfo& swapChainCI) override;
		void recreateSwapChain() override;
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
		Device() = default;
		TextureVk* createRenderTarget(const TextureDesc& desc, VkImage image);
		bool createInstance(bool enableDebugRuntime);
		bool pickPhysicalDevice();
		bool createDevice(const DeviceCreateInfo& desc);

		void destroyDebugUtilsMessenger();
#if defined RHI_ENABLE_THREAD_RECORDING
		std::mutex m_Mutex;
#endif

		ContextVk m_Context = {};
		VmaAllocator m_Allocator = VK_NULL_HANDLE;

		VkDebugUtilsMessengerEXT m_DebugUtilsMessenger = VK_NULL_HANDLE;

		VkSemaphore m_SwapChainImgAvailableSemaphore = VK_NULL_HANDLE;

		VkSemaphore m_RenderCompleteSemaphore = VK_NULL_HANDLE;

		std::array<std::unique_ptr<CommandQueue>, static_cast<uint32_t>(QueueType::Count)> m_Queues;

		std::vector<VkCommandBufferSubmitInfo> m_CmdBufSubmitInfos;
	};
}

