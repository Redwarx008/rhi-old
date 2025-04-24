#include "Surface.h"
#include "DeviceBase.h"
#include "SwapchainBase.h"
#include "common/Error.h"

namespace rhi
{
	Surface::Surface(InstanceBase* instance) :
		mInstance(instance)
	{

	}

	Surface::~Surface()
	{

	}

	Ref<Surface> Surface::CreateFromWindowsHWND(InstanceBase* instance, void* hwnd, void* hinstance)
	{
		Ref<Surface> surface = AcquireRef(new Surface(instance));
		surface->mHWND = hwnd;
		surface->mHInstance = hinstance;
		return surface;
	}

	void Surface::APIConfigure(const SurfaceConfiguration& config)
	{
		mSwapChain = mDevice->CreateSwapChain(this, mSwapChain.Get(), config);
	}

	void Surface::APIUnconfigure()
	{
		mSwapChain = nullptr;
	}

	SurfaceAcquireNextTextureStatus Surface::APIAcquireNextTexture()
	{
		ASSERT(mSwapChain != nullptr);
		return mSwapChain->AcquireNextTexture();
	}

	TextureBase* Surface::APIGetCurrentTexture()
	{
		ASSERT(mSwapChain != nullptr);
		return mSwapChain->GetCurrentTexture().Detach();
	}

	void Surface::APIPresent()
	{
		ASSERT(mSwapChain != nullptr);
		mSwapChain->Present();
	}

	InstanceBase* Surface::GetInstance() const
	{
		return mInstance;
	}

	void* Surface::GetHInstance() const
	{
		return mHInstance;
	}

	void* Surface::GetHWND() const
	{
		return mHWND;
	}
}