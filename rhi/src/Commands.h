#pragma once

#include <array>
#include <string_view>
#include "CommandAllocator.h"
#include "common/Constants.h"

namespace rhi
{
	enum class Command
	{
        ClearBuffer,
        BeginRenderPass,
        BeginComputePass,
        BeginDebugLabel,
        CopyBufferToBuffer,
        CopyBufferToTexture,
        CopyTextureToBuffer,
        CopyTextureToTexture,
        Dispatch,
        DispatchIndirect,
        Draw,
        DrawIndexed,
        DrawIndirect,
        DrawIndexedIndirect,
        MultiDrawIndirect,
        MultiDrawIndexedIndirect,
        SetRenderPipeline,
        SetComputePipeline,
        SetViewport,
        SetScissorRect,
        SetIndexBuffer,
        SetVertexBuffer,
        SetPushConstant,
        SetStencilReference,
        SetBlendConstant,
        SetBindSet,
        EndRenderPass,
        EndComputePass,
        EndDebugLabel,
        MapBufferAsync
	};


    struct RenderPassColorAttachment
    {
        Ref<TextureViewBase> view;
        Ref<TextureViewBase> resolveView;
        LoadOp loadOp;
        StoreOp storeOp;
        Color clearColor;
    };


    struct RenderPassDepthStencilAttachment
    {
        Ref<TextureViewBase> view;
        LoadOp depthLoadOp;
        StoreOp depthStoreOp;
        LoadOp stencilLoadOp;
        StoreOp stencilStoreOp;
        float depthClearValue;
        uint32_t stencilClearValue;
    };


    struct BeginRenderPassCmd
    {
        BeginRenderPassCmd() = default;
        ~BeginRenderPassCmd() = default;
        std::array<RenderPassColorAttachment, cMaxColorAttachments> colorAttachments;
        RenderPassDepthStencilAttachment depthStencilAttachment;
        uint8_t colorAttachmentCount;
    };


    struct BeginComputePassCmd
    {
        BeginComputePassCmd() = default;
        ~BeginComputePassCmd() = default;
        std::string label;
    };

    struct ClearBufferCmd 
    {
        ClearBufferCmd() = default;
        ~ClearBufferCmd() = default;

        Ref<BufferBase> buffer;
        uint32_t value;
        uint64_t offset;
        uint64_t size;
    };

    struct CopyBufferToBufferCmd
    {
        CopyBufferToBufferCmd() = default;
        ~CopyBufferToBufferCmd() = default;

        Ref<BufferBase> srcBuffer;
        uint64_t srcOffset;
        Ref<BufferBase> dstBuffer;
        uint64_t dstOffset;
        uint64_t size;
    };

    struct CopyBufferToTextureCmd
    {
        CopyBufferToTextureCmd() = default;
        ~CopyBufferToTextureCmd() = default;

        Ref<BufferBase> srcBuffer;
        TextureDataLayout dataLayout;
        Ref<TextureBase> dstTexture;
        Origin3D origin;
        Extent3D size;
        uint32_t mipLevel = 0;
        Aspect aspect;
    };

    struct CopyTextureToBufferCmd
    {
        CopyTextureToBufferCmd() = default;
        ~CopyTextureToBufferCmd() = default;

        Ref<TextureBase> srcTexture;
        TextureDataLayout dataLayout;
        Origin3D origin;
        Extent3D size;
        uint32_t mipLevel = 0;
        Aspect aspect;
        Ref<BufferBase> dstBuffer;
    };

    struct CopyTextureToTextureCmd
    {
        CopyTextureToTextureCmd() = default;
        ~CopyTextureToTextureCmd() = default;

        Ref<TextureBase> srcTexture;
        uint32_t srcMipLevel;
        Origin3D srcOrigin;
        Extent3D srcSize;
        Aspect srcAspect;

        Ref<TextureBase> dstTexture;
        uint32_t dstMipLevel;
        Origin3D dstOrigin;
        Extent3D dstSize;
        Aspect dstAspect;
    };

    struct MapBufferAsyncCmd
    {
        MapBufferAsyncCmd() = default;
        ~MapBufferAsyncCmd() = default;

