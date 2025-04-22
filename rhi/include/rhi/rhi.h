#pragma once
#include <cstdint>
#include "RHIStruct.h"

namespace rhi
{
	class Object
	{
	public:
		Object(uint64_t i)
		{
			integer = i;
		}
		Object(void* p)
		{
			pointer = p;
		}
		template<typename T> operator T* () const { return static_cast<T*>(pointer); }
	private:
		union
		{
			uint64_t integer;
			void* pointer;
		};
	};
	 

	class ITextureView : public IRefCounted
	{
	public:
		virtual ITexture* GetTexture() const = 0;
		virtual uint32_t GetBaseMipLevel() const = 0;
		virtual uint32_t GetLevelCount() const = 0;
		virtual uint32_t GetBaseArrayLayer() const = 0;
		virtual uint32_t GetLayerCount() const = 0;
	protected:
		virtual ~ITextureView() = default;
	};

	class ITexture : public IRefCounted
	{
	public:
		virtual ITextureView* getDefaultView() const = 0;
		virtual ITextureView* createView(TextureViewDesc desc) = 0;
		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetDepthOrArrayLayers() const = 0;
		virtual uint32_t GetMipLevelCount() const = 0;
		virtual uint32_t GetSampleCount() const = 0;
		virtual TextureDimension GetDimension() const = 0;
		virtual TextureFormat GetFormat() const = 0;
		virtual TextureUsage GetUsage() const = 0;
	protected:
		virtual ~ITexture() = default;
	};

	class ISampler : public IRefCounted
	{
	protected:
		virtual ~ISampler() = default;
	};

	class IShader : public IRefCounted
	{
	protected:
		~IShader() = default;
	};

	class IBindSetLayout : public IRefCounted
	{
	protected:
		virtual ~IBindSetLayout() = default;
	};

	class IBindSet : public IRefCounted
	{
	protected:
		~IBindSet() = default;
	};

	class IPipeline : public IRefCounted
	{
	public:
		virtual IBindSetLayout* GetResourceSetLayout(uint32_t setIndex) = 0;
	protected:
		virtual ~IPipeline() = default;
	};

	class IRenderPipeline : public IPipeline
	{
	protected:
		virtual ~IRenderPipeline() = default;
	};

	class IComputePipeline : public IPipeline
	{
	protected:
		~IComputePipeline() = default;
	};

	class ICommandList : public IRefCounted
	{
	public:
		virtual void Wait(ICommandList* other) = 0;
	protected:
		~ICommandList() = default;
	};

	class ICommandEncoder : public IRefCounted
	{
	public:
		virtual void ClearColorTexture(ITextureView* textureView, const ClearColor& color) = 0;
		virtual void ClearDepthStencil(ITextureView* textureView, ClearDepthStencilFlag flag, float depthVal, uint8_t stencilVal) = 0;
		virtual void ClearBuffer(IBuffer* buffer, uint32_t value, uint64_t offset = 0, uint64_t size = ~0ull) = 0;
		virtual void CopyBufferToBuffer(IBuffer* srcBuffer, uint64_t srcOffset, IBuffer* dstBuffer, uint64_t dstOffset, uint64_t dataSize) = 0;
		virtual void CopyTextureToBuffer();
		virtual void MapBufferAsync(IBuffer* buffer, MapMode usage, BufferMapCallback callback, void* userData) = 0;
		virtual void commitShaderResources(IBindSetLayout* shaderResourceBinding) = 0;




		virtual IRenderPassEncoder* BeginRenderPass(const RenderPassDesc& renderPassDesc) = 0;

		virtual void setPushConstant(ShaderStage stages, const void* data) = 0;






		virtual ICommandList* Finish() = 0;
		virtual void APIBeginDebugLabel(const char* labelName, Color color = Color()) = 0;
		virtual void APIEndDebugLabel() = 0;
	protected:
		virtual ~ICommandEncoder() = default;
	};

