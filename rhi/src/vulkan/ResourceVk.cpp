#include "rhi/common/Error.h"
#include "vk_resource.h"

#include <array>
#include <unordered_map>
#include "vk_errors.h"

namespace rhi::vulkan
{
	// texture 


	TextureCopy getTextureCopyInfo(Format format, const Region3D& region, uint32_t  optimalBufferCopyRowPitchAlignment)
	{
		assert(region.isValid());

		TextureCopy copyInfo;

		FormatInfo formatInfo = GetFormatInfo(format);

		uint32_t regionWidth = region.getWidth();
		uint32_t regionHeight = region.getHeight();
		uint32_t regionDepth = region.getDepth();

		if (formatInfo.IsCompressed())
		{
			uint32_t blockAlignedRegionWidth = AlignUp(regionWidth, formatInfo.blockSize);
			uint32_t blockAlignedRegionHeight = AlignUp(regionHeight, formatInfo.blockSize);

			copyInfo.rowBytesCount = blockAlignedRegionWidth / formatInfo.blockSize * formatInfo.bytesPerBlock;
			copyInfo.rowCount = blockAlignedRegionHeight / formatInfo.blockSize;
		}
		else
		{
			copyInfo.rowBytesCount = regionWidth * formatInfo.bytesPerBlock;
			copyInfo.rowCount = regionHeight;
		}

		copyInfo.rowStride = static_cast<uint32_t>(AlignUp(copyInfo.rowBytesCount, optimalBufferCopyRowPitchAlignment));
		copyInfo.depthStride = copyInfo.rowCount * copyInfo.rowStride;
		copyInfo.regionBytesCount = regionDepth * copyInfo.depthStride;
		return copyInfo;
	}

	TextureVk::~TextureVk()
	{
		if (managed && image)
		{
			vmaDestroyImage(mMemoryAllocator, image, allocation);
		}

		if (m_DefaultView)
		{
			delete m_DefaultView;
			m_DefaultView = nullptr; 
		}
	}

	ITextureView* TextureVk::createView(TextureViewDesc desc)
	{
		auto textureView = new TextureViewVk(m_Context, *this);

		VkImageViewCreateInfo viewCreateInfo{};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.viewType = GetVkImageViewType(desc.dimension);
		viewCreateInfo.format = format;
		viewCreateInfo.image = image;
		viewCreateInfo.subresourceRange.aspectMask = GetVkAspectMask(format);
		viewCreateInfo.subresourceRange.baseArrayLayer = desc.baseArrayLayer;
		viewCreateInfo.subresourceRange.layerCount = desc.arrayLayerCount;
		viewCreateInfo.subresourceRange.baseMipLevel = desc.baseMipLevel;
		viewCreateInfo.subresourceRange.levelCount = desc.mipLevelCount;
		VkResult err = vkCreateImageView(m_Context.device, &viewCreateInfo, nullptr, &textureView->imageView);
		CHECK_VK_RESULT(err, "Could not create TextureView");
		if (err != VK_SUCCESS)
		{
			delete textureView;
		}
		return textureView;
	}

	void TextureVk::createDefaultView()
	{
		assert(this->m_DefaultView == nullptr);
		TextureViewDesc desc{};
		desc.dimension = this->m_Desc.dimension;
		desc.baseArrayLayer = 0;
		desc.arrayLayerCount = this->m_Desc.arraySize;
		desc.baseMipLevel = 0;
		desc.mipLevelCount = this->m_Desc.mipLevelCount;
		this->m_DefaultView = checked_cast<TextureViewVk*>(createView(desc));
	}

	Object TextureVk::getNativeObject(NativeObjectType type) const
	{
		switch (type)
		{
		case NativeObjectType::VK_Image:
			return static_cast<Object>(image);
		case NativeObjectType::VK_ImageView:
			return static_cast<Object>(m_DefaultView);
		default:
			return nullptr;
		}
	}

	TextureViewVk::~TextureViewVk()
	{
		vkDestroyImageView(m_Context.device, imageView, nullptr);
	}

	Object TextureViewVk::getNativeObject(NativeObjectType type) const
	{
		if (type == NativeObjectType::VK_ImageView)
		{
			return static_cast<Object>(imageView);
		}
		return nullptr;
	}

	// buffer

	Buffer::~Buffer()
	{
		assert(buffer != VK_NULL_HANDLE);
		vmaDestroyBuffer(mMemoryAllocator, buffer, allocation);
	}

	Object Buffer::getNativeObject(NativeObjectType type) const
	{
		if (type == NativeObjectType::VK_Buffer)
		{
			return static_cast<Object>(buffer);
		}
		return nullptr;
	}

	// sampler

	SamplerVk::~SamplerVk()
	{
		vkDestroySampler(m_Context.device, sampler, nullptr);
	}

	// resourece state