        Ref<BufferBase> buffer;
        MapMode mapMode;
        BufferMapCallback callback;
        void* userData;
    };

    struct BeginDebugLabelCmd
    {
        BeginDebugLabelCmd() = default;
        ~BeginDebugLabelCmd() = default;
        
        uint32_t labelLength;
        Color color;
    };

    struct EndDebugLabelCmd
    {
        EndDebugLabelCmd() = default;
        ~EndDebugLabelCmd() = default;
    };

    inline const char* EnsureValidString(CommandAllocator& allocator, std::string_view s, uint32_t* length) {
        *length = s.length() + 1;

        // Include extra null-terminator character. The string_view may not be null-terminated. It also
        // may already have a null-terminator inside of it, in which case adding the null-terminator is
        // unnecessary. However, this is unlikely, so always include the extra character.
        char* out = allocator.AllocateData<char>(s.length() + 1);
        memcpy(out, s.data(), s.length());
        out[s.length()] = '\0';

        return out;
    }

    struct SetRenderPipelineCmd
    {
        SetRenderPipelineCmd() = default;
        ~SetRenderPipelineCmd() = default;

        Ref<RenderPipelineBase> pipeline;
    };

    struct VertexBuffer
    {
        Ref<BufferBase> buffer;
        uint64_t offset;
    };

    struct SetVertexBufferCmd
    {
        SetVertexBufferCmd() = default;
        ~SetVertexBufferCmd() = default;

        std::array<VertexBuffer, cMaxVertexBuffers> buffers;
        uint32_t bufferCount;
        uint32_t firstSlot;
    };

    struct SetIndexBufferCmd
    {
        SetIndexBufferCmd() = default;
        ~SetIndexBufferCmd() = default;

        Ref<BufferBase> buffer;
        IndexFormat format;
        uint64_t offset;
        uint64_t size;
    };

    struct SetScissorRectsCmd
    {
        SetScissorRectsCmd() = default;
        ~SetScissorRectsCmd() = default;

        std::array<Rect, cMaxColorAttachments> scissors;
        uint32_t scissorCount;
        uint32_t firstScissor;
    };

    struct SetStencilReferenceCmd
    {
        uint32_t reference;
    };

    struct SetBlendConstantCmd
    {
        Color color;
    };

    struct SetBindSetCmd
    {
        SetBindSetCmd() = default;
        ~SetBindSetCmd() = default;

        Ref<BindSetBase> set;
        uint32_t setIndex;
        uint32_t dynamicOffsetCount;
    };

    struct SetPushConstantCmd
    {
        SetPushConstantCmd() = default;
        ~SetPushConstantCmd() = default;
        uint32_t size;
    };


    struct DrawCmd 
    {
        uint32_t vertexCount;
        uint32_t instanceCount;
        uint32_t firstVertex;
        uint32_t firstInstance;
    };

    struct DrawIndexedCmd 
    {
        uint32_t indexCount;
        uint32_t instanceCount;
        uint32_t firstIndex;
        int32_t baseVertex;
        uint32_t firstInstance;
    };

    struct DrawIndirectCmd 
    {
        DrawIndirectCmd() = default;
        ~DrawIndirectCmd() = default;

        Ref<BufferBase> indirectBuffer;
        uint64_t indirectOffset;
    };

    struct DrawIndexedIndirectCmd : DrawIndirectCmd
    {
    };

    struct MultiDrawIndirectCmd
    {
        MultiDrawIndirectCmd() = default;
        ~MultiDrawIndirectCmd() = default;

        Ref<BufferBase> indirectBuffer;
        uint64_t indirectOffset;
        uint32_t maxDrawCount;
        Ref<BufferBase> drawCountBuffer;
        uint64_t drawCountOffset;
    };

    struct MultiDrawIndexedIndirectCmd : MultiDrawIndirectCmd
    {
    };

    struct EndRenderPassCmd
    {

    };

    struct SetViewportCmd
    {
        SetViewportCmd() = default;
        ~SetViewportCmd() = default;

        std::array<Viewport, cMaxColorAttachments> viewports;
        uint32_t viewportCount;
        uint32_t firstViewport;
    };
}