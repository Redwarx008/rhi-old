#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>

#include "rhi/RHI.h"
#include "../common/Ref.hpp"
#include "../TextureBase.h"
#include "../Subresource.h"
#include "../SubresourceStorage.hpp"
#include "../SyncScopeUsageTracker.h"

namespace rhi::vulkan
{
	class Device;
	class Queue;
	class TextureView;

	class Texture final : public TextureBase
	{
	public:
		static Ref<Texture> Create(Device* device, const TextureDesc& desc);
		// interface 
		TextureViewBase* CreateView(const TextureViewDesc& desc) override;

		// internal 
		VkImage GetHandle() const;
		//void TransitionUsageNow(TextureUsage usage,
		//	ShaderStage shaderStages,
		//	const SubresourceRange& range,
		//	std::vector<VkImageMemoryBarrier>& barriers);
		void TransitionOwnership(Queue* queue, const SubresourceRange& range, Queue* recevingQueue);
		void TransitionUsageAndGetResourceBarrier(Queue* queue,
			TextureUsage usage,
			ShaderStage shaderStages,
			const SubresourceRange& range);
		void TransitionUsageForMultiRange(Queue* queue, const SubresourceStorage<TextureSyncInfo>& syncInfos);

	private:
		explicit Texture(Device* device, const TextureDesc& desc) noexcept;
		bool Initialize();
		void DestroyImpl() override;

		VkImage mHandle = VK_NULL_HANDLE;
		VmaAllocation mAllocation = VK_NULL_HANDLE;
		VkFormat mVkFormat; // we will get it in the hot path, so cache it.
		SubresourceStorage<TextureSyncInfo> mSubresourceLastSyncInfos;

		friend class TextureView;
	};

	class TextureView final : public TextureViewBase
	{
	public:
		static Ref<TextureView> Create(TextureBase* texture, const TextureViewDesc& desc);
		// internal 
		VkImageView GetHandle() const;
	private:
		explicit TextureView(TextureBase* texture, const TextureViewDesc& desc);
		bool Initialize();
		void DestroyImpl() override;

		VkImageView mHandle = VK_NULL_HANDLE;
	};

	VkFormat GetVkFormat(TextureFormat format);

	TextureFormat GetFormat(VkFormat format);

	VkImageUsageFlags GetVkImageUsageFlags(TextureUsage usage, TextureFormat format);

	VkSampleCountFlagBits SampleCountConvert(uint32_t sampleCount);

	VkImageType GetVkImageType(TextureDimension dimension);

	VkImageViewType GetVkImageViewType(TextureDimension dimension);

	VkImageAspectFlags GetVkAspectMask(VkFormat format);

	VkImageLayout ImageLayoutConvert(TextureUsage usage, TextureFormat format);
}