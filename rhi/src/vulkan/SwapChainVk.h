#pragma once

#include "../SwapchainBase.h"

#include <vector>
#include <memory>
#include <vulkan/vulkan.h>

namespace rhi::vulkan
{
	class Device;
	class SwapChainTexture;
	class SwapChain final : public SwapChainBase
	{
	public:
		static Ref<SwapChain> Create(Device* device, Surface* surface, SwapChainBase* previous, const SurfaceConfiguration& config);
		~SwapChain();
		VkSwapchainKHR GetHandle() const;
		SurfaceAcquireNextTextureStatus AcquireNextTexture() override;
		Ref<TextureBase> GetCurrentTexture() override;
		void Present() override;
		void DestroySwapChain();
	private:
		SwapChain(Device* device, Surface* surface, const SurfaceConfiguration& config);
		bool Initialize(SwapChainBase* previous);
		VkSurfaceKHR CreateSurface(Surface* surface);
		void CreateSwapChainInternal(SwapChainBase* previous);
		SurfaceAcquireNextTextureStatus AcquireNextTextureImpl(bool isReentrant);

		VkSurfaceKHR mVkSurface = VK_NULL_HANDLE;
		VkSwapchainKHR mHandle = VK_NULL_HANDLE;

		struct SemaphoreAndFence
		{
			VkSemaphore semaphore;
			VkFence fence;
		};

		std::vector<SemaphoreAndFence> mAquireImageSemaphoreAndFences;

		uint32_t mCurrentFrameIndex = 0;

		struct PerTexture
		{
			Ref<SwapChainTexture> texture;
			VkSemaphore renderingDoneSemaphore;
		};

		std::vector<PerTexture> mTextures;

		uint32_t mImageIndex = UINT32_MAX;
	};
}