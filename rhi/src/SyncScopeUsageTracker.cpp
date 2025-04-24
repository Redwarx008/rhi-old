#include "SyncScopeUsageTracker.h"

#include "TextureBase.h"
#include "BindSetBase.h"
#include "BindSetLayoutBase.h"
#include "common/Constants.h"
#include "common/Error.h"

namespace rhi
{
	void SyncScopeUsageTracker::BufferUsedAs(BufferBase* buffer,
		BufferUsage usage,
		ShaderStage shaderStages)
	{
		BufferSyncInfo& bufferSyncInfo = mBufferSyncInfos[buffer];

		bufferSyncInfo.usage |= usage;
		bufferSyncInfo.shaderStages |= shaderStages;
	}

	void SyncScopeUsageTracker::TextureRangeUsedAs(TextureBase* texture,
		const SubresourceRange& range,
		TextureUsage usage,
		ShaderStage shaderStages)
	{
		Aspect formatAspects = GetAspectFromFormat(texture->APIGetFormat());
		auto it = mTextureSyncInfos.try_emplace(texture, formatAspects, texture->APIGetDepthOrArrayLayers(), texture->APIGetMipLevelCount());

		SubresourceStorage<TextureSyncInfo>& textureSyncInfo = it.first->second;
		textureSyncInfo.Update(range, [usage, shaderStages](const SubresourceRange&, TextureSyncInfo& storedSyncInfo)
			{
				storedSyncInfo.usage |= usage;
				storedSyncInfo.shaderStages |= shaderStages;
			});
	}
	
	void SyncScopeUsageTracker::TextureViewUsedAs(TextureViewBase* view,
		TextureUsage usage,
		ShaderStage shaderStages)
	{
		TextureRangeUsedAs(view->GetTexture(), view->GetSubresourceRange(), usage, shaderStages);
	}

	void SyncScopeUsageTracker::AddBindSet(BindSetBase* set)
	{
		for (const BindSetEntry& bindingEntry : set->GetBindingEntries())
		{
			BindingType type = set->GetLayout()->GetBindingType(bindingEntry.binding);
			ShaderStage visibility = set->GetLayout()->GetVisibility(bindingEntry.binding);
			switch (type)
			{
			case rhi::BindingType::CombinedTextureSampler:
			case rhi::BindingType::SampledTexture:
			{
				TextureViewUsedAs(bindingEntry.textureView, TextureUsage::SampledBinding, visibility);
				break;
			}
			case rhi::BindingType::StorageTexture:
			{
				TextureViewUsedAs(bindingEntry.textureView, TextureUsage::StorageBinding, visibility);
				break;
			}
			case rhi::BindingType::ReadOnlyStorageTexture:
			{
				TextureViewUsedAs(bindingEntry.textureView, cReadOnlyStorageTexture, visibility);
				break;
			}
			case rhi::BindingType::UniformBuffer:
			{
				BufferUsedAs(bindingEntry.buffer, BufferUsage::Uniform, visibility);
				break;
			}
			case rhi::BindingType::StorageBuffer:
			{
				BufferUsedAs(bindingEntry.buffer, BufferUsage::Storage, visibility);
				break;
			}
			case rhi::BindingType::ReadOnlyStorageBuffer:
			{
				BufferUsedAs(bindingEntry.buffer, cReadOnlyStorageBuffer, visibility);
				break;
			}
			case rhi::BindingType::Sampler:
				break;
			case rhi::BindingType::None:
			default:
				ASSERT(!"Unreachable");
				break;
			}
		}
	}

	SyncScopeResourceUsage SyncScopeUsageTracker::AcquireSyncScopeUsage()
	{
		SyncScopeResourceUsage usages;
		usages.buffers.reserve(mBufferSyncInfos.size());
		usages.bufferSyncInfos.reserve(mBufferSyncInfos.size());
		usages.textures.reserve(mTextureSyncInfos.size());
		usages.textureSyncInfos.reserve(mTextureSyncInfos.size());

		for (auto& [buffer, syncInfo] : mBufferSyncInfos)
		{
			usages.buffers.push_back(buffer);
			usages.bufferSyncInfos.push_back(std::move(syncInfo));
		}

		for (auto& [texture, syncInfo] : mTextureSyncInfos)
		{
			usages.textures.push_back(texture);
			usages.textureSyncInfos.push_back(std::move(syncInfo));
		}

		mBufferSyncInfos.clear();
		mTextureSyncInfos.clear();

		return usages;
	}
}