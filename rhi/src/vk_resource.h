#pragma once

#include "rhi/rhi.h"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <vector>

namespace rhi
{
	class MemoryResource
	{
	public:
		bool managed = true;
		VmaAllocation allocation = nullptr;
	};

	struct ContextVk
	{
		VkInstance instace{ VK_NULL_HANDLE };
		VkPhysicalDevice physicalDevice{ VK_NULL_HANDLE };
		VkDevice device{ VK_NULL_HANDLE };
	};

	struct FormatInfo
	{
		Format format;
		const char* name;
		union
		{
			uint8_t bytesPerBlock;
			uint8_t bytes;
		};
		uint8_t blockSize;
		bool hasRed : 1;
		bool hasGreen : 1;
		bool hasBlue : 1;
		bool hasAlpha : 1;
		bool hasDepth : 1;
		bool hasStencil : 1;

		bool isCompressed() const { return blockSize != 1; }
	};

	class TextureVk final : public ITexture, public MemoryResource
	{
	public:
		explicit TextureVk(const ContextVk& context, const VmaAllocator& allocator)
			:m_Context(context),
			m_Allocator(allocator) {}
		void setState(ResourceState state) { m_State = state; }
		ResourceState getState() const override { return m_State; }
		const TextureDesc& getDesc() const override { return desc; }
		Object getNativeObject(NativeObjectType type) const override;
		~TextureVk();
		
		ResourceState submittedState = ResourceState::Undefined;
		TextureDesc desc;
		VkImage image = VK_NULL_HANDLE;
		VkImageView	view = VK_NULL_HANDLE;
		VkFormat format = VK_FORMAT_UNDEFINED;
	private:
		TextureVk() = default;
		const ContextVk& m_Context;
		const VmaAllocator& m_Allocator;
		ResourceState m_State = ResourceState::Undefined;
	};

	struct TextureCopyInfo
	{
		//The number of rows in the region. 
		//For compressed formats, this is the number of compressed-block rows.
		uint32_t rowCount = 0;
		uint64_t rowBytesCount = 0;
		// apply alignment to rowBytesCount.
		uint32_t rowStride = 0;
		// rowCount * rowStride.
		uint32_t depthStride = 0;

		uint64_t regionBytesCount = 0;
	};

	TextureCopyInfo getTextureCopyInfo(Format format, const Region3D& region, uint32_t  optimalBufferCopyRowPitchAlignment);

	class SamplerVk : public ISampler
	{
	public:
		explicit SamplerVk(const ContextVk& context)
			:m_Context(context) {}
		~SamplerVk();
		const SamplerDesc& getDesc() const override { return desc; }
		SamplerDesc desc;
		VkSampler sampler = nullptr;
	private:
		const ContextVk& m_Context;
	};

	class BufferVk : public IBuffer, public MemoryResource
	{
	public:
		explicit BufferVk(const ContextVk& context, const VmaAllocator& allocator)
			:m_Context(context),
			m_Allocator(allocator)
		{}
		void setState(ResourceState state) { m_State = state; }
		ResourceState getState() const override { return m_State; }
		const BufferDesc& getDesc() const override { return desc; }
		Object getNativeObject(NativeObjectType type) const override;
		~BufferVk();

		//ResourceState submittedState = ResourceState::Undefined;
		BufferDesc desc;
		VkBuffer buffer = nullptr;
		VmaAllocationInfo allocaionInfo{};
	private:
		const ContextVk& m_Context;
		const VmaAllocator& m_Allocator;
		ResourceState m_State = ResourceState::Undefined;
	};

	// shader

	class ShaderVk final : public IShader
	{
	public:
		explicit ShaderVk(const ContextVk& context, ShaderDesc desc)
			:m_Context(context),
			m_Desc(desc) {}
		~ShaderVk();
		Object getNativeObject(NativeObjectType type) const override;
		const ShaderDesc& getDesc() const override { return m_Desc; }
		VkShaderModule shaderModule;
		std::vector<SpecializationConstant> specializationConstants;
	private:
		const ContextVk& m_Context;
		ShaderDesc m_Desc;
	};

	// resource set

	class ResourceSetLayoutVk final : public IResourceSetLayout
	{
	public:
		explicit ResourceSetLayoutVk(const ContextVk& context)
			:m_Context(context) {}
		~ResourceSetLayoutVk();
		Object getNativeObject(NativeObjectType type) const override;
		VkDescriptorSetLayout descriptorSetLayout = nullptr;
		std::vector<ResourceSetLayoutItem> resourceSetLayoutItems;
	private:
		const ContextVk& m_Context;
	};

	struct ResourceSetItemWithVisibleStages
	{
		ResourceSetItem item;
		ShaderType visibleStages;
	};

	class ResourceSetVk final : public IResourceSet
	{
	public:
		explicit ResourceSetVk(const ContextVk& context)
			:m_Context(context) {}
		~ResourceSetVk();
		Object getNativeObject(NativeObjectType type) const override;
		VkDescriptorPool descriptorPool = nullptr;
		VkDescriptorSet descriptorSet = nullptr;
		const ResourceSetLayoutVk* resourceSetLayout = nullptr;
		std::vector<ResourceSetItemWithVisibleStages> resourcesNeedStateTransition;
	private:
		const ContextVk& m_Context;
	};

	const FormatInfo& getFormatInfo(Format format);

	VkImageUsageFlags getVkImageUsageFlags(const TextureDesc& desc);

	VkImageCreateFlags getVkImageCreateFlags(TextureDimension dimension);

	VkSampleCountFlagBits getVkImageSampleCount(const TextureDesc& desc);

	VkImageType getVkImageType(TextureDimension dimension);

	VkImageViewType getVkImageViewType(TextureDimension dimension);

	VkImageAspectFlags getVkAspectMask(VkFormat format);

	VkFormat formatToVkFormat(Format format);

	Format vkFormatToFormat(VkFormat format);


	VkAccessFlags2 resourceStatesToVkAccessFlags2(ResourceState states);

	VkPipelineStageFlags2 resourceStatesToVkPipelineStageFlags2(ResourceState states);

	VkImageLayout resourceStateToVkImageLayout(ResourceState state);


	VkShaderStageFlagBits shaderTypeToVkShaderStageFlagBits(ShaderType type);

	VkDescriptorType shaderResourceTypeToVkDescriptorType(ShaderResourceType type);


	VkSamplerAddressMode convertVkSamplerAddressMode(SamplerAddressMode mode);
	VkBorderColor convertVkBorderColor(BorderColor color);
}
