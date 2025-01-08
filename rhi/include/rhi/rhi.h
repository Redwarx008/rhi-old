#pragma once

#include <atomic>
#include <cstdint>
#include "rhi/common/Utils.h"
#include "rhi/common/RefCounted.h"
#include "rhi_struct.h"

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



	class IObject
	{
	public:
		virtual ~IObject() = default;
		virtual Object getNativeObject(NativeObjectType type) const = 0;
	};

	class IBuffer : public RefCounted
	{
	public:
		virtual BufferUsage GetUsage() const = 0;
		virtual uint64_t GetSize() const = 0;
		virtual void MapAsync(MapMode mode, BufferMapCallback callback, void* userData) = 0;
	protected:
		virtual ~IBuffer() = default;
	};

	class ITextureView : public IObject
	{
	public:
		virtual ~ITextureView() = default;
		virtual const TextureViewDesc& getDesc() const = 0;
		virtual ITexture* getTexture() const = 0;
	};

	class ITexture : public RefCounted
	{
	public:
		virtual ~ITexture() = default;
		virtual const TextureDesc& getDesc() const = 0;
		virtual ITextureView* getDefaultView() const = 0;
		virtual ITextureView* createView(TextureViewDesc desc) = 0;
	};

	class ISampler
	{
	public:
		virtual ~ISampler() = default;
		virtual const SamplerDesc& getDesc() const = 0;
	};

	class IShader
	{
	public:
		~IShader() = default;
		virtual const ShaderDesc& getDesc() const = 0;
	};

	class IShaderResourceBinding
	{
	public:
		virtual ~IShaderResourceBinding() = default;
		virtual void bindBuffer(IBuffer* buffer, uint32_t slot, uint32_t set = 0, uint64_t offset = 0, uint64_t range = ~0ull) = 0;
		virtual void bindTexture(ITextureView* textureView, uint32_t slot, uint32_t set = 0) = 0;
		virtual void bindSampler(ISampler* sampler, uint32_t slot, uint32_t set = 0) = 0;
		virtual void bindTextureWithSampler(ITextureView* textrueView, ISampler* sampler, uint32_t slot, uint32_t set = 0) = 0;
	};

	class IPipeline : public IObject
	{
	public:
		virtual ~IPipeline() = default;
		virtual IShaderResourceBinding* createShaderResourceBinding() = 0;
	};

	class IGraphicsPipeline : public IPipeline
	{
	public:
		virtual ~IGraphicsPipeline() = default;

		virtual const GraphicsPipelineDesc& getDesc() const = 0;
	};

	class IComputePipeline : public IPipeline
	{
	public:
		~IComputePipeline() = default;
	};

	class ICommandList : public IObject
	{
	public:
		virtual ~ICommandList() = default;

		virtual void open() = 0;
		virtual void close() = 0;

		virtual void waitCommandList(ICommandList* other) = 0;

		virtual void setResourceAutoTransition(bool enable) = 0;
		virtual void commitBarriers() = 0;
		virtual void transitionTextureState(ITexture* texture, ResourceState newState) = 0;
		virtual void transitionBufferState(IBuffer* buffer, ResourceState newState) = 0;

		virtual void clearColorTexture(ITextureView* textureView, const ClearColor& color) = 0;
		virtual void clearDepthStencil(ITextureView* textureView, ClearDepthStencilFlag flag, float depthVal, uint8_t stencilVal) = 0;
		virtual void clearBuffer(IBuffer* buffer, uint32_t value, uint64_t offset = 0, uint64_t size = ~0ull) = 0;
		virtual void updateBuffer(IBuffer* buffer, const void* data, uint64_t dataSize, uint64_t offset) = 0;
		virtual void copyBuffer(IBuffer* srcBuffer, uint64_t srcOffset, IBuffer* dstBuffer, uint64_t dstOffset, uint64_t dataSize) = 0;
		virtual void* mapBuffer(IBuffer* buffer, MapMode usage) = 0;
		virtual void updateTexture(ITexture* texture, const void* data, uint64_t dataSize, const TextureUpdateInfo& updateInfo) = 0;

		virtual void commitShaderResources(IShaderResourceBinding* shaderResourceBinding) = 0;

		virtual void setPipeline(const IGraphicsPipeline* pipeline) = 0;
		virtual void setPipeline(const IComputePipeline* pipeline) = 0;

		virtual void setPushConstant(ShaderStage stages, const void* data) = 0;
		virtual void setVertexBuffer(uint32_t startSlot, uint32_t bufferCount, IBuffer* buffers, uint64_t* offsets) = 0;
		virtual void setIndexBuffer(IBuffer* indexBuffer, uint64_t offset, Format indexFormat) = 0;
		virtual void setScissors(const Rect* scissors, uint32_t scissorCount) = 0;

		virtual void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
		virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;
		virtual void drawIndirect(IBuffer* argsBuffer, uint64_t offset, uint32_t drawCount) = 0;
		virtual void drawIndexedIndirect(IBuffer* argsBuffer, uint64_t offset, uint32_t drawCount) = 0;

		virtual void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
		virtual void dispatchIndirect(IBuffer* argsBuffer, uint64_t offset) = 0;

		virtual void beginDebugLabel(const char* labelName, Color color = Color()) = 0;
		virtual void endDebugLabel() = 0;
	};

	class IDevice
	{
	public:
		virtual ~IDevice() = default;
		virtual void present() = 0;
		virtual void resizeSwapChain() = 0;
		virtual ITextureView* getCurrentRenderTargetView() = 0;
		virtual ITextureView* getDepthStencilView() = 0;
		virtual Format getRenderTargetFormat() = 0;
		virtual Format getDepthStencilFormat() = 0;
		virtual void waitIdle() = 0;
		virtual void createSwapChain(const SwapChainCreateInfo& swapChainCI) = 0;
		virtual void recreateSwapChain() = 0;
		virtual IGraphicsPipeline* createGraphicsPipeline(const GraphicsPipelineCreateInfo& pipelineCI) = 0;
		virtual IComputePipeline* createComputePipeline(const ComputePipelineCreateInfo& pipelineCI) = 0;
		virtual IResourceSetLayout* createResourceSetLayout(const ResourceSetLayoutBinding* bindings, uint32_t bindingCount) = 0;
		virtual IResourceSet* createResourceSet(const IResourceSetLayout* layout, const ResourceSetBinding* bindings, uint32_t bindingCount) = 0;
		virtual void updateResourceSet(IResourceSet* set, const ResourceSetBinding* bindings, uint32_t bindingCount) = 0;
		virtual ITexture* createTexture(const TextureDesc& desc) = 0;
		virtual IBuffer* createBuffer(const BufferDesc& desc) = 0;
		virtual IBuffer* createBuffer(const BufferDesc& desc, const void* data, uint64_t dataSize) = 0;
		virtual IShader* createShader(const ShaderCreateInfo& shaderCI, const uint32_t* pCode, size_t codeSize) = 0;
		virtual ISampler* createSampler(const SamplerDesc& desc) = 0;
		virtual void* mapBuffer(IBuffer* buffer) = 0;
		virtual ICommandList* beginCommandList(QueueType queueType = QueueType::Graphics) = 0;
		virtual uint64_t executeCommandLists(ICommandList** cmdLists, size_t numCmdLists) = 0;
		virtual void waitForExecution(uint64_t executeID, uint64_t timeout = UINT64_MAX) = 0;
	};


    IDevice* createDevice(const DeviceCreateInfo& createInfo);

	class ISwapChain
	{
	public:
		virtual ~ISwapChain() = default;

	};
}