#pragma once

#include "rhi/rhi.h"
#if defined RHI_ENABLE_THREAD_RECORDING
#include <mutex>
#endif
#include <vk_mem_alloc.h>
#include "vk_resource.h"

#include <array>

namespace rhi
{
	class CommandBuffer;
	class CommandQueue;

	class RenderDeviceVk final : public IRenderDevice
	{
	public:
		~RenderDeviceVk();
		// Internal methodsd
		static RenderDeviceVk* create(const RenderDeviceCreateInfo& desc);

		void setSwapChainImageAvailableSeamaphore(const VkSemaphore& semaphore);
		void setRenderCompleteSemaphore(const VkSemaphore& semaphore);
		void recycleCommandBuffers();
		void executePresentCommandList(ICommandList* cmdList);

		RenderDeviceCreateInfo createInfo{};
		VkPhysicalDeviceProperties physicalDeviceProperties{};
		uint32_t maxPushDescriptors = 0;

		// Interface implementation

		ITextureView* getCurrentRenderTargetView() override;
		ITextureView* getDepthStencilView() override;
		Format getRenderTargetFormat() override { return m_SwapChainFormat; }
		Format getDepthStencilFormat() override { return m_DepthStencilFormat; }

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
		RenderDeviceVk() = default;
		TextureVk* createRenderTarget(const TextureDesc& desc, VkImage image);
		bool createInstance(bool enableDebugRuntime);
		bool pickPhysicalDevice();
		bool createDevice(const RenderDeviceCreateInfo& desc);
		void createSurface(void* platformWindow);
		void createSwapChainInternal();
		void destroySwapChain();
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

		uint32_t m_SwapChainImageWidth = 0;
		uint32_t m_SwapChainImageHeight = 0;

		Format m_SwapChainFormat = Format::UNKNOWN;
		Format m_DepthStencilFormat = Format::UNKNOWN;

		bool m_VSyncEnabled = false;

		VkSurfaceKHR m_WindowSurface = VK_NULL_HANDLE;
		VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;

		uint32_t m_CurrentFrameInFlight = 0;
		uint32_t m_SwapChainImageIndex = UINT32_MAX;

		std::queue<uint64_t> m_LastSubmittedIDPerFrame;

		std::array<VkSemaphore, g_MaxConcurrentFrames + 1> m_ImageAvailableSemaphores{};
		std::array<VkSemaphore, g_MaxConcurrentFrames + 1> m_RenderCompleteSemaphores{};

		std::vector<std::unique_ptr<TextureVk>> m_ColorAttachments;
		std::unique_ptr<TextureVk> m_DepthStencilAttachments;
	};
}

