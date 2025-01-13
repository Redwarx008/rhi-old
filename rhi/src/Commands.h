#pragma once

namespace rhi
{
	enum class Command
	{
        WirteBuffer,

        ClearBuffer,
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
        SetGraphicsPipeline,
        SetComputePipeline,
        SetViewports,
        SetScissors,
        SetIndexBuffer,
        SetVertexBuffer,
        SetPushConstant,
        BeginDebugLabel,
        EndDebugLabel,
	};
}