	static VkAccessFlags2 resourceStateToVkAccessFlags2(ResourceState state)
	{
		// clang-format off
		switch (state)
		{
		case ResourceState::Undefined:			return VK_ACCESS_2_NONE;
		case ResourceState::Common:				return VK_ACCESS_2_NONE;
		case ResourceState::VertexBuffer:		return VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
		case ResourceState::IndexBuffer:		return VK_ACCESS_2_INDEX_READ_BIT;
		case ResourceState::ConstantBuffer:	    return VK_ACCESS_2_UNIFORM_READ_BIT;
		case ResourceState::ShaderResource:		return VK_ACCESS_2_SHADER_READ_BIT;
		case ResourceState::UnorderedAccess:	return VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
		case ResourceState::RenderTarget:		return VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		case ResourceState::IndirectBuffer:		return VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
		case ResourceState::Present:			return VK_ACCESS_2_NONE;
		case ResourceState::DepthRead:			return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		case ResourceState::DepthWrite:			return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		case ResourceState::CopySource:			return VK_ACCESS_2_TRANSFER_READ_BIT;
		case ResourceState::CopyDst:		    return VK_ACCESS_2_TRANSFER_WRITE_BIT;
		case ResourceState::ResolveSource:      return VK_ACCESS_2_TRANSFER_READ_BIT;
		case ResourceState::ResolveDest:	    return VK_ACCESS_2_TRANSFER_WRITE_BIT;
		case ResourceState::InitialRenderTarget:return VK_ACCESS_2_NONE;
		default:
			LOG_ERROR("Unexpected resource state");
			return 0;
		}
		// clang-format on
	}

	static VkPipelineStageFlags2 resourceStateToVkPipelineStageFlags2(ResourceState state)
	{
		constexpr VkPipelineStageFlags2 allShaderStages = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT |
			VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		// clang-format off
		switch (state)
		{
		case ResourceState::Undefined:			return VK_PIPELINE_STAGE_2_NONE;
		case ResourceState::Common:				return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		case ResourceState::VertexBuffer:		return VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
		case ResourceState::IndexBuffer:		return VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
		case ResourceState::ConstantBuffer:	    return allShaderStages;
		case ResourceState::ShaderResource:		return allShaderStages;
		case ResourceState::UnorderedAccess:	return allShaderStages;
		case ResourceState::RenderTarget:		return VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		case ResourceState::IndirectBuffer:		return VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
		case ResourceState::Present:			return VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
		case ResourceState::DepthRead:			return VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		case ResourceState::DepthWrite:			return VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		case ResourceState::CopySource:			return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		case ResourceState::CopyDst:		    return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		case ResourceState::ResolveSource:      return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		case ResourceState::ResolveDest:	    return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		case ResourceState::InitialRenderTarget:return VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		default:
			LOG_ERROR("Unexpected resource state");
			return 0;
		}
		// clang-format on
	}

	VkAccessFlags2 resourceStatesToVkAccessFlags2(ResourceState states)
	{
		const uint32_t numStateBits = 32;
		uint32_t stateTmp = uint32_t(states);
		uint32_t bitIndex = 0;

		VkAccessFlags2 accessflags = 0;
		while (stateTmp != 0 && bitIndex < numStateBits)
		{
			uint32_t bit = (1 << bitIndex);
			if (bit & stateTmp)
			{
				accessflags |= resourceStateToVkAccessFlags2(ResourceState(bit));
				stateTmp &= ~bit;
			}
			bitIndex++;
		}

		return accessflags;
	}

	VkPipelineStageFlags2 resourceStatesToVkPipelineStageFlags2(ResourceState states)
	{
		const uint32_t numStateBits = 32;
		uint32_t stateTmp = uint32_t(states);
		uint32_t bitIndex = 0;

		VkPipelineStageFlags2 pipelineStages = 0;
		while (stateTmp != 0 && bitIndex < numStateBits)
		{
			uint32_t bit = (1 << bitIndex);
			if (bit & stateTmp)
			{
				pipelineStages |= resourceStateToVkPipelineStageFlags2(ResourceState(bit));
				stateTmp &= ~bit;
			}
			bitIndex++;
		}

		return pipelineStages;
	}

	VkImageLayout resourceStateToVkImageLayout(ResourceState state)
	{
		// clang-format off
		switch (state)
		{
		case ResourceState::Undefined:			return VK_IMAGE_LAYOUT_UNDEFINED;
		case ResourceState::Common:				return VK_IMAGE_LAYOUT_GENERAL;
		case ResourceState::ShaderResource:		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case ResourceState::UnorderedAccess:	return VK_IMAGE_LAYOUT_GENERAL;
		case ResourceState::RenderTarget:		return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case ResourceState::Present:			return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		case ResourceState::DepthRead:			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		case ResourceState::DepthWrite:			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case ResourceState::CopySource:			return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		case ResourceState::CopyDst:		    return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		case ResourceState::ResolveSource:      return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		case ResourceState::ResolveDest:	    return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		case ResourceState::InitialRenderTarget:return VK_IMAGE_LAYOUT_UNDEFINED;
		default:
			LOG_ERROR("Unexpected resource state");
			return VK_IMAGE_LAYOUT_UNDEFINED;
		}
		// clang-format on
	}

