#pragma once

#include "rhi/RHIStruct.h"
#include "SubresourceStorage.hpp"
#include "Subresource.h"

namespace rhi
{
	class BufferBase;
	class TextureBase;
	class TextureViewBase;

	struct BufferSyncInfo
	{
		BufferUsage usage;
		ShaderStage shaderStages;
	};

	struct TextureSyncInfo
	{
		TextureUsage usage;
		ShaderStage shaderStages;
		QueueType queue;
	};

	struct SyncScopeResourceUsage
	{
		std::vector<BufferBase*> buffers;
		std::vector<BufferSyncInfo> bufferSyncInfos;

		std::vector<TextureBase*> textures;
		std::vector<SubresourceStorage<TextureSyncInfo>> textureSyncInfos;
	};

	struct CommandListResourceUsage
	{
		std::vector<SyncScopeResourceUsage> renderPassUsages;
		std::vector<SyncScopeResourceUsage> computePassUsages;
	};
}