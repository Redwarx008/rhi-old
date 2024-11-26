#include "vk_render_device.h"

#include "vk_command_list.h"
#include "vk_errors.h"
#include "vk_pipeline.h"
#include "vk_resource.h"

#include <string>
#include <sstream>
#include <memory>
#include <unordered_map>
#include <algorithm>

namespace rhi
{
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessageCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		//RenderDeviceVk* rd = reinterpret_cast<RenderDeviceVk*>(pUserData);

		MessageSeverity serverity = MessageSeverity::Info;

		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
			serverity = MessageSeverity::Verbose;
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
			serverity = MessageSeverity::Info;
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			serverity = MessageSeverity::Warning;
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
			serverity = MessageSeverity::Error;
		}

		std::stringstream debugMessage;
		if (pCallbackData->pMessageIdName) {
			debugMessage << "[" << pCallbackData->pMessageIdName << "] : " << pCallbackData->pMessage;
		}

		if (g_DebugMessageCallback)
		{
			g_DebugMessageCallback(serverity, debugMessage.str().c_str());
		}
		else
		{
			std::cerr << debugMessage.str().c_str() << std::endl;
		}
		// The return value of this callback controls whether the Vulkan call that caused the validation message will be aborted or not
		// We return VK_FALSE as we DON'T want Vulkan calls that cause a validation message to abort
		// If you instead want to have calls abort, pass in VK_TRUE and the function will return VK_ERROR_VALIDATION_FAILED_EXT
		return VK_FALSE;
	}

	bool RenderDeviceVk::createInstance(bool enableDebugRuntime)
	{
		std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };


		instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
		instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

		std::vector<std::string> supportedInstanceExtensions;
		// Get extensions supported by the instance and store for later use
		uint32_t extCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
		if (extCount > 0)
		{
			std::vector<VkExtensionProperties> extensions(extCount);
			if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
			{
				for (VkExtensionProperties& extension : extensions)
				{
					supportedInstanceExtensions.push_back(extension.extensionName);
				}
			}
		}

		for (auto extensionName : instanceExtensions)
		{
			if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), extensionName) == supportedInstanceExtensions.end())
			{
				LOG_ERROR(extensionName, " is not supported.");
				return false;
			}
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "rhi";
		appInfo.apiVersion = VK_API_VERSION_1_3;

		VkInstanceCreateInfo instanceCreateInfo{};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pApplicationInfo = &appInfo;

		if (enableDebugRuntime) {
			VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI{};
			debugUtilsMessengerCI.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debugUtilsMessengerCI.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debugUtilsMessengerCI.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
			debugUtilsMessengerCI.pfnUserCallback = DebugMessageCallback;
			debugUtilsMessengerCI.pUserData = this;
			instanceCreateInfo.pNext = &debugUtilsMessengerCI;

			instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

			// The VK_LAYER_KHRONOS_validation contains all current validation functionality.
			// Note that on Android this layer requires at least NDK r20
			const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
			// Check if this layer is available at instance level
			uint32_t instanceLayerCount;
			vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
			std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
			vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
			bool validationLayerPresent = false;
			for (VkLayerProperties& layer : instanceLayerProperties) {
				if (strcmp(layer.layerName, validationLayerName) == 0) {
					validationLayerPresent = true;
					break;
				}
			}
			if (validationLayerPresent) {
				instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
				instanceCreateInfo.enabledLayerCount = 1;
			}
			else {
				LOG_ERROR("Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled");
			}
		}

		instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
		instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

		VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &context.instace);
		if (result != VK_SUCCESS)
		{
			LOG_ERROR("Failed to create a Vulkan instance");
			return false;
		}
		return true;
	}


	bool RenderDeviceVk::pickPhysicalDevice()
	{
		assert(context.instace != VK_NULL_HANDLE);

		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(context.instace, &deviceCount, nullptr);

		if (deviceCount == 0)
		{
			LOG_ERROR("No device with Vulkan support found");
			return false;
		}

		std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
		vkEnumeratePhysicalDevices(context.instace, &deviceCount, physicalDevices.data());

		// pick the first discrete GPU if it exists, otherwise the first integrated GPU
		for (const VkPhysicalDevice& physicalDevice : physicalDevices)
		{
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

			if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				context.physicalDevice = physicalDevice;
				physicalDeviceProperties = deviceProperties;
				return true;
			}
		}

		context.physicalDevice = physicalDevices[0];
		vkGetPhysicalDeviceProperties(context.physicalDevice, &physicalDeviceProperties);
		return true;
	}

	bool RenderDeviceVk::createDevice(const RenderDeviceCreateInfo& desc)
	{
		assert(context.physicalDevice != VK_NULL_HANDLE);
		// find queue family
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(context.physicalDevice, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilyPropertieses(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(context.physicalDevice, &queueFamilyCount, queueFamilyPropertieses.data());
		for (uint32_t i = 0; i < queueFamilyPropertieses.size(); i++)
		{
			const auto& queueFamilyProps = queueFamilyPropertieses[i];

			if (queueFamilyProps.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT))
			{
				queueFamilyIndex = i;
				break;
			}
		}

		float priority = 1.f;
		VkDeviceQueueCreateInfo queueCI{};
		queueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCI.queueFamilyIndex = queueFamilyIndex;;
		queueCI.queueCount = 1;
		queueCI.pQueuePriorities = &priority;

		// Create the logical device
		std::vector<const char*> deviceExtensions;

		deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		deviceExtensions.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);

		// Get list of supported extensions
		std::vector<std::string> supportedExtensions;
		uint32_t extCount = 0;
		vkEnumerateDeviceExtensionProperties(context.physicalDevice, nullptr, &extCount, nullptr);
		if (extCount > 0)
		{
			std::vector<VkExtensionProperties> extensions(extCount);
			if (vkEnumerateDeviceExtensionProperties(context.physicalDevice, nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
			{
				for (auto& ext : extensions)
				{
					supportedExtensions.push_back(ext.extensionName);
				}
			}
		}

		for (auto extension : deviceExtensions)
		{
			if (std::find(supportedExtensions.begin(), supportedExtensions.end(), extension) == supportedExtensions.end())
			{
				LOG_ERROR(extension, "is not supported.");
				return false;
			}
		}

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.textureCompressionBC = true;
		deviceFeatures.geometryShader = true;
		deviceFeatures.tessellationShader = true;
		deviceFeatures.multiViewport = desc.enableMultiViewport;
		deviceFeatures.depthBiasClamp = desc.enableDepthBiasClamp;
		deviceFeatures.depthClamp = desc.enableDepthClamp;
		deviceFeatures.samplerAnisotropy = desc.enableSamplerAnisotropy;
		deviceFeatures.sampleRateShading = desc.enableSampleRateShading;

		VkPhysicalDeviceVulkan13Features feature13{};
		feature13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		feature13.synchronization2 = true;
		feature13.dynamicRendering = true;

		VkPhysicalDeviceVulkan12Features feature12{};
		feature12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		feature12.timelineSemaphore = true;
		feature12.uniformBufferStandardLayout = true;
		feature12.scalarBlockLayout = true;
		feature12.pNext = &feature13;

		VkDeviceCreateInfo deviceCreateInfo{};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.pQueueCreateInfos = &queueCI;
		deviceCreateInfo.pNext = &feature12;

		VkResult err = vkCreateDevice(context.physicalDevice, &deviceCreateInfo, nullptr, &context.device);
		CHECK_VK_RESULT(err);
		if (err != VK_SUCCESS)
		{
			return false;
		}

		vkGetDeviceQueue(context.device, queueFamilyIndex, 0, &queue);
		return true;
	}

	void RenderDeviceVk::destroyDebugUtilsMessenger()
	{
		assert(context.instace && m_DebugUtilsMessenger);

		auto vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(context.instace, "vkDestroyDebugUtilsMessengerEXT"));

		vkDestroyDebugUtilsMessengerEXT(context.instace, m_DebugUtilsMessenger, nullptr);
	}

	RenderDeviceVk* RenderDeviceVk::create(const RenderDeviceCreateInfo& createInfo)
	{
		auto renderDevice = new RenderDeviceVk();
		renderDevice->createInfo = createInfo;

		g_DebugMessageCallback = createInfo.messageCallback;

		if (!renderDevice->createInstance(createInfo.enableDebugRuntime))
		{
			delete renderDevice;
			return nullptr;
		}

		if (createInfo.enableDebugRuntime)
		{
			auto vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(renderDevice->context.instace, "vkCreateDebugUtilsMessengerEXT"));

			VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI{};
			debugUtilsMessengerCI.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debugUtilsMessengerCI.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debugUtilsMessengerCI.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
			debugUtilsMessengerCI.pfnUserCallback = DebugMessageCallback;
			debugUtilsMessengerCI.pUserData = renderDevice;
			VkResult result = vkCreateDebugUtilsMessengerEXT(renderDevice->context.instace, &debugUtilsMessengerCI, nullptr, &renderDevice->m_DebugUtilsMessenger);
			assert(result == VK_SUCCESS);
		}

		if (!renderDevice->pickPhysicalDevice())
		{
			delete renderDevice;
			return nullptr;
		}

		if (!renderDevice->createDevice(createInfo))
		{
			delete renderDevice;
			return nullptr;
		}

		// Get device push descriptor properties (to display them)
		PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>(vkGetInstanceProcAddr(renderDevice->context.instace, "vkGetPhysicalDeviceProperties2KHR"));
		if (!vkGetPhysicalDeviceProperties2KHR) {
			LOG_ERROR("Could not get a valid function pointer for vkGetPhysicalDeviceProperties2KHR");
			delete renderDevice;
			return nullptr;
		}

		VkPhysicalDeviceProperties2KHR deviceProps2{};
		VkPhysicalDevicePushDescriptorPropertiesKHR pushDescriptorProps{};
		pushDescriptorProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR;
		deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
		deviceProps2.pNext = &pushDescriptorProps;
		vkGetPhysicalDeviceProperties2KHR(renderDevice->context.physicalDevice, &deviceProps2);
		renderDevice->maxPushDescriptors = pushDescriptorProps.maxPushDescriptors;

		VmaAllocatorCreateInfo allocatorCreateInfo = {};
		allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
		allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
		allocatorCreateInfo.physicalDevice = renderDevice->context.physicalDevice;
		allocatorCreateInfo.device = renderDevice->context.device;
		allocatorCreateInfo.instance = renderDevice->context.instace;
		vmaCreateAllocator(&allocatorCreateInfo, &renderDevice->m_Allocator);

		// Setup the timeline semaphore
		VkSemaphoreTypeCreateInfo semaphoreTypeCI{};
		semaphoreTypeCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO_KHR;
		semaphoreTypeCI.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE_KHR;

		VkSemaphoreCreateInfo semaphoreCI{};
		semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreCI.pNext = &semaphoreTypeCI;

		VkResult err = vkCreateSemaphore(renderDevice->context.device, &semaphoreCI, nullptr, &renderDevice->m_TrackingSubmittedSemaphore);
		CHECK_VK_RESULT(err);
		if (err != VK_SUCCESS)
		{
			delete renderDevice;
			return nullptr;
		}
		return renderDevice;
	}

	RenderDeviceVk::~RenderDeviceVk()
	{
		waitIdle();

		destroyDebugUtilsMessenger();
		vmaDestroyAllocator(m_Allocator);
		vkDestroySemaphore(context.device, m_TrackingSubmittedSemaphore, nullptr);

		for (auto commandBuffer : m_AllCommandBuffers)
		{
			delete commandBuffer;
			commandBuffer = nullptr;
		}

		vkDestroyDevice(context.device, nullptr);
		vkDestroyInstance(context.instace, nullptr);
	}

	void RenderDeviceVk::waitIdle()
	{
		vkDeviceWaitIdle(context.device);
	}

	ITexture* RenderDeviceVk::createTexture(const TextureDesc& desc)
	{
		assert(desc.format != Format::UNKNOWN);
		assert(desc.dimension != TextureDimension::Undefined);
		TextureVk* tex = new TextureVk(context, m_Allocator);
		tex->format = formatToVkFormat(desc.format);
		tex->desc = desc;

		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = getVkImageType(desc.dimension);
		imageCreateInfo.extent = { desc.width, desc.height, desc.depth };
		imageCreateInfo.mipLevels = desc.mipLevels;
		imageCreateInfo.arrayLayers = desc.depth;
		imageCreateInfo.format = tex->format;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.usage = getVkImageUsageFlags(desc);
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.samples = getVkImageSampleCount(desc);
		imageCreateInfo.flags = getVkImageCreateFlags(desc.dimension);

		// Let the library select the optimal memory type, which will likely have VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT.
		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		allocCreateInfo.priority = 1.0f;
		VkResult err = vmaCreateImage(m_Allocator, &imageCreateInfo, &allocCreateInfo, &tex->image, &tex->allocation, nullptr);
		CHECK_VK_RESULT(err, "Could not to create vkImage");
		if (err != VK_SUCCESS)
		{
			delete tex;
			return nullptr;
		}
		tex->managed = true;
		tex->createDefaultView();

		return tex;
	}

	IBuffer* RenderDeviceVk::createBuffer(const BufferDesc& desc)
	{
		BufferVk* buffer = new BufferVk(context, m_Allocator);
		buffer->desc = desc;
		VkBufferCreateInfo bufferCI{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferCI.size = desc.size;
		bufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		if ((desc.usage & BufferUsage::VertexBuffer) != 0)
		{
			bufferCI.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		}
		if ((desc.usage & BufferUsage::IndexBuffer) != 0)
		{
			bufferCI.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		}
		if ((desc.usage & BufferUsage::IndirectBuffer) != 0)
		{
			bufferCI.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
		}
		if ((desc.usage & BufferUsage::UniformBuffer) != 0)
		{
			bufferCI.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		}
		if ((desc.usage & BufferUsage::StorageBuffer) != 0)
		{
			bufferCI.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		}
		if ((desc.usage & BufferUsage::UniformTexelBuffer) != 0)
		{
			bufferCI.usage |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
		}
		if ((desc.usage & BufferUsage::StorageTexelBuffer) != 0)
		{
			bufferCI.usage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
		}

		VmaAllocationCreateInfo allocCI{};
		allocCI.usage = VMA_MEMORY_USAGE_AUTO;
		allocCI.priority = 1.0f;
		switch (desc.access)
		{
		case BufferAccess::GpuOnly:
			allocCI.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
			break;
		case BufferAccess::CpuWrite:
			allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			break;
		case BufferAccess::CpuRead:
			allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			break;
		}

		VkResult err = vmaCreateBuffer(m_Allocator, &bufferCI, &allocCI, &buffer->buffer, &buffer->allocation, &buffer->allocaionInfo);
		CHECK_VK_RESULT(err, "Could not create buffer");

		if (err != VK_SUCCESS)
		{
			delete buffer;
			buffer = nullptr;
		}

		return buffer;
	}

	ISampler* RenderDeviceVk::createSampler(const SamplerDesc& desc)
	{
		SamplerVk* sampler = new SamplerVk(context);

		VkSamplerCreateInfo samplerCI{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		samplerCI.magFilter = desc.magFilter == FilterMode::Linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
		samplerCI.minFilter = desc.minFilter == FilterMode::Linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
		samplerCI.mipmapMode = desc.mipmapMode == FilterMode::Linear ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
		samplerCI.addressModeU = convertVkSamplerAddressMode(desc.addressModeU);
		samplerCI.addressModeV = convertVkSamplerAddressMode(desc.addressModeV);
		samplerCI.addressModeW = convertVkSamplerAddressMode(desc.addressModeW);
		samplerCI.mipLodBias = desc.mipLodBias;
		samplerCI.anisotropyEnable = desc.maxAnisotropy > 0.f;
		samplerCI.maxAnisotropy = desc.maxAnisotropy;
		samplerCI.minLod = 0.f;
		samplerCI.maxLod = VK_LOD_CLAMP_NONE;
		samplerCI.borderColor = convertVkBorderColor(desc.borderColor);
		samplerCI.compareOp = VK_COMPARE_OP_NEVER;

		VkResult err = vkCreateSampler(context.device, &samplerCI, nullptr, &sampler->sampler);
		CHECK_VK_RESULT(err, "Failed to create sampler");

		if (err != VK_SUCCESS)
		{
			delete sampler;
			sampler = nullptr;
		}
		return sampler;
	}

	void* RenderDeviceVk::mapBuffer(IBuffer* buffer)
	{
		assert(buffer);
		auto buf = checked_cast<BufferVk*>(buffer);
		if (buf->getDesc().access == BufferAccess::GpuOnly)
		{
			LOG_ERROR("Could not map gpu only buffer");
			return nullptr;
		}

		return buf->allocaionInfo.pMappedData;
	}

	IBuffer* RenderDeviceVk::createBuffer(const BufferDesc& desc, const void* data, size_t dataSize)
	{
		BufferVk* buffer = checked_cast<BufferVk*>(createBuffer(desc));
		if (buffer->getDesc().access == BufferAccess::GpuOnly)
		{
			auto tmpCmdList = std::unique_ptr<CommandListVk>(checked_cast<CommandListVk*>(createCommandList()));
			tmpCmdList->open();
			tmpCmdList->updateBuffer(buffer, data, dataSize, 0);
			tmpCmdList->close();
			ICommandList* cmdListArr[] = { tmpCmdList.get() };
			uint64_t submitID = executeCommandLists(cmdListArr, 1);
			waitForExecution(submitID, UINT64_MAX);
		}
		else
		{
			vmaCopyMemoryToAllocation(m_Allocator, data, buffer->allocation, 0, dataSize);
		}

		return buffer;
	}

	IShader* RenderDeviceVk::createShader(const ShaderCreateInfo& shaderCI, const uint32_t* pCode, size_t codeSize)
	{
		assert(pCode != nullptr && codeSize != 0);

		ShaderDesc desc{};
		desc.entry = shaderCI.entry;
		desc.type = shaderCI.type;

		auto shader = new ShaderVk(context, desc);

		VkShaderModuleCreateInfo moduleCreateInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		moduleCreateInfo.codeSize = codeSize;
		moduleCreateInfo.pCode = pCode;

		VkResult err = vkCreateShaderModule(context.device, &moduleCreateInfo, nullptr, &shader->shaderModule);
		CHECK_VK_RESULT(err, "Failed to create shaderModule.");

		for (uint32_t i = 0; i < shaderCI.specializationConstantCount; ++i)
		{
			shader->specializationConstants.push_back(shaderCI.specializationConstants[i]);
		}

		if (err != VK_SUCCESS)
		{
			delete shader;
			shader = nullptr;
		}

		return shader;
	}

	IResourceSetLayout* RenderDeviceVk::createResourceSetLayout(const ResourceSetLayoutBinding* bindings, uint32_t bindingCount)
	{
		assert(bindings != nullptr && bindingCount != 0);
		ASSERT_MSG(bindingCount <= maxPushDescriptors, "A set can have a maximum of 32 descriptors.");

		auto resourceLayoutVk = new ResourceSetLayoutVk(context);

		std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings{ bindingCount };

		for (uint32_t i = 0; i < bindingCount; ++i)
		{
			VkDescriptorType descriptorType = shaderResourceTypeToVkDescriptorType(bindings[i].type);

			auto& binding = descriptorSetLayoutBindings[i];
			binding.binding = bindings[i].bindingSlot;
			binding.descriptorCount = bindings[i].arrayElementCount;
			binding.descriptorType = descriptorType;
			binding.stageFlags = shaderTypeToVkShaderStageFlagBits(bindings[i].visibleStages);
			// we will use them in ResourceSet creation
			resourceLayoutVk->resourceSetLayoutBindings.push_back(bindings[i]);
		}

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
		descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCI.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
		descriptorSetLayoutCI.bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size());
		descriptorSetLayoutCI.pBindings = descriptorSetLayoutBindings.data();
		descriptorSetLayoutCI.pNext = nullptr;

		VkResult err = vkCreateDescriptorSetLayout(context.device, &descriptorSetLayoutCI, nullptr, &resourceLayoutVk->descriptorSetLayout);
		CHECK_VK_RESULT(err, "Could not create ShaderBindingLayout");
		if (err != VK_SUCCESS)
		{
			delete resourceLayoutVk;
			return nullptr;
		}

		return resourceLayoutVk;
	}

	static void writeDescriptor(
		std::vector<VkWriteDescriptorSet>& writeDescriptorSets,
		std::vector<VkDescriptorImageInfo>& descriptorImageInfos,
		std::vector<VkDescriptorBufferInfo>& descriptorBufferInfos,
		const ResourceSetBinding& binding)
	{
		auto& setWriter = writeDescriptorSets.emplace_back();

		switch (binding.type)
		{
		case ShaderResourceType::SampledTexture:
		{
			assert(binding.textureView != nullptr);
			auto textureView = checked_cast<TextureViewVk*>(binding.textureView);

			auto& descriptorImageInfo = descriptorImageInfos.emplace_back();
			descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			descriptorImageInfo.imageView = textureView->imageView;
			descriptorImageInfo.sampler = nullptr;

			setWriter.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			setWriter.pNext = nullptr;
			setWriter.dstBinding = binding.bindingSlot;
			setWriter.dstSet = 0;
			setWriter.dstArrayElement = binding.arrayElementIndex;
			setWriter.pImageInfo = &descriptorImageInfo;
			setWriter.descriptorType = shaderResourceTypeToVkDescriptorType(binding.type);
			setWriter.descriptorCount = 1;

			break;
		}
		case ShaderResourceType::StorageTexture:
		{
			assert(binding.textureView != nullptr);
			auto textureView = checked_cast<TextureViewVk*>(binding.textureView);

			auto& descriptorImageInfo = descriptorImageInfos.emplace_back();
			descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			descriptorImageInfo.imageView = textureView->imageView;
			descriptorImageInfo.sampler = nullptr;

			setWriter.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			setWriter.pNext = nullptr;
			setWriter.dstBinding = binding.bindingSlot;
			setWriter.dstSet = 0;
			setWriter.dstArrayElement = binding.arrayElementIndex;
			setWriter.pImageInfo = &descriptorImageInfo;
			setWriter.descriptorType = shaderResourceTypeToVkDescriptorType(binding.type);
			setWriter.descriptorCount = 1;

			break;
		}
		case ShaderResourceType::TextureWithSampler:
		{
			assert(binding.textureView != nullptr);
			assert(binding.sampler != nullptr);
			auto textureView = checked_cast<TextureViewVk*>(binding.textureView);

			auto& descriptorImageInfo = descriptorImageInfos.emplace_back();
			descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			descriptorImageInfo.imageView = textureView->imageView;
			descriptorImageInfo.sampler = checked_cast<SamplerVk*>(binding.sampler)->sampler;

			setWriter.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			setWriter.pNext = nullptr;
			setWriter.dstBinding = binding.bindingSlot;
			setWriter.dstSet = 0;
			setWriter.dstArrayElement = binding.arrayElementIndex;
			setWriter.pImageInfo = &descriptorImageInfo;
			setWriter.descriptorType = shaderResourceTypeToVkDescriptorType(binding.type);
			setWriter.descriptorCount = 1;

			break;
		}
		case ShaderResourceType::StorageBuffer:
		case ShaderResourceType::UniformBuffer:
		{
			assert(binding.buffer != nullptr);
			auto buffer = checked_cast<BufferVk*>(binding.buffer);

			auto& descriptorBufferInfo = descriptorBufferInfos.emplace_back();
			descriptorBufferInfo.buffer = buffer->buffer;
			descriptorBufferInfo.offset = binding.bufferOffset;
			descriptorBufferInfo.range = binding.bufferRange == 0 ? VK_WHOLE_SIZE : binding.bufferRange;

			setWriter.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			setWriter.pNext = nullptr;
			setWriter.dstBinding = binding.bindingSlot;
			setWriter.dstSet = 0;
			setWriter.dstArrayElement = binding.arrayElementIndex;
			setWriter.pBufferInfo = &descriptorBufferInfo;
			setWriter.descriptorType = shaderResourceTypeToVkDescriptorType(binding.type);
			setWriter.descriptorCount = 1;

			break;
		}
		case ShaderResourceType::Sampler:
		{
			assert(binding.sampler != nullptr);

			auto sampler = checked_cast<SamplerVk*>(binding.sampler);

			auto& descriptorImageInfo = descriptorImageInfos.emplace_back();
			descriptorImageInfo.sampler = sampler->sampler;
			descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			descriptorImageInfo.imageView = nullptr;

			setWriter.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			setWriter.pNext = nullptr;
			setWriter.dstBinding = binding.bindingSlot;
			setWriter.dstSet = 0;
			setWriter.dstArrayElement = binding.arrayElementIndex;
			setWriter.pImageInfo = &descriptorImageInfo;
			setWriter.descriptorCount = 1;

			break;
		}
		case ShaderResourceType::UniformTexelBuffer:
		case ShaderResourceType::StorageTexelBuffer:
		{
			assert(!"not yet implemented");
			break;
		}
		default:
			assert(!"invalid ShaderResourceType");
			break;
		}
	}

	IResourceSet* RenderDeviceVk::createResourceSet(const IResourceSetLayout* layout, const ResourceSetBinding* bindings, uint32_t bindingCount)
	{
		assert(layout);
		const auto setLayout = checked_cast<const ResourceSetLayoutVk*>(layout);

		auto resourceSet = new ResourceSetVk(context);
		resourceSet->resourceSetLayout = setLayout;
		resourceSet->writeDescriptorSets.reserve(maxPushDescriptors);
		resourceSet->descriptorBufferInfos.reserve(maxPushDescriptors);
		resourceSet->descriptorImageInfos.reserve(maxPushDescriptors);

		for (uint32_t i = 0; i < bindingCount; ++i)
		{
			const ResourceSetBinding& binding = bindings[i];

			ShaderType bindingVisibleStages;

			auto checkValidBinding = [&](const ResourceSetLayoutBinding& layoutBinding)->bool
				{
					return layoutBinding.bindingSlot == binding.bindingSlot && layoutBinding.type == binding.type;
				};

			if (auto it = std::find_if(
				std::begin(setLayout->resourceSetLayoutBindings),
				std::end(setLayout->resourceSetLayoutBindings),
				checkValidBinding);
				it != std::end(setLayout->resourceSetLayoutBindings))
			{
				bindingVisibleStages = it->visibleStages;
			}
			else
			{
				assert(!"Invalid ResourceSetBinding, make sure the bindingSlot and resourceType match those in the ResourceSetLayout.");
			}

			resourceSet->resourcesNeedStateTransition.emplace_back(ResourceSetBindngWithVisibleStages{ binding, bindingVisibleStages });

			writeDescriptor(resourceSet->writeDescriptorSets, resourceSet->descriptorImageInfos, resourceSet->descriptorBufferInfos, binding);
		}

		return resourceSet;
	}

	void RenderDeviceVk::updateResourceSet(IResourceSet* set, const ResourceSetBinding* bindings, uint32_t bindingCount)
	{
		assert(set);
		assert(bindings != nullptr && bindingCount != 0);
		auto resourceSet = checked_cast<ResourceSetVk*>(set);

		const ResourceSetLayoutVk* layout = resourceSet->resourceSetLayout;

		for (uint32_t i = 0; i < bindingCount; ++i)
		{
			const ResourceSetBinding& binding = bindings[i];

			auto checkValidBinding = [&](const VkWriteDescriptorSet& setWriter)->bool
				{
					return setWriter.dstBinding == binding.bindingSlot && setWriter.dstArrayElement == binding.arrayElementIndex;
				};

			if (auto it = std::find_if(
				std::begin(resourceSet->writeDescriptorSets),
				std::end(resourceSet->writeDescriptorSets),
				checkValidBinding);
				it != std::end(resourceSet->writeDescriptorSets))
			{
				auto& setWriter = *it;
				// if exists, only update resource pointer
				ASSERT_MSG(shaderResourceTypeToVkDescriptorType(binding.type) == setWriter.descriptorType,
					"Invalid ResourceSetBinding, resourceType mismatch.");
				switch (binding.type)
				{
				case ShaderResourceType::SampledTexture:
				case ShaderResourceType::StorageTexture:
				{

					auto descriptorImageInfo = const_cast<VkDescriptorImageInfo*>(setWriter.pImageInfo);
					descriptorImageInfo->imageView = checked_cast<TextureViewVk*>(binding.textureView)->imageView;
					break;
				}
				case ShaderResourceType::TextureWithSampler:
				{
					auto descriptorImageInfo = const_cast<VkDescriptorImageInfo*>(setWriter.pImageInfo);
					descriptorImageInfo->imageView = checked_cast<TextureViewVk*>(binding.textureView)->imageView;
					descriptorImageInfo->sampler = checked_cast<SamplerVk*>(binding.sampler)->sampler;
					break;
				}
				case ShaderResourceType::StorageBuffer:
				case ShaderResourceType::UniformBuffer:
				{
					auto descriptorBufferInfo = const_cast<VkDescriptorBufferInfo*>(setWriter.pBufferInfo);
					descriptorBufferInfo->buffer = checked_cast<BufferVk*>(binding.buffer)->buffer;
					descriptorBufferInfo->offset = binding.bufferOffset;
					descriptorBufferInfo->range = binding.bufferRange == 0 ? VK_WHOLE_SIZE : binding.bufferRange;
					break;
				}
				case ShaderResourceType::Sampler:
				{
					auto descriptorImageInfo = const_cast<VkDescriptorImageInfo*>(setWriter.pImageInfo);
					descriptorImageInfo->sampler = checked_cast<SamplerVk*>(binding.sampler)->sampler;
					break;
				}
				default:
					break; //nothing to do
				}

			}
			else
			{
				ShaderType bindingVisibleStages;

				auto checkValidBinding = [&](const ResourceSetLayoutBinding& layoutBinding)->bool
					{
						return layoutBinding.bindingSlot == binding.bindingSlot && layoutBinding.type == binding.type;
					};

				if (auto it = std::find_if(
					std::begin(layout->resourceSetLayoutBindings),
					std::end(layout->resourceSetLayoutBindings),
					checkValidBinding);
					it != std::end(layout->resourceSetLayoutBindings))
				{
					bindingVisibleStages = it->visibleStages;
				}
				else
				{
					assert(!"Invalid ResourceSetBinding, make sure the bindingSlot and resourceType match those in the ResourceSetLayout.");
				}

				resourceSet->resourcesNeedStateTransition.emplace_back(ResourceSetBindngWithVisibleStages{ binding, bindingVisibleStages });

				writeDescriptor(resourceSet->writeDescriptorSets, resourceSet->descriptorImageInfos, resourceSet->descriptorBufferInfos, binding);
			}

		}
	}

	static void resolveVertexInputOffsetAndStride(VertexInputAttribute* attributes, uint32_t attributeCount)
	{
		uint32_t bufferSlotUsed = 0;
		for (uint32_t i = 0; i < attributeCount; ++i)
		{
			bufferSlotUsed = (std::max)(bufferSlotUsed, attributes[i].bindingBufferSlot + 1);
		}
		std::vector<uint32_t> autoSetStrides(bufferSlotUsed, 0);
		std::vector<uint32_t> originStrides(bufferSlotUsed, UINT32_MAX);

		for (uint32_t i = 0; i < attributeCount; ++i)
		{
			uint32_t bufferSlot = attributes[i].bindingBufferSlot;
			auto& currentAutoSetStride = autoSetStrides[bufferSlot];

			auto& formatInfo = getFormatInfo(attributes[i].format);

			if (attributes[i].offsetInElement == UINT32_MAX)
			{
				attributes[i].offsetInElement = currentAutoSetStride;
			}

			if (attributes[i].elementStride != UINT32_MAX)
			{
				if (originStrides[bufferSlot] != UINT32_MAX &&
					attributes[i].elementStride != originStrides[bufferSlot])
				{
					LOG_ERROR("stride between elements is not consistent in the same buffer slot.");
				}
				originStrides[bufferSlot] = attributes[i].elementStride;
			}

			currentAutoSetStride = (std::max)(currentAutoSetStride, attributes[i].offsetInElement + formatInfo.bytes);
		}

		for (uint32_t i = 0; i < attributeCount; ++i)
		{
			uint32_t bufferSlot = attributes[i].bindingBufferSlot;
			if (originStrides[bufferSlot] != UINT32_MAX && originStrides[bufferSlot] >= autoSetStrides[bufferSlot])
			{
				LOG_ERROR("stride is too small");
			}

			if (attributes[i].elementStride == UINT32_MAX)
			{
				attributes[i].elementStride = autoSetStrides[bufferSlot];
			}
		}
	}

	static VkPipelineShaderStageCreateInfo getShaderStageCreateInfo(ShaderVk& shader, std::vector<VkSpecializationMapEntry>& specMapEntries,
		std::vector<VkSpecializationInfo>& specInfos, std::vector<uint32_t>& specData)
	{
		VkPipelineShaderStageCreateInfo shaderStageCI{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		shaderStageCI.stage = shaderTypeToVkShaderStageFlagBits(shader.getDesc().type);
		shaderStageCI.module = shader.shaderModule;
		shaderStageCI.pName = shader.getDesc().entry;

		if (!shader.specializationConstants.empty())
		{
			// The vectors are pre-allocated, so it's safe to use .data() before writing the data
			shaderStageCI.pSpecializationInfo = specInfos.data() + specInfos.size();

			VkSpecializationInfo specInfo{};
			specInfo.pData = specData.data() + specData.size();
			specInfo.mapEntryCount = static_cast<uint32_t>(shader.specializationConstants.size());
			specInfo.pMapEntries = specMapEntries.data() + specMapEntries.size();
			specInfo.dataSize = shader.specializationConstants.size() * sizeof(uint32_t);

			uint32_t dataOffset = 0;
			for (const auto& constant : shader.specializationConstants)
			{
				VkSpecializationMapEntry specMapEntry{};
				specMapEntry.constantID = constant.constantID;
				specMapEntry.offset = dataOffset;
				specMapEntry.size = sizeof(uint32_t);

				specMapEntries.push_back(specMapEntry);
				specData.push_back(constant.value.u);
				dataOffset += static_cast<uint32_t>(specMapEntry.size);
			}

			specInfos.push_back(specInfo);
		}
		return shaderStageCI;
	}

	static void countSpecializationConstants(ShaderVk* shader, uint32_t& shaderCount, uint32_t& shaderWithSpecializationCount, uint32_t& specializationConstantCount)
	{
		if (!shader)
		{
			return;
		}
		shaderCount++;
		if (shader->specializationConstants.empty())
		{
			return;
		}
		shaderWithSpecializationCount++;
		specializationConstantCount += static_cast<uint32_t>(shader->specializationConstants.size());
	}

	static GraphicsPipelineDesc getGraphicsPipelineDesc(const GraphicsPipelineCreateInfo& pipelineCI)
	{
		GraphicsPipelineDesc desc;
		desc.blendState = pipelineCI.blendState;
		desc.rasterState = pipelineCI.rasterState;
		desc.primType = pipelineCI.primType;
		desc.depthStencilFormat = pipelineCI.depthStencilFormat;
		desc.renderTargetFormatCount = pipelineCI.renderTargetFormatCount;
		desc.viewportCount = pipelineCI.viewportCount;
		desc.sampleCount = pipelineCI.sampleCount;
		desc.patchControlPoints = pipelineCI.patchControlPoints;
		for (uint32_t i = 0; i < pipelineCI.renderTargetFormatCount; ++i)
		{
			desc.renderTargetFormats[i] = pipelineCI.renderTargetFormats[i];
		}
		return desc;
	}

	IGraphicsPipeline* RenderDeviceVk::createGraphicsPipeline(const GraphicsPipelineCreateInfo& pipelineCI)
	{
		auto pipeline = new GraphicsPipelineVk(context);

		// pipeline layout
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ pipelineCI.resourceSetLayoutCount };
		for (uint32_t i = 0; i < pipelineCI.resourceSetLayoutCount; ++i)
		{
			auto resourceSetLayout = checked_cast<ResourceSetLayoutVk*>(pipelineCI.resourceSetLayouts[i]);
			descriptorSetLayouts[i] = resourceSetLayout->descriptorSetLayout;
		}

		std::vector<VkPushConstantRange> pushConstantRanges(pipelineCI.pushConstantCount);
		pipeline->pushConstantInfos.resize(pushConstantRanges.size());
		ShaderType usedStages = ShaderType::Unknown;
		uint32_t offset = 0;
		for (uint32_t i = 0; i < pushConstantRanges.size(); ++i)
		{
			ASSERT_MSG((pipelineCI.pushConstantDescs[i].stage & usedStages) == 0, "Each pipeline stage can only have one pushConstants."); // to simplify the design
			pushConstantRanges[i].stageFlags = shaderTypeToVkShaderStageFlagBits(pipelineCI.pushConstantDescs[i].stage);
			pushConstantRanges[i].size = pipelineCI.pushConstantDescs[i].size;
			pushConstantRanges[i].offset = offset;
			pipeline->pushConstantInfos[i] = { pipelineCI.pushConstantDescs[i], offset };
			offset += pushConstantRanges[i].size;
			usedStages = usedStages | pipelineCI.pushConstantDescs[i].stage;
		}

		VkPipelineLayoutCreateInfo pipelineLayoutCI{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		pipelineLayoutCI.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
		pipelineLayoutCI.pPushConstantRanges = pushConstantRanges.data();
		pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutCI.pSetLayouts = descriptorSetLayouts.data();
		VkResult err = vkCreatePipelineLayout(context.device, &pipelineLayoutCI, nullptr, &pipeline->pipelineLayout);
		CHECK_VK_RESULT(err);

		VkGraphicsPipelineCreateInfo createInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

		// Rasterization state
		VkPipelineRasterizationStateCreateInfo rasterizationStateCI{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
		rasterizationStateCI.cullMode = convertCullMode(pipelineCI.rasterState.cullMode);
		rasterizationStateCI.polygonMode = convertPolygonMode(pipelineCI.rasterState.fillMode);
		rasterizationStateCI.frontFace = pipelineCI.rasterState.frontCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;;
		rasterizationStateCI.depthClampEnable = pipelineCI.rasterState.depthClampEnable;
		rasterizationStateCI.depthBiasEnable = pipelineCI.rasterState.depthBiasEnable;
		rasterizationStateCI.lineWidth = pipelineCI.rasterState.lineWidth;

		// blend state
		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates{ pipelineCI.renderTargetFormatCount };
		for (uint32_t i = 0; i < pipelineCI.renderTargetFormatCount; ++i)
		{
			auto& colorBlendAttachmentState = colorBlendAttachmentStates[i];
			colorBlendAttachmentState.blendEnable = pipelineCI.blendState.renderTargetBlendStates[i].blendEnable;
			colorBlendAttachmentState.srcColorBlendFactor = convertBlendFactor(pipelineCI.blendState.renderTargetBlendStates[i].srcColorBlend);
			colorBlendAttachmentState.dstColorBlendFactor = convertBlendFactor(pipelineCI.blendState.renderTargetBlendStates[i].srcColorBlend);
			colorBlendAttachmentState.colorBlendOp = convertBlendOp(pipelineCI.blendState.renderTargetBlendStates[i].colorBlendOp);
			colorBlendAttachmentState.srcAlphaBlendFactor = convertBlendFactor(pipelineCI.blendState.renderTargetBlendStates[i].srcAlphaBlend);
			colorBlendAttachmentState.dstAlphaBlendFactor = convertBlendFactor(pipelineCI.blendState.renderTargetBlendStates[i].destAlphaBlend);
			colorBlendAttachmentState.alphaBlendOp = convertBlendOp(pipelineCI.blendState.renderTargetBlendStates[i].alphaBlendOp);
			colorBlendAttachmentState.colorWriteMask = convertColorMask(pipelineCI.blendState.renderTargetBlendStates[i].colorWriteMask);
		}

		VkPipelineColorBlendStateCreateInfo colorBlendStateCI{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
		colorBlendStateCI.attachmentCount = static_cast<uint32_t>(colorBlendAttachmentStates.size());
		colorBlendStateCI.pAttachments = colorBlendAttachmentStates.data();

		// Viewport state sets the number of viewports and scissor used in this pipeline
		// Note: This is actually overridden by the dynamic states (see below)
		VkPipelineViewportStateCreateInfo viewportStateCI{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
		viewportStateCI.viewportCount = pipelineCI.viewportCount;
		viewportStateCI.scissorCount = pipelineCI.viewportCount;

		// Enable dynamic states
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicStateCI{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
		dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
		dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

		// Depth and stencil state
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
		depthStencilStateCI.depthTestEnable = pipelineCI.depthStencilState.depthTestEnable;
		depthStencilStateCI.depthWriteEnable = pipelineCI.depthStencilState.depthWriteEnable;
		depthStencilStateCI.depthCompareOp = convertCompareOp(pipelineCI.depthStencilState.depthCompareOp);
		depthStencilStateCI.stencilTestEnable = pipelineCI.depthStencilState.stencilTestEnable;
		depthStencilStateCI.front = convertStencilOpState(pipelineCI.depthStencilState.frontFaceStencil);
		depthStencilStateCI.back = convertStencilOpState(pipelineCI.depthStencilState.backFaceStencil);

		VkPipelineMultisampleStateCreateInfo multisampleStateCI{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
		multisampleStateCI.alphaToCoverageEnable = pipelineCI.blendState.alphaToCoverageEnable;
		multisampleStateCI.rasterizationSamples = static_cast<VkSampleCountFlagBits>(pipelineCI.sampleCount);

		// vertex input 
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
		inputAssemblyStateCI.topology = convertPrimitiveTopology(pipelineCI.primType);

		resolveVertexInputOffsetAndStride(pipelineCI.vertexInputAttributes, pipelineCI.vertexInputAttributeCount);

		std::vector<VkVertexInputBindingDescription> vertexInputBindings;
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributes;

		constexpr int maxVertexInputBindings = 32;
		int bufferSlotUsed[maxVertexInputBindings]{};
		for (uint32_t i = 0; i < pipelineCI.vertexInputAttributeCount; ++i)
		{
			auto& attribute = pipelineCI.vertexInputAttributes[i];

			uint32_t bufferSlot = attribute.bindingBufferSlot;
			if (bufferSlotUsed[bufferSlot] == 0)
			{
				auto& vertexInputBinding = vertexInputBindings.emplace_back();
				vertexInputBinding.binding = attribute.bindingBufferSlot;
				vertexInputBinding.stride = attribute.elementStride;
				vertexInputBinding.inputRate = attribute.isInstanced ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;

				bufferSlotUsed[bufferSlot]++;
			}

			auto& vertexInputAttribute = vertexInputAttributes.emplace_back();
			vertexInputAttribute.binding = attribute.bindingBufferSlot;
			vertexInputAttribute.location = attribute.location;
			vertexInputAttribute.format = formatToVkFormat(attribute.format);
			vertexInputAttribute.offset = attribute.offsetInElement;
		}

		VkPipelineVertexInputStateCreateInfo vertexInputStateCI{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
		vertexInputStateCI.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
		vertexInputStateCI.pVertexBindingDescriptions = vertexInputBindings.data();
		vertexInputStateCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
		vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributes.data();

		// shader 

		auto vertexShader = checked_cast<ShaderVk*>(pipelineCI.vertexShader);
		auto fragmentShader = checked_cast<ShaderVk*>(pipelineCI.fragmentShader);
		auto tessControlShader = checked_cast<ShaderVk*>(pipelineCI.tessControlShader);
		auto tessEvaluationShader = checked_cast<ShaderVk*>(pipelineCI.tessEvaluationShader);
		auto geometryShader = checked_cast<ShaderVk*>(pipelineCI.geometryShader);

		uint32_t shaderCount = 0;
		uint32_t shaderWithSpecializationCount = 0;
		uint32_t specializationConstantCount = 0;

		countSpecializationConstants(vertexShader, shaderCount, specializationConstantCount, specializationConstantCount);
		countSpecializationConstants(fragmentShader, shaderCount, specializationConstantCount, specializationConstantCount);
		countSpecializationConstants(tessControlShader, shaderCount, specializationConstantCount, specializationConstantCount);
		countSpecializationConstants(vertexShader, shaderCount, specializationConstantCount, specializationConstantCount);
		countSpecializationConstants(tessEvaluationShader, shaderCount, specializationConstantCount, specializationConstantCount);
		countSpecializationConstants(geometryShader, shaderCount, specializationConstantCount, specializationConstantCount);

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		std::vector<VkSpecializationMapEntry> specializationMapEntries;
		std::vector<VkSpecializationInfo> specializationInfo;
		std::vector<uint32_t> specializationData;

		shaderStages.reserve(shaderCount);
		specializationMapEntries.reserve(specializationConstantCount);
		specializationInfo.reserve(shaderWithSpecializationCount);
		specializationData.reserve(specializationConstantCount);

		if (vertexShader)
		{
			shaderStages.push_back(getShaderStageCreateInfo(*vertexShader, specializationMapEntries, specializationInfo, specializationData));
		}
		if (fragmentShader)
		{
			shaderStages.push_back(getShaderStageCreateInfo(*fragmentShader, specializationMapEntries, specializationInfo, specializationData));
		}
		if (tessControlShader)
		{
			shaderStages.push_back(getShaderStageCreateInfo(*tessControlShader, specializationMapEntries, specializationInfo, specializationData));
		}
		if (tessEvaluationShader)
		{
			shaderStages.push_back(getShaderStageCreateInfo(*tessEvaluationShader, specializationMapEntries, specializationInfo, specializationData));
		}
		if (geometryShader)
		{
			shaderStages.push_back(getShaderStageCreateInfo(*geometryShader, specializationMapEntries, specializationInfo, specializationData));
		}

		// tessellation 
		if (pipelineCI.primType == PrimitiveType::PatchList)
		{
			VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
			tessellationStateCreateInfo.patchControlPoints = pipelineCI.patchControlPoints;
			createInfo.pTessellationState = &tessellationStateCreateInfo;
		}

		// Attachment information for dynamic rendering
		std::vector<VkFormat> colorAttachmentFormats{ pipelineCI.renderTargetFormatCount };
		for (int i = 0; i < colorAttachmentFormats.size(); ++i)
		{
			colorAttachmentFormats[i] = formatToVkFormat(pipelineCI.renderTargetFormats[i]);
		}
		VkPipelineRenderingCreateInfoKHR pipelineRenderingCI{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
		pipelineRenderingCI.colorAttachmentCount = pipelineCI.renderTargetFormatCount;
		pipelineRenderingCI.pColorAttachmentFormats = colorAttachmentFormats.data();
		pipelineRenderingCI.depthAttachmentFormat = formatToVkFormat(pipelineCI.depthStencilFormat);
		pipelineRenderingCI.stencilAttachmentFormat = formatToVkFormat(pipelineCI.depthStencilFormat);

		createInfo.layout = pipeline->pipelineLayout;
		createInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		createInfo.pStages = shaderStages.data();
		createInfo.pVertexInputState = &vertexInputStateCI;
		createInfo.pInputAssemblyState = &inputAssemblyStateCI;
		createInfo.pViewportState = &viewportStateCI;
		createInfo.pRasterizationState = &rasterizationStateCI;
		createInfo.pColorBlendState = &colorBlendStateCI;
		createInfo.pMultisampleState = &multisampleStateCI;
		createInfo.pDepthStencilState = &depthStencilStateCI;
		createInfo.pDynamicState = &dynamicStateCI;
		createInfo.pNext = &pipelineRenderingCI;

		VkPipelineCacheCreateInfo cacheCI{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
		cacheCI.initialDataSize = pipelineCI.cacheSize;
		cacheCI.pInitialData = pipelineCI.cacheData;

		err = vkCreatePipelineCache(context.device, &cacheCI, nullptr, &pipeline->pipelineCache);
		CHECK_VK_RESULT(err, "Failed to create pipeline cache.");

		err = vkCreateGraphicsPipelines(context.device, pipeline->pipelineCache, 1, &createInfo, nullptr, &pipeline->pipeline);
		CHECK_VK_RESULT(err, "Failed to create pipeline.");

		pipeline->desc = getGraphicsPipelineDesc(pipelineCI);
		if (err != VK_SUCCESS)
		{
			delete pipeline;
			pipeline = nullptr;
		}

		return pipeline;
	}

	IComputePipeline* RenderDeviceVk::createComputePipeline(const ComputePipelineCreateInfo& pipelineCI)
	{
		auto pipeline = new ComputePipelineVk(context);

		// pipeline layout
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ pipelineCI.resourceSetLayoutCount };
		for (uint32_t i = 0; i < pipelineCI.resourceSetLayoutCount; ++i)
		{
			auto resourceSetLayout = checked_cast<ResourceSetLayoutVk*>(pipelineCI.resourceSetLayouts[i]);
			descriptorSetLayouts[i] = resourceSetLayout->descriptorSetLayout;
		}

		std::vector<VkPushConstantRange> pushConstantRanges(pipelineCI.pushConstantCount);
		pipeline->pushConstantInfos.resize(pushConstantRanges.size());
		ShaderType usedStages = ShaderType::Unknown;
		uint32_t offset = 0;
		for (uint32_t i = 0; i < pushConstantRanges.size(); ++i)
		{
			ASSERT_MSG((pipelineCI.pushConstantDescs[i].stage & usedStages) == 0, "Each pipeline stage can only have one pushConstants."); // to simplify the design
			pushConstantRanges[i].stageFlags = shaderTypeToVkShaderStageFlagBits(pipelineCI.pushConstantDescs[i].stage);
			pushConstantRanges[i].size = pipelineCI.pushConstantDescs[i].size;
			pushConstantRanges[i].offset = offset;
			pipeline->pushConstantInfos[i] = { pipelineCI.pushConstantDescs[i], offset };
			offset += pushConstantRanges[i].size;
			usedStages = usedStages | pipelineCI.pushConstantDescs[i].stage;
		}

		VkPipelineLayoutCreateInfo pipelineLayoutCI{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		pipelineLayoutCI.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
		pipelineLayoutCI.pPushConstantRanges = pushConstantRanges.data();
		pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutCI.pSetLayouts = descriptorSetLayouts.data();
		VkResult err = vkCreatePipelineLayout(context.device, &pipelineLayoutCI, nullptr, &pipeline->pipelineLayout);
		CHECK_VK_RESULT(err);

		auto shader = checked_cast<ShaderVk*>(pipelineCI.computeShader);

		uint32_t shaderCount = 0;
		uint32_t shaderWithSpecializationCount = 0;
		uint32_t specializationConstantCount = 0;

		countSpecializationConstants(shader, shaderCount, shaderWithSpecializationCount, specializationConstantCount);

		std::vector<VkSpecializationMapEntry> specializationMapEntries;
		std::vector<VkSpecializationInfo> specializationInfo;
		std::vector<uint32_t> specializationData;

		specializationMapEntries.reserve(specializationConstantCount);
		specializationInfo.reserve(shaderWithSpecializationCount);
		specializationData.reserve(specializationConstantCount);

		VkPipelineShaderStageCreateInfo shaderStageCI = getShaderStageCreateInfo(*shader, specializationMapEntries,
			specializationInfo, specializationData);

		VkPipelineCacheCreateInfo cacheCI{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
		cacheCI.initialDataSize = pipelineCI.cacheSize;
		cacheCI.pInitialData = pipelineCI.cacheData;

		err = vkCreatePipelineCache(context.device, &cacheCI, nullptr, &pipeline->pipelineCache);
		CHECK_VK_RESULT(err, "Failed to create pipeline cache.");

		VkComputePipelineCreateInfo computePipelineCI{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
		computePipelineCI.stage = shaderStageCI;
		computePipelineCI.layout = pipeline->pipelineLayout;

		err = vkCreateComputePipelines(context.device, pipeline->pipelineCache, 1, &computePipelineCI, nullptr, &pipeline->pipeline);
		CHECK_VK_RESULT(err, "Failed to create pipeline.");
		if (err != VK_SUCCESS)
		{
			delete pipeline;
			pipeline = nullptr;
		}
		return pipeline;
	}

	TextureVk* RenderDeviceVk::createTextureWithExistImage(const TextureDesc& desc, VkImage image)
	{
		assert(desc.format != Format::UNKNOWN);
		assert(desc.dimension != TextureDimension::Undefined);

		auto tex = new TextureVk{ context, m_Allocator };
		tex->image = image;
		tex->managed = false;
		tex->format = formatToVkFormat(desc.format);
		tex->desc = desc;
		tex->createDefaultView();

		return tex;
	}

	void RenderDeviceVk::setSwapChainImageAvailableSeamaphore(const VkSemaphore& semaophore)
	{
		m_SwapChainImgAvailableSemaphore = semaophore;
	}

	void RenderDeviceVk::setRenderCompleteSemaphore(const VkSemaphore& semaphore)
	{
		m_RenderCompleteSemaphore = semaphore;
	}

	ICommandList* RenderDeviceVk::createCommandList()
	{
		return new CommandListVk(*this);
	}

	uint64_t RenderDeviceVk::executeCommandLists(ICommandList** cmdLists, size_t numCmdLists)
	{
		++lastSubmittedID;
		bool hasGraphicPipeline = false;
		m_CmdBufSubmitInfos.resize(numCmdLists);
		for (int i = 0; i < numCmdLists; ++i)
		{
			assert(cmdLists[i] != nullptr);
			auto cmdList = checked_cast<CommandListVk*>(cmdLists[i]);
			cmdList->updateSubmittedState();
			hasGraphicPipeline = cmdList->hasSetGraphicPipeline();

			CommandBuffer* cmdBuffer = cmdList->getCommandBuffer();
			cmdBuffer->submitID = lastSubmittedID;
			m_CommandBufferInFlight.push_back(cmdBuffer);

			m_CmdBufSubmitInfos[i].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
			m_CmdBufSubmitInfos[i].commandBuffer = cmdBuffer->vkCmdBuf;
		}

		VkSubmitInfo2 submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO_2 };
		submitInfo.signalSemaphoreInfoCount = 1;

		VkSemaphoreSubmitInfo waitSemaphoreSubmitInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
		waitSemaphoreSubmitInfo.semaphore = m_SwapChainImgAvailableSemaphore;
		waitSemaphoreSubmitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		if (hasGraphicPipeline && m_SwapChainImgAvailableSemaphore != VK_NULL_HANDLE)
		{
			submitInfo.waitSemaphoreInfoCount = 1;
			submitInfo.pWaitSemaphoreInfos = &waitSemaphoreSubmitInfo;
		}

		VkSemaphoreSubmitInfo signalSemaphoreSubmitInfos[2]{};
		signalSemaphoreSubmitInfos[0].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signalSemaphoreSubmitInfos[0].semaphore = m_TrackingSubmittedSemaphore;
		signalSemaphoreSubmitInfos[0].value = lastSubmittedID;
		signalSemaphoreSubmitInfos[0].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

		if (m_RenderCompleteSemaphore != VK_NULL_HANDLE)
		{
			signalSemaphoreSubmitInfos[1].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
			signalSemaphoreSubmitInfos[1].semaphore = m_RenderCompleteSemaphore;
			signalSemaphoreSubmitInfos[1].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			submitInfo.signalSemaphoreInfoCount = 2;
		}

		submitInfo.pSignalSemaphoreInfos = signalSemaphoreSubmitInfos;
		submitInfo.commandBufferInfoCount = static_cast<uint32_t>(m_CmdBufSubmitInfos.size());
		submitInfo.pCommandBufferInfos = m_CmdBufSubmitInfos.data();

		VkResult err = vkQueueSubmit2(queue, 1, &submitInfo, VK_NULL_HANDLE);
		CHECK_VK_RESULT(err);

		m_CmdBufSubmitInfos.clear();

		// we only need to wait for swapChain image available at first time that graphicPipeline is set.
		m_SwapChainImgAvailableSemaphore = VK_NULL_HANDLE;
		m_RenderCompleteSemaphore = VK_NULL_HANDLE;
		return lastSubmittedID;
	}

	void RenderDeviceVk::waitForExecution(uint64_t executeID, uint64_t timeout)
	{
		uint64_t lastFinishedID;
		VkResult err = vkGetSemaphoreCounterValue(context.device, m_TrackingSubmittedSemaphore, &lastFinishedID);
		CHECK_VK_RESULT(err);

		if (lastFinishedID >= executeID)
		{
			return;
		}

		VkSemaphoreWaitInfo semaphoreWaitInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO };
		semaphoreWaitInfo.semaphoreCount = 1;
		semaphoreWaitInfo.pSemaphores = &m_TrackingSubmittedSemaphore;
		semaphoreWaitInfo.pValues = &executeID;

		err = vkWaitSemaphores(context.device, &semaphoreWaitInfo, timeout);
		CHECK_VK_RESULT(err);
	}

	CommandBuffer* RenderDeviceVk::getOrCreateCommandBuffer()
	{
#if defined RHI_ENABLE_THREAD_RECORDING
		std::lock_guard(m_Mutex);
#endif
		CommandBuffer* cmdBuf;
		if (m_CommandBufferPool.empty())
		{
			cmdBuf = new CommandBuffer(context);
			VkCommandPoolCreateInfo commandPoolCI{};
			commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			commandPoolCI.queueFamilyIndex = queueFamilyIndex;
			commandPoolCI.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

			VkResult err = vkCreateCommandPool(context.device, &commandPoolCI, nullptr, &cmdBuf->vkCmdPool);
			CHECK_VK_RESULT(err, "Could not create vkCommandPool");

			VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
			commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			commandBufferAllocateInfo.commandPool = cmdBuf->vkCmdPool;
			commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			commandBufferAllocateInfo.commandBufferCount = 1;

			err = vkAllocateCommandBuffers(context.device, &commandBufferAllocateInfo, &cmdBuf->vkCmdBuf);
			CHECK_VK_RESULT(err, "Could not create vkCommandBuffer");

			if (err != VK_SUCCESS)
			{
				delete cmdBuf;
				return nullptr;
			}
			m_AllCommandBuffers.push_back(cmdBuf);
		}
		else
		{
			cmdBuf = m_CommandBufferPool.back();
			m_CommandBufferPool.pop_back();
		}

		return cmdBuf;
	}

	void RenderDeviceVk::recycleCommandBuffers()
	{
		std::vector<CommandBuffer*> submittedCmdBuf = std::move(m_CommandBufferInFlight);

		uint64_t lastFinishedID;
		VkResult err = vkGetSemaphoreCounterValue(context.device, m_TrackingSubmittedSemaphore, &lastFinishedID);
		CHECK_VK_RESULT(err);

		for (auto commandBuffer : submittedCmdBuf)
		{
			if (commandBuffer->submitID <= lastFinishedID)
			{
				commandBuffer->referencedInternalStageBuffer.clear();
				commandBuffer->submitID = 0;
				m_CommandBufferPool.push_back(commandBuffer);
			}
			else
			{
				m_CommandBufferInFlight.push_back(commandBuffer);
			}
		}
	}
}