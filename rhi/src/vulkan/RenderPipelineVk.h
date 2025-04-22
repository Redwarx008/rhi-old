#pragma once

#include "../RenderPipelinebase.h"
#include <vulkan/vulkan.h>

namespace rhi::vulkan
{
	class Device;
	class RenderPipeline final : public RenderPipelineBase
	{
	public:
		static Ref<RenderPipeline> Create(Device* device, const RenderPipelineDesc& desc);
		VkPipeline GetHandle() const;
	private:
		explicit RenderPipeline(Device* device, const RenderPipelineDesc& desc) noexcept;
		~RenderPipeline();
		bool Initialize();
		void DestroyImpl() override;

		VkPipeline mHandle = VK_NULL_HANDLE;
	};
}