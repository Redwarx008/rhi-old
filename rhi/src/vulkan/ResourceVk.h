#pragma once

#include "rhi/rhi.h"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <vector>

namespace rhi::vulkan
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

	enum class FormatComponentType : uint8_t
	{
		Undefined,
		Float,
		Snorm,
		Unorm,
		UnormSrgb,
		Sint,
		Uint,
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
		FormatComponentType componentType;
		bool isCompressed() const { return blockSize != 1; }
	};


	class TextureVk final : public ITexture, public MemoryResource
	{
	public:
		explicit TextureVk(TextureDesc desc, const ContextVk& context, const VmaAllocator& allocator)
			:m_Desc(desc),
			m_Context(context),
			m_Allocator(allocator) {}

		ITextureView* getDefaultView() const override { return m_DefaultView; }
		ITextureView* createView(TextureViewDesc desc) override;

		~TextureVk();
		
		void createDefaultView();
		ResourceState submittedState = ResourceState::Undefined;
		VkImage image = VK_NULL_HANDLE;
		VkFormat format = VK_FORMAT_UNDEFINED;
	private:
		TextureVk() = default;
		TextureDesc m_Desc;
		const ContextVk& m_Context;
		const VmaAllocator& m_Allocator;
		ResourceState m_State = ResourceState::Undefined;
		ITextureView* m_DefaultView = nullptr;
	};

	class TextureViewVk final : public ITextureView
	{
	public:
		explicit TextureViewVk(TextureViewDesc desc, const ContextVk& context, TextureVk& texture)
			:m_Desc(desc),
			m_Context(context),
			m_Texture(texture){}
		const TextureViewDesc& getDesc() const override { return m_Desc; }
		ITexture* getTexture() const override { return &m_Texture; }
		Object getNativeObject(NativeObjectType type) const override;
		~TextureViewVk();

		VkImageView imageView = VK_NULL_HANDLE;
	private:
		TextureViewVk() = default;
		TextureViewDesc m_Desc;
		const ContextVk& m_Context;
		TextureVk& m_Texture;
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
		explicit SamplerVk(SamplerDesc desc, const ContextVk& context)
			:m_Desc(desc),
			m_Context(context) {}
		~SamplerVk();
		const SamplerDesc& getDesc() const override { return m_Desc; }
		VkSampler sampler = VK_NULL_HANDLE;
	private:
		SamplerDesc m_Desc;
		const ContextVk& m_Context;
	};



	// shader

	class ShaderVk final : public IShader
	{
	public:
		explicit ShaderVk(ShaderDesc desc)
			:m_Desc(desc) {}
		~ShaderVk();
		const ShaderDesc& getDesc() const override { return m_Desc; }
		VkShaderModule shaderModule;
		std::vector<uint32_t> spirv;
		std::vector<SpecializationConstant> specializationConstants;
	private:
		ShaderDesc m_Desc;
		const ContextVk& m_Context;
	};

	// shaderResourceBinding

	class ShaderResourceBinding final : public IShaderResourceBinding
	{
	public:
		explicit ShaderResourceBinding();
		~ShaderResourceBinding();
		void bindBuffer(IBuffer* buffer, uint32_t slot, uint32_t set = 0, uint64_t offset = 0, uint64_t range = ~0ull) override;
		void bindTexture(ITextureView* textureView, uint32_t slot, uint32_t set = 0) override;
		void bindSampler(ISampler* sampler, uint32_t slot, uint32_t set = 0) override;
		void bindTextureWithSampler(ITextureView* textrueView, ISampler* sampler, uint32_t slot, uint32_t set = 0) override;

	private:
		std::array<std::vector<VkWriteDescriptorSet>, 4> m_DescriptorSets;
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

	VkClearColorValue convertVkClearColor(ClearColor color, Format textureFormat);
}
