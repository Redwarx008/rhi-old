#include "Subresource.h"

#include "common/Utils.h"

namespace rhi
{
    SubresourceRange::SubresourceRange(Aspect _aspects, uint32_t _baseArrayLayer, uint32_t _layerCount, uint32_t _baseMipLevel, uint32_t _levelCount)
        :
        aspects(_aspects),
        baseArrayLayer(_baseArrayLayer),
        layerCount(_layerCount),
        baseMipLevel(_baseArrayLayer),
        levelCount(_levelCount)
    {

    }

	SubresourceRange SubresourceRange::MakeSingle(Aspect aspect, uint32_t baseArrayLayer, uint32_t baseMipLevel)
	{
        SubresourceRange res;
        res.aspects = aspect;
        res.baseMipLevel = baseMipLevel;
        res.levelCount = 1;
        res.baseArrayLayer = baseArrayLayer;
        res.layerCount = 1;

        return res;
	}

    SubresourceRange SubresourceRange::MakeFull(Aspect aspects, uint32_t layerCount, uint32_t levelCount)
    {
        SubresourceRange res;
        res.aspects = aspects;
        res.baseMipLevel = 0;
        res.levelCount = levelCount;
        res.baseArrayLayer = 0;
        res.layerCount = layerCount;

        return res;
    }

    uint8_t GetAspectIndex(Aspect aspect)
    {
        assert(HasOneFlag(aspect));
        switch (aspect)
        {
        case rhi::Aspect::Color:
        case rhi::Aspect::Depth:
        case rhi::Aspect::Plane0:
            return 0;
        case rhi::Aspect::Stencil:
        case rhi::Aspect::Plane1:
            return 1;
        case rhi::Aspect::Plane2:
            return 2;
        default:
            assert(!"unreachable");
        }
    }

    Aspect GetAspectFromFormat(TextureFormat format)
    {
        switch (format)
        {
        case rhi::TextureFormat::R8_UINT:
        case rhi::TextureFormat::R8_SINT:
        case rhi::TextureFormat::R8_UNORM:
        case rhi::TextureFormat::R8_SNORM:
        case rhi::TextureFormat::RG8_UINT:
        case rhi::TextureFormat::RG8_SINT:
        case rhi::TextureFormat::RG8_UNORM:
        case rhi::TextureFormat::RG8_SNORM:
        case rhi::TextureFormat::R16_UINT:
        case rhi::TextureFormat::R16_SINT:
        case rhi::TextureFormat::R16_UNORM:
        case rhi::TextureFormat::R16_SNORM:
        case rhi::TextureFormat::R16_FLOAT:
        case rhi::TextureFormat::BGRA4_UNORM:
        case rhi::TextureFormat::B5G6R5_UNORM:
        case rhi::TextureFormat::B5G5R5A1_UNORM:
        case rhi::TextureFormat::RGBA8_UINT:
        case rhi::TextureFormat::RGBA8_SINT:
        case rhi::TextureFormat::RGBA8_UNORM:
        case rhi::TextureFormat::RGBA8_SNORM:
        case rhi::TextureFormat::BGRA8_UNORM:
        case rhi::TextureFormat::RGBA8_SRGB:
        case rhi::TextureFormat::BGRA8_SRGB:
        case rhi::TextureFormat::R10G10B10A2_UNORM:
        case rhi::TextureFormat::R11G11B10_FLOAT:
        case rhi::TextureFormat::RG16_UINT:
        case rhi::TextureFormat::RG16_SINT:
        case rhi::TextureFormat::RG16_UNORM:
        case rhi::TextureFormat::RG16_SNORM:
        case rhi::TextureFormat::RG16_FLOAT:
        case rhi::TextureFormat::R32_UINT:
        case rhi::TextureFormat::R32_SINT:
        case rhi::TextureFormat::R32_FLOAT:
        case rhi::TextureFormat::RGBA16_UINT:
        case rhi::TextureFormat::RGBA16_SINT:
        case rhi::TextureFormat::RGBA16_FLOAT:
        case rhi::TextureFormat::RGBA16_UNORM:
        case rhi::TextureFormat::RGBA16_SNORM:
        case rhi::TextureFormat::RG32_UINT:
        case rhi::TextureFormat::RG32_SINT:
        case rhi::TextureFormat::RG32_FLOAT:
        case rhi::TextureFormat::RGB32_UINT:
        case rhi::TextureFormat::RGB32_SINT:
        case rhi::TextureFormat::RGB32_FLOAT:
        case rhi::TextureFormat::RGBA32_UINT:
        case rhi::TextureFormat::RGBA32_SINT:
        case rhi::TextureFormat::RGBA32_FLOAT:
        case rhi::TextureFormat::BC1_UNORM:
        case rhi::TextureFormat::BC1_UNORM_SRGB:
        case rhi::TextureFormat::BC2_UNORM:
        case rhi::TextureFormat::BC2_UNORM_SRGB:
        case rhi::TextureFormat::BC3_UNORM:
        case rhi::TextureFormat::BC3_UNORM_SRGB:
        case rhi::TextureFormat::BC4_UNORM:
        case rhi::TextureFormat::BC4_SNORM:
        case rhi::TextureFormat::BC5_UNORM:
        case rhi::TextureFormat::BC5_SNORM:
        case rhi::TextureFormat::BC6H_UFLOAT:
        case rhi::TextureFormat::BC6H_SFLOAT:
        case rhi::TextureFormat::BC7_UNORM:
        case rhi::TextureFormat::BC7_UNORM_SRGB:
            return Aspect::Color;
        case rhi::TextureFormat::D16_UNORM:
        case rhi::TextureFormat::D32_UNORM:
            return Aspect::Depth;
        case rhi::TextureFormat::D24_UNORM_S8_UINT:
        case rhi::TextureFormat::D32_UNORM_S8_UINT:
            return Aspect::Depth | Aspect::Stencil;
        default:
            assert(!"Invalid Enumeration Value");
        } // todo: add  multi-planar formats support.
    }

