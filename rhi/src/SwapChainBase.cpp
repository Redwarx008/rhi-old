#include "SwapchainBase.h"

namespace rhi
{
	SwapChainBase::SwapChainBase(DeviceBase* device, Surface* surface, const SurfaceConfiguration& config) :
		mDevice(device),
		mWidth(config.width),
		mHeight(config.height),
		mFormat(config.format),
		mPresentMode(config.presentMode)
	{

	}

	SwapChainBase::~SwapChainBase() {}
}