	Object ResourceSetVk::getNativeObject(NativeObjectType type) const
	{
		// now we use push descriptor, no longer exists VkDescriptorSet.
		return nullptr;
	}

	Object ResourceSetLayoutVk::getNativeObject(NativeObjectType type) const
	{
		if (type == NativeObjectType::VK_DescriptorSetLayout)
		{
			return static_cast<Object>(descriptorSetLayout);
		}
		return nullptr;
	}

	ResourceSetLayoutVk::~ResourceSetLayoutVk()
	{
		assert(descriptorSetLayout != VK_NULL_HANDLE);
		vkDestroyDescriptorSetLayout(m_Context.device, descriptorSetLayout, nullptr);
	}

	ShaderVk::~ShaderVk()
	{
		specializationConstants.clear();
		assert(shaderModule != VK_NULL_HANDLE);
		vkDestroyShaderModule(m_Context.device, shaderModule, nullptr);
	}

	VkShaderStageFlagBits shaderTypeToVkShaderStageFlagBits(ShaderStage shaderType)
	{
		assert(shaderType != ShaderStage::None);

		uint32_t result = 0;
		// clang-format off
		if ((shaderType & ShaderStage::Compute) != 0)                result |= uint32_t(VK_SHADER_STAGE_COMPUTE_BIT);
		if ((shaderType & ShaderStage::Vertex) != 0)                 result |= uint32_t(VK_SHADER_STAGE_VERTEX_BIT);
		if ((shaderType & ShaderStage::TessellationControl) != 0)    result |= uint32_t(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
		if ((shaderType & ShaderStage::TessellationEvaluation) != 0) result |= uint32_t(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
		if ((shaderType & ShaderStage::Geometry) != 0)               result |= uint32_t(VK_SHADER_STAGE_GEOMETRY_BIT);
		if ((shaderType & ShaderStage::Fragment) != 0)               result |= uint32_t(VK_SHADER_STAGE_FRAGMENT_BIT);
		if ((shaderType & ShaderStage::Mesh) != 0)                   result |= uint32_t(VK_SHADER_STAGE_MESH_BIT_EXT);
		if ((shaderType & ShaderStage::Task) != 0)                   result |= uint32_t(VK_SHADER_STAGE_TASK_BIT_EXT);
		// clang-format on
		return VkShaderStageFlagBits(result);
	}

	VkDescriptorType shaderResourceTypeToVkDescriptorType(BindingType type)
	{
		VkDescriptorType descriptorType{};
		switch (type)
		{
		case BindingType::CombinedTextureSampler:
			descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			break;
		case BindingType::Sampler:
			descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			break;
		case BindingType::UniformBuffer:
			descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			break;
		case BindingType::StorageBuffer:
			descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			break;
		case BindingType::SampledTexture:
			descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			break;
		case BindingType::StorageTexture:
			descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			break;
		case BindingType::UniformTexelBuffer:
			descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
			break;
		case BindingType::StorageTexelBuffer:
			descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
			break;
		case BindingType::None:
		default:
			assert(!"unknown ShaderResourceType");
			break;
		}
		return descriptorType;
	}

	VkSamplerAddressMode convertVkSamplerAddressMode(SamplerAddressMode mode)
	{
		switch (mode)
		{
		case SamplerAddressMode::ClampToEdge:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

		case SamplerAddressMode::Repeat:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;

		case SamplerAddressMode::ClampToBorder:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

		case SamplerAddressMode::MirroredRepeat:
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

		case SamplerAddressMode::MirrorClampToEdge:
			return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;

		default:
			assert(!"Invalid SamplerAddressMode");
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}
	}

	VkBorderColor convertVkBorderColor(BorderColor color)
	{
		switch (color)
		{
		case rhi::BorderColor::FloatOpaqueBlack:
			return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		case rhi::BorderColor::FloatOpaqueWhite:
			return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		case rhi::BorderColor::FloatTransparentBlack:
			return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		default:
			assert(!"Invalid BorderColor");
			return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		}
	}

	VkClearColorValue convertVkClearColor(ClearColor color, Format textureFormat)
	{
		VkClearColorValue val{};
		const FormatInfo& formatInfo = GetFormatInfo(textureFormat);

		if (formatInfo.componentType == FormatComponentType::Float)
		{
			for (int i = 0; i < 4; ++i)
			{
				val.float32[i] = color.float32[i];
			}
		}
		else if (formatInfo.componentType == FormatComponentType::Uint)
		{
			for (int i = 0; i < 4; ++i)
			{
				val.uint32[i] = color.uint32[i];
			}
		}
		else 
		{
			for (int i = 0; i < 4; ++i)
			{
				val.int32[i] = color.int32[i];
			}
		}

		return val;
	}
}