    uint8_t GetAspectCount(Aspect aspects)
    {
        if (aspects == Aspect::Stencil)
        {
            // Fake a the existence of a depth aspect so that the stencil data stays at index 1.
            assert(GetAspectIndex(Aspect::Stencil) == 1);
            return 2;
        }
        return PopCount(static_cast<uint8_t>(aspects));
    }

    Aspect AspectConvert(TextureFormat format, TextureAspect aspect)
    {
        Aspect aspectsFromFormat = GetAspectFromFormat(format);
        switch (aspect)
        {
        case rhi::TextureAspect::All:
            return aspectsFromFormat;
        case rhi::TextureAspect::Depth:
            return aspectsFromFormat & Aspect::Depth;
        case rhi::TextureAspect::Stencil:
            return aspectsFromFormat & Aspect::Stencil;
        case rhi::TextureAspect::Plane0:
            return aspectsFromFormat & Aspect::Plane0;
        case rhi::TextureAspect::Plane1:
            return aspectsFromFormat & Aspect::Plane1;
        case rhi::TextureAspect::Plane2:
            return aspectsFromFormat & Aspect::Plane2;
        default:
            break;
        }
        assert(!"Unreachable");
    }

    Aspect ViewAspectConvert(TextureFormat format, TextureAspect aspect)
    {
        // Color view |format| must be treated as the same plane |aspect|.
        if (GetAspectFromFormat(format) == Aspect::Color)
        {
            switch (aspect)
            {
            case rhi::TextureAspect::Plane0:
                return Aspect::Plane0;
            case rhi::TextureAspect::Plane1:
                return Aspect::Plane1;
            case rhi::TextureAspect::Plane2:
                return Aspect::Plane2;
            default:
                break;
            }
        }

        return AspectConvert(format, aspect);
    }
}