	class IRenderPassEncoder : public IRefCounted
	{
	public:
		virtual void APISetPipeline(const IRenderPipeline* pipeline) = 0;
		virtual void SetVertexBuffer(uint32_t startSlot, uint32_t bufferCount, IBuffer* buffers, uint64_t* offsets) = 0;
		virtual void SetIndexBuffer(IBuffer* indexBuffer, uint64_t offset, TextureFormat indexFormat) = 0;
		virtual void SetScissorRects(const Rect* scissors, uint32_t scissorCount) = 0;
		virtual void SetStencilReference(uint32_t reference) = 0;
		virtual void SetBlendConstant(const Color& blendConstants) = 0;
		virtual void SetBindGroup() = 0;
		virtual void SetShaderResourceBinding(const IBindSetLayout* srb) = 0;
		virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
		virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;
		virtual void DrawIndirect(IBuffer* argsBuffer, uint64_t offset, uint32_t drawCount) = 0;
		virtual void DrawIndexedIndirect(IBuffer* argsBuffer, uint64_t offset, uint32_t drawCount) = 0;
		virtual void End() = 0;
		virtual void SetLabel(const char* label) = 0;
	};

	class IComputePassEncoder : public IRefCounted
	{
	public:
		virtual void setPipeline(const IComputePipeline* pipeline) = 0;
		virtual void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
		virtual void dispatchIndirect(IBuffer* argsBuffer, uint64_t offset) = 0;
	};

	class IQueue : public IRefCounted
	{
	public:
		virtual ICommandEncoder* CreateCommandEncoder() = 0;
		virtual void WriteBuffer(IBuffer* buffer, const void* data, uint64_t dataSize, uint64_t offset) = 0;
		virtual void WriteTexture(const TextureCopy& destination, const void* data, size_t dataSize, const TextureDataLayout& dataLayout) = 0;
		virtual void WaitQueue(IQueue* queue, uint64_t submitID) = 0;
		virtual uint64_t Submit(const ICommandList** commands, size_t commandListCount) = 0;
	protected:
		virtual ~IQueue() = default;
	};

	class IDevice : public IRefCounted
	{
	public:
		virtual void WaitIdle() = 0;
		virtual ISwapChain* CreateSwapChain(const SwapChainDesc& desc) = 0;
		virtual IQueue* GetQueue(QueueType queueType) = 0;
		virtual IRenderPipeline* CreateGraphicsPipeline(const RenderPipelineDesc& pipelineCI) = 0;
		virtual IComputePipeline* CreateComputePipeline(const ComputePipelineDesc& pipelineCI) = 0;
		virtual IBindSetLayout* CreateBindSetLayout(const BindSetLayoutDesc& desc) = 0;
		virtual IBindSetLayout* CreateBindSet(const BindSetDesc& desc) = 0;
		virtual ITexture* CreateTexture(const TextureDesc& desc) = 0;
		virtual IBuffer* CreateBuffer(const BufferDesc& desc) = 0;
		virtual IBuffer* CreateBuffer(const BufferDesc& desc, const void* data, uint64_t dataSize) = 0;
		virtual IShader* CreateShader(const ShaderModuleDesc& desc) = 0;
		virtual ISampler* CreateSampler(const SamplerDesc& desc) = 0;
	protected:
		virtual ~IDevice() = default;
	};

	class IAdapter : public IRefCounted
	{
	public:
		virtual IDevice* CreateDevice(const DeviceDesc& desc) = 0;
		virtual AdapterInfo GetInfo() const = 0;
		virtual Limits GetLimits() const = 0;

	protected:
		~IAdapter() = default;
	};

	class IInstance : public IRefCounted
	{
	public:
		virtual void EnumerateAdapters(IAdapter** const adapters, uint32_t* adapterCount) = 0;
	};

	IInstance* CreateInstance(const InstanceDesc& desc);

	class ISwapChain : public IRefCounted
	{
	public:
		virtual ~ISwapChain() = default;

	};
}