#pragma once

#include <cstdint>
#include "rhi/common/Utils.h"
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

	class IResource : public IObject
	{
	public:
		virtual ResourceState getState() const = 0;
	};

	class IBuffer : public IResource
	{
	public:
		virtual ~IBuffer() = default;
		virtual const BufferDesc& getDesc() const = 0;
	};

	class ITextureView : public IObject
	{
	public:
		virtual ~ITextureView() = default;
		virtual const TextureViewDesc& getDesc() const = 0;
		virtual ITexture* getTexture() const = 0;
	};

	class ITexture : public IResource
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

	class IShader : public IObject
	{
	public:
		~IShader() = default;
		virtual const ShaderDesc& getDesc() const = 0;
	};

	class IResourceSetLayout : public IObject
	{
	public:
		virtual ~IResourceSetLayout() = default;
	};

	class IResourceSet : public IObject
	{
	public:
		virtual ~IResourceSet() = default;
	};

	class IPipeline : public IObject
	{
	public:
		virtual ~IPipeline() = default;
		virtual bool getPipelineCacheData(void* pData, size_t* pDataSize) const = 0;
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

		virtual void setResourceAutoTransition(bool enable) = 0;
		virtual void commitBarriers() = 0;
		virtual void transitionTextureState(ITexture* texture, ResourceState newState) = 0;
		virtual void transitionBufferState(IBuffer* buffer, ResourceState newState) = 0;
		virtual void transitionResourceSet(IResourceSet* resourceSet) = 0;

		virtual void clearColorTexture(ITextureView* textureView, const ClearColor& color) = 0;
		virtual void clearDepthStencil(ITextureView* textureView, ClearDepthStencilFlag flag, float depthVal, uint8_t stencilVal) = 0;
		virtual void clearBuffer(IBuffer* buffer, uint32_t value, uint64_t offset = 0, uint64_t size = ~0ull) = 0;
		virtual void updateBuffer(IBuffer* buffer, const void* data, uint64_t dataSize, uint64_t offset) = 0;
		virtual void copyBuffer(IBuffer* srcBuffer, uint64_t srcOffset, IBuffer* dstBuffer, uint64_t dstOffset, uint64_t dataSize) = 0;
		virtual void* mapBuffer(IBuffer* buffer, MapBufferUsage usage) = 0;
		virtual void updateTexture(ITexture* texture, const void* data, uint64_t dataSize, const TextureUpdateInfo& updateInfo) = 0;

		virtual void commitResourceSet(IResourceSet* resourceSet, uint32_t dstSet = 0) = 0;

		virtual void setPushConstant(ShaderType stages, const void* data) = 0;
		virtual void setScissors(const Rect* scissors, uint32_t scissorCount) = 0;
		virtual void setGraphicsState(const GraphicsState& state) = 0;

		virtual void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
		virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;
		virtual void drawIndirect(IBuffer* argsBuffer, uint64_t offset, uint32_t drawCount) = 0;
		virtual void drawIndexedIndirect(IBuffer* argsBuffer, uint64_t offset, uint32_t drawCount) = 0;

		virtual void setComputeState(const ComputeState& state) = 0;
		virtual void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
		virtual void dispatchIndirect(IBuffer* argsBuffer, uint64_t offset) = 0;

		virtual void beginDebugLabel(const char* labelName, Color color = Color()) = 0;
		virtual void endDebugLabel() = 0;
	};

	class IRenderDevice
	{
	public:
		virtual ~IRenderDevice() = default;
		virtual void waitIdle() = 0;
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

	class ISwapChain
	{
	public:
		virtual ~ISwapChain() = default;
		virtual void beginFrame() = 0;
		virtual void present() = 0;
		virtual void resize() = 0;
		virtual ITextureView* getCurrentRenderTargetView() = 0;
		virtual ITextureView* getDepthStencilView() = 0;
		virtual Format getRenderTargetFormat() = 0;
		virtual Format getDepthStencilFormat() = 0;
	};

    IRenderDevice* createRenderDevice(const RenderDeviceCreateInfo& createInfo);

    ISwapChain* createSwapChain(const SwapChainCreateInfo& createInfo);
}