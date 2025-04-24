#pragma once

#include "rhi/RHIStruct.h"
#include "common/Ref.hpp"
#include "common/RefCounted.h"

namespace rhi
{
	class Surface final : public RefCounted
	{
	public:
		static Ref<Surface> CreateFromWindowsHWND(InstanceBase* instance, void* hwnd, void* hinstance);
		//static Ref<Surface> CreateFromWaylandSurface(InstanceBase* instance, void* display, void* surface);
		void APIConfigure(const SurfaceConfiguration& config);
		SurfaceAcquireNextTextureStatus APIAcquireNextTexture();
		TextureBase* APIGetCurrentTexture();
		void APIPresent();
		void APIUnconfigure();

		InstanceBase* GetInstance() const;
		void* GetHInstance() const;
		void* GetHWND() const;
	private:
		explicit Surface(InstanceBase* instance);
		~Surface();
		Ref<DeviceBase> mDevice;
		InstanceBase* mInstance;
		Ref<SwapChainBase> mSwapChain;

		// WindowsHwnd
		void* mHWND = nullptr;
		void* mHInstance = nullptr;
	};
}