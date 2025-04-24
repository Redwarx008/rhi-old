#pragma once

#include "rhi/RHIStruct.h"
#include "common/Ref.hpp"
#include "common/RefCounted.h"

namespace rhi
{
	class SwapChainBase : public RefCounted
	{
	public:
		explicit SwapChainBase(DeviceBase* device, Surface* surface, const SurfaceConfiguration& config);
		~SwapChainBase();
		virtual SurfaceAcquireNextTextureStatus AcquireNextTexture() = 0;
		virtual Ref<TextureBase> GetCurrentTexture() = 0;
		virtual void Present() = 0;

	protected:
		Ref<DeviceBase> mDevice;
		Surface* mSurface;

		uint32_t mHeight;
		uint32_t mWidth;
		TextureFormat mFormat;
		TextureUsage mUsage;
		PresentMode mPresentMode;
	};
}