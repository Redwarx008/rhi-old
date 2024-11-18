#pragma once
#include <cstdint>
#include "rhi/common/Utils.h"

namespace rhi
{
	class IObject;
	class IResource;
	class IBuffer;
	class ITexture;
	class ITextureView;
	class ISampler;
	class IShader;
	class IResourceSetLayout;
	class IResourceSet;
	class IPipeline;
	class IGraphicsPipeline;
	class IComputePipeline;
	class ICommandList;
	class IRenderDevice;
	class ISwapChain;

	// resource 

	enum class ResourceState : uint32_t
	{
		Undefined = (0 << 0),
		Common = (1 << 0),
		ConstantBuffer = (1 << 1),
		VertexBuffer = (1 << 2),
		IndexBuffer = (1 << 3),
		IndirectBuffer = (1 << 4),
		ShaderResource = (1 << 5),
		UnorderedAccess = (1 << 6),
		RenderTarget = (1 << 7),
		DepthWrite = (1 << 8),
		DepthRead = (1 << 9),
		CopyDest = (1 << 10),
		CopySource = (1 << 11),
		ResolveDest = (1 << 12),
		ResolveSource = (1 << 13),
		Present = (1 << 14),
		//AccelStructRead = (1 << 15),
		//AccelStructWrite = (1 << 16),
		//AccelStructBuildInput = (1 << 17),
		//AccelStructBuildBlas = (1 << 18),
		//ShadingRateSurface = (1 << 19),
	};
	ENUM_CLASS_FLAG_OPERATORS(ResourceState);

	enum class NativeObjectType
	{
		VK_Device,
		VK_PhysicalDevice,
		VK_Instance,
		VK_Queue,
		VK_CommandBuffer,
		VK_DeviceMemory,
		VK_Buffer,
		VK_Image,
		VK_ImageView,
		//VK_AccelerationStructureKHR,
		VK_Sampler,
		VK_ShaderModule,
		VK_DescriptorPool,
		VK_DescriptorSetLayout,
		VK_DescriptorSet,
		VK_PipelineLayout,
		VK_Pipeline,
		VK_PipelineCache,
		VK_Micromap
	};



	// buffer
	enum class MapBufferUsage : uint8_t
	{
		Read,
		Write
	};

	enum class BufferAccess : uint8_t
	{
		// means VK_MEMORY_HEAP_DEVICE_LOCAL_BIT
		GpuOnly,
		// means VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		CpuWrite,
		// Buffers for data written by or transferred from the GPU that you want to read back on the CPU, e.g. results of some computations.
		// means vVK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT 
		CpuRead,
	};

	// defualt VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
	enum class BufferUsage : uint32_t
	{
		None = 0 << 0,
		VertexBuffer = 1 << 0,
		IndexBuffer = 1 << 1,
		IndirectBuffer = 1 << 2,
		UniformBuffer = 1 << 3,
		StorageBuffer = 1 << 4,
		UniformTexelBuffer = 1 << 5,
		StorageTexelBuffer = 1 << 6
	};
	ENUM_CLASS_FLAG_OPERATORS(BufferUsage);

	struct BufferDesc
	{
		size_t size = 0;

		BufferAccess access = BufferAccess::GpuOnly;

		BufferUsage usage = BufferUsage::None;

		BufferDesc& setSize(size_t size) { this->size = size; return *this; }
		BufferDesc& setAccess(BufferAccess accessFlag) { this->access = accessFlag; return *this; }
		BufferDesc& setUsage(BufferUsage usageFlag) { this->usage = usage; return *this; }
	};

	// texture

	enum class TextureDimension : uint8_t
	{
		Undefined,
		Texture1D,
		Texture1DArray,
		Texture2D,
		Texture2DArray,
		TextureCube,
		TextureCubeArray,
		Texture3D
	};

	enum class Format : uint8_t
	{
		UNKNOWN,

		R8_UINT,
		R8_SINT,
		R8_UNORM,
		R8_SNORM,
		RG8_UINT,
		RG8_SINT,
		RG8_UNORM,
		RG8_SNORM,
		R16_UINT,
		R16_SINT,
		R16_UNORM,
		R16_SNORM,
		R16_FLOAT,
		BGRA4_UNORM,
		B5G6R5_UNORM,
		B5G5R5A1_UNORM,
		RGBA8_UINT,
		RGBA8_SINT,
		RGBA8_UNORM,
		RGBA8_SNORM,
		BGRA8_UNORM,
		RGBA8_SRGB,
		BGRA8_SRGB,
		R10G10B10A2_UNORM,
		R11G11B10_FLOAT,
		RG16_UINT,
		RG16_SINT,
		RG16_UNORM,
		RG16_SNORM,
		RG16_FLOAT,
		R32_UINT,
		R32_SINT,
		R32_FLOAT,
		RGBA16_UINT,
		RGBA16_SINT,
		RGBA16_FLOAT,
		RGBA16_UNORM,
		RGBA16_SNORM,
		RG32_UINT,
		RG32_SINT,
		RG32_FLOAT,
		RGB32_UINT,
		RGB32_SINT,
		RGB32_FLOAT,
		RGBA32_UINT,
		RGBA32_SINT,
		RGBA32_FLOAT,

		D16_UNORM,
		D24_UNORM_S8_UINT,
		D32_UNORM,
		D32_UNORM_S8_UINT,

		BC1_UNORM,
		BC1_UNORM_SRGB,
		BC2_UNORM,
		BC2_UNORM_SRGB,
		BC3_UNORM,
		BC3_UNORM_SRGB,
		BC4_UNORM,
		BC4_SNORM,
		BC5_UNORM,
		BC5_SNORM,
		BC6H_UFLOAT,
		BC6H_SFLOAT,
		BC7_UNORM,
		BC7_UNORM_SRGB,

		COUNT,
	};

	enum class TextureUsage : uint8_t
	{
		Unknown = 0 << 0,
		ShaderResource = 1 << 0,
		UnorderedAccess = 1 << 1,
		RenderTarget = 1 << 2,
		DepthStencil = 1 << 3
	};
	ENUM_CLASS_FLAG_OPERATORS(TextureUsage);

	struct Region3D
	{
		uint32_t minX = 0;
		uint32_t maxX = 0;
		uint32_t minY = 0;
		uint32_t maxY = 0;
		uint32_t minZ = 0;
		uint32_t maxZ = 1;
		uint32_t getWidth() const { return maxX - minX; }
		uint32_t getHeight() const { return maxY - minY; }
		uint32_t getDepth() const { return maxZ - minZ; }
		bool isValid() const { return maxX > minX && maxY > minY && maxZ > minZ; }
	};

	struct TextureUpdateInfo
	{
		uint32_t srcRowPitch = 0;
		uint32_t srcDepthPitch = 0;
		uint32_t arrayLayer = 0;
		uint32_t mipLevel = 0;
		Region3D dstRegion;
	};

	struct TextureViewDesc
	{
		TextureDimension dimension = TextureDimension::Undefined;
		union
		{
			uint32_t baseArrayLayer = 0;
			uint32_t baseDepthLayer;
		};

		union
		{
			uint32_t arrayLayerCount = 1;
			uint32_t depthLayerCount;
		};

		uint32_t baseMipLevel = 0;
		uint32_t mipLevelCount = 1;
	};

	struct TextureDesc
	{
		TextureDimension dimension = TextureDimension::Undefined;
		uint32_t width = 1;
		uint32_t height = 1;
		/// For a 1D array or 2D array, number of array slices
		uint32_t arraySize = 1;
		/// For a 3D texture, number of depth slices
		uint32_t depth = 1;
		uint32_t sampleCount = 1;
		uint32_t mipLevels = 1;
		Format format = Format::UNKNOWN;

		TextureUsage usage = TextureUsage::Unknown;

		constexpr TextureDesc& setWidth(uint32_t value) { width = value; return *this; }
		constexpr TextureDesc& setHeight(uint32_t value) { height = value; return *this; }
		constexpr TextureDesc& setDepth(uint32_t value) { depth = value; return *this; }
		constexpr TextureDesc& setArraySize(uint32_t value) { arraySize = value; return *this; }
		constexpr TextureDesc& setMipLevels(uint32_t value) { mipLevels = value; return *this; }
		constexpr TextureDesc& setFormat(Format value) { format = value; return *this; }
		constexpr TextureDesc& setDimension(TextureDimension value) { dimension = value; return *this; }
	};

	enum class SamplerAddressMode : uint8_t
	{
		// D3D names
		Clamp,
		Wrap,
		Border,
		Mirror,
		MirrorOnce,

		// Vulkan names
		ClampToEdge = Clamp,
		Repeat = Wrap,
		ClampToBorder = Border,
		MirroredRepeat = Mirror,
		MirrorClampToEdge = MirrorOnce
	};

	enum class FilterMode : uint8_t
	{
		Linear,
		nearest
	};

	enum class BorderColor : uint8_t
	{
		FloatOpaqueBlack,
		FloatOpaqueWhite,
		FloatTransparentBlack
	};

	struct SamplerDesc
	{
		float maxAnisotropy = 1.f;
		float mipLodBias = 0.f;
		BorderColor borderColor = BorderColor::FloatOpaqueBlack;

		FilterMode magFilter = FilterMode::Linear;
		FilterMode minFilter = FilterMode::Linear;
		FilterMode mipmapMode = FilterMode::Linear;

		SamplerAddressMode addressModeU = SamplerAddressMode::Clamp;
		SamplerAddressMode addressModeV = SamplerAddressMode::Clamp;
		SamplerAddressMode addressModeW = SamplerAddressMode::Clamp;
	};

	// shader

	enum class ShaderResourceType : uint8_t
	{
		Unknown,
		SampledTexture, //SRV
		StorageTexture, //URV
		UniformBuffer, // CBV
		StorageBuffer, // UAV
		UniformTexelBuffer,
		StorageTexelBuffer,
		Sampler,
		TextureWithSampler // Combined texture and sampler
	};

	enum class ShaderType : uint32_t
	{
		Unknown = (0 << 0),
		Vertex = (1 << 0),
		TessellationControl = (1 << 1),
		TessellationEvaluation = (1 << 2),
		Geometry = (1 << 3),
		Fragment = (1 << 4),
		Task = (1 << 5),
		Mesh = (1 << 6),
		Compute = (1 << 7),
		AllGraphics = Vertex | TessellationControl | TessellationEvaluation | Geometry | Fragment,
		All = Vertex | TessellationControl | TessellationEvaluation | Geometry | Fragment | Task | Mesh | Compute
	};
	inline constexpr ShaderType operator | (ShaderType a, ShaderType b) {
		return ShaderType(uint32_t(a) | uint32_t(b));
	} inline constexpr ShaderType operator |= (ShaderType a, ShaderType b) {
		return ShaderType(a = a | b);
	} inline constexpr ShaderType operator & (ShaderType a, ShaderType b) {
		return ShaderType(uint32_t(a) & uint32_t(b));
	} inline constexpr ShaderType operator ~ (ShaderType a) {
		return ShaderType(~uint32_t(a));
	} inline constexpr bool operator !(ShaderType a) {
		return uint32_t(a) == 0;
	} inline constexpr bool operator ==(ShaderType a, uint32_t b) {
		return uint32_t(a) == b;
	} inline constexpr bool operator !=(ShaderType a, uint32_t b) {
		return uint32_t(a) != b;
	};

	struct SpecializationConstant
	{
		uint32_t constantID = 0;
		union
		{
			uint32_t u = 0;
			int32_t i;
			float f;
		} value;

		static SpecializationConstant UInt32(uint32_t constantID, uint32_t value) {
			SpecializationConstant s;
			s.constantID = constantID;
			s.value.u = value;
			return s;
		}

		static SpecializationConstant Int32(uint32_t constantID, int32_t value) {
			SpecializationConstant s;
			s.constantID = constantID;
			s.value.i = value;
			return s;
		}

		static SpecializationConstant Float(uint32_t constantID, float value) {
			SpecializationConstant s;
			s.constantID = constantID;
			s.value.f = value;
			return s;
		}
	};

	struct ShaderCreateInfo
	{
		ShaderType type = ShaderType::Unknown;
		const char* entry = nullptr;
		const SpecializationConstant* specializationConstants;
		uint32_t specializationConstantCount = 0;
	};

	struct ShaderDesc
	{
		ShaderType type = ShaderType::Unknown;
		const char* entry = nullptr;
	};

	// resource set 

	struct ResourceSetLayoutBinding
	{
		ShaderType visibleStages = ShaderType::Unknown;
		ShaderResourceType type = ShaderResourceType::Unknown;
		uint32_t bindingSlot = 0;
		uint32_t arrayElementCount = 1;

		static ResourceSetLayoutBinding SampledTexture(ShaderType stage, uint32_t bindingSlot, uint32_t arrayElementCount = 1)
		{
			ResourceSetLayoutBinding binding{};
			binding.visibleStages = stage;
			binding.type = ShaderResourceType::SampledTexture;
			binding.bindingSlot = bindingSlot;
			binding.arrayElementCount = arrayElementCount;
			return binding;
		}
		static ResourceSetLayoutBinding StorageTexture(ShaderType stage, uint32_t bindingSlot, uint32_t arrayElementCount = 1)
		{
			ResourceSetLayoutBinding binding{};
			binding.visibleStages = stage;
			binding.type = ShaderResourceType::StorageTexture;
			binding.bindingSlot = bindingSlot;
			binding.arrayElementCount = arrayElementCount;
			return binding;
		}
		static ResourceSetLayoutBinding UniformBuffer(ShaderType stage, uint32_t bindingSlot, uint32_t arrayElementCount = 1)
		{
			ResourceSetLayoutBinding binding{};
			binding.visibleStages = stage;
			binding.type = ShaderResourceType::UniformBuffer;
			binding.bindingSlot = bindingSlot;
			binding.arrayElementCount = arrayElementCount;
			return binding;
		}
		static ResourceSetLayoutBinding StorageBuffer(ShaderType stage, uint32_t bindingSlot, uint32_t arrayElementCount = 1)
		{
			ResourceSetLayoutBinding binding{};
			binding.visibleStages = stage;
			binding.type = ShaderResourceType::StorageBuffer;
			binding.bindingSlot = bindingSlot;
			binding.arrayElementCount = arrayElementCount;
			return binding;
		}
		static ResourceSetLayoutBinding Sampler(ShaderType stage, uint32_t bindingSlot, uint32_t arrayElementCount = 1)
		{
			ResourceSetLayoutBinding binding{};
			binding.visibleStages = stage;
			binding.type = ShaderResourceType::Sampler;
			binding.bindingSlot = bindingSlot;
			binding.arrayElementCount = arrayElementCount;
			return binding;
		}
		static ResourceSetLayoutBinding TextureWithSampler(ShaderType stage, uint32_t bindingSlot, uint32_t arrayElementCount = 1)
		{
			ResourceSetLayoutBinding binding{};
			binding.visibleStages = stage;
			binding.type = ShaderResourceType::TextureWithSampler;
			binding.bindingSlot = bindingSlot;
			binding.arrayElementCount = arrayElementCount;
			return binding;
		}
	};

	struct ResourceSetBinding
	{
		ShaderResourceType type = ShaderResourceType::Unknown;

		uint32_t bindingSlot = 0;
		uint32_t arrayElementIndex = 0;

		ITextureView* textureView = nullptr;
		ISampler* sampler = nullptr;
		IBuffer* buffer = nullptr;

		//for buffer
		uint32_t bufferOffset = 0;
		uint32_t bufferRange = 0;  // if range is 0, use the range from offset to the end of the buffer.

		static ResourceSetBinding SampledTexture(ITextureView* textureView, uint32_t bindingSlot, uint32_t arrayElementIndex = 0)
		{
			ResourceSetBinding binding{};
			binding.type = ShaderResourceType::SampledTexture;
			binding.bindingSlot = bindingSlot;
			binding.textureView = textureView;
			binding.arrayElementIndex = arrayElementIndex;
			return binding;
		}

		static ResourceSetBinding StorageTexture(ITextureView* textureView, uint32_t bindingSlot, uint32_t arrayElementIndex = 0)
		{
			ResourceSetBinding binding{};
			binding.type = ShaderResourceType::StorageTexture;
			binding.bindingSlot = bindingSlot;
			binding.textureView = textureView;
			binding.arrayElementIndex = arrayElementIndex;
			return binding;
		}

		static ResourceSetBinding UniformBuffer(IBuffer* buffer, uint32_t bindingSlot, uint32_t arrayElementIndex = 0, 
			uint32_t offset = 0, uint32_t range = 0)
		{
			ResourceSetBinding binding{};
			binding.type = ShaderResourceType::UniformBuffer;
			binding.bindingSlot = bindingSlot;
			binding.buffer = buffer;
			binding.bufferOffset = offset;
			binding.bufferRange = range;
			binding.arrayElementIndex = arrayElementIndex;
			return binding;
		}

		static ResourceSetBinding StorageBuffer(IBuffer* buffer, uint32_t bindingSlot, uint32_t arrayElementIndex = 0, 
			uint32_t offset = 0, uint32_t range = 0)
		{
			ResourceSetBinding binding{};
			binding.type = ShaderResourceType::StorageBuffer;
			binding.bindingSlot = bindingSlot;
			binding.buffer = buffer;
			binding.bufferOffset = offset;
			binding.bufferRange = range;
			binding.arrayElementIndex = arrayElementIndex;
			return binding;
		}

		static ResourceSetBinding Sampler(ISampler* sampler, uint32_t bindingSlot, uint32_t arrayElementIndex = 0)
		{
			ResourceSetBinding binding{};
			binding.type = ShaderResourceType::StorageBuffer;
			binding.bindingSlot = bindingSlot;
			binding.sampler = sampler;
			binding.arrayElementIndex = arrayElementIndex;
			return binding;
		}

		static ResourceSetBinding TextureWithSampler(ITextureView* textureView, ISampler* sampler, uint32_t bindingSlot, 
			uint32_t arrayElementIndex = 0)
		{
			ResourceSetBinding binding{};
			binding.type = ShaderResourceType::TextureWithSampler;
			binding.bindingSlot = bindingSlot;
			binding.textureView = textureView;
			binding.sampler = sampler;
			binding.arrayElementIndex = arrayElementIndex;
			return binding;
		}
	};

	// pipeline

	static constexpr uint32_t g_MaxColorAttachments = 8;
	static constexpr uint32_t g_MaxViewPorts = 16;
	static constexpr uint32_t g_MaxBoundDescriptorSets = 4;
	static constexpr uint32_t g_MaxVertexInputBindings = 16;

	enum class PrimitiveType : uint8_t
	{
		PointList,
		LineList,
		LineStrip,
		TriangleList,
		TriangleStrip,
		TriangleFan,
		PatchList
	};

	struct VertexInputAttribute
	{
		uint32_t bindingBufferSlot = 0;
		uint32_t location = 0;
		Format format = Format::R16_UINT;
		bool isInstanced = false;
		// If this value is set to UINT32_MAX (default value), the offset will
		// be computed automatically by placing the element right after the previous one.
		uint32_t offsetInElement = UINT32_MAX;
		// Stride in bytes between two elements, for one buffer slot.
		// If this value is set to LAYOUT_ELEMENT_AUTO_STRIDE, the stride will be
		// computed automatically assuming that all elements in the same buffer slot that are
		// packed one by one. or must specify the same stride in the same buffer.
		uint32_t elementStride = UINT32_MAX;
	};

	enum class BlendFactor : uint32_t
	{
		Zero,
		One,
		SrcColor,
		OneMinusSrcColor,
		DstColor,
		OneMinusDstColor,
		SrcAlpha,
		OneMinusSrcAlpha,
		DstAlpha,
		OneMinusDstAlpha,
		ConstantColor,
		OneMinusConstantColor,
		ConstantAlpha,
		OneMinusConstantAlpha,
		SrcAlphaSaturate,
		Src1Color,
		OneMinusSrc1Color,
		Src1Alpha,
		OneMinusSrc1Alpha
	};

	enum class BlendOp : uint8_t
	{
		Add = 1,
		Subrtact = 2,
		ReverseSubtract = 3,
		Min = 4,
		Max = 5
	};

	enum class ColorMask : uint32_t
	{
		// These values are equal to their counterparts in DX11, DX12, and Vulkan.
		Red = 1,
		Green = 2,
		Blue = 4,
		Alpha = 8,
		All = 0xF
	};
	ENUM_CLASS_FLAG_OPERATORS(ColorMask);

	struct BlendState
	{
		bool alphaToCoverageEnable = false;

		struct RenderTargetBlendState
		{
			bool        blendEnable = false;
			BlendFactor srcColorBlend = BlendFactor::One;
			BlendFactor destColorBlend = BlendFactor::Zero;
			BlendOp     colorBlendOp = BlendOp::Add;
			BlendFactor srcAlphaBlend = BlendFactor::One;
			BlendFactor destAlphaBlend = BlendFactor::Zero;
			BlendOp     alphaBlendOp = BlendOp::Add;
			ColorMask   colorWriteMask = ColorMask::All;
		};

		static constexpr uint32_t MaxRenderTargets = 8;
		RenderTargetBlendState renderTargetBlendStates[MaxRenderTargets];
	};

	enum class CompareOp : uint32_t
	{
		Never = 0,
		Less = 1,
		Equal = 2,
		LessOrEqual = 3,
		Greater = 4,
		NotEqual = 5,
		GreaterOrEqual = 6,
		Always = 7
	};

	enum class StencilOp : uint32_t
	{
		Keep = 0,
		Zero = 1,
		Replace = 2,
		IncrementAndClamp = 3,
		DecrementAndClamp = 4,
		Invert = 5,
		IncrementAndWrap = 6,
		DecrementAndWrap = 7
	};

	struct StencilOpState
	{
		StencilOp failOp = StencilOp::Keep;
		StencilOp passOp = StencilOp::Keep;
		StencilOp depthFailOp = StencilOp::Keep;
		CompareOp compareOp = CompareOp::Always;
		uint8_t writeMask = 0xff;
		uint8_t compareMak = 0xff;
		uint32_t referenceValue = 0;
	};

	struct DepthStencilState
	{
		bool            depthTestEnable = true;
		bool            depthWriteEnable = true;
		CompareOp		depthCompareOp = CompareOp::LessOrEqual;
		bool            stencilTestEnable = false;
		uint8_t         stencilReadMask = 0xff;
		uint8_t         stencilWriteMask = 0xff;
		StencilOpState	frontFaceStencil;
		StencilOpState	backFaceStencil;
	};

	enum class PolygonMode : uint8_t
	{
		Fill = 0,
		Line = 1,
		Point = 2
	};

	enum class CullMode : uint8_t
	{
		None,
		Front,
		back
	};

	struct RasterState
	{
		PolygonMode fillMode = PolygonMode::Fill;
		CullMode cullMode = CullMode::back;
		bool depthClampEnable = false; //must be false if this feature is not enabled
		bool depthBiasEnable = false; //must be false if this feature is not enabled
		bool frontCounterClockwise = true;
		float lineWidth = 1.0f;
	};

	struct Viewport
	{
		float minX, maxX;
		float minY, maxY;
		float minZ, maxZ;
		Viewport() : minX(0.f), maxX(0.f), minY(0.f), maxY(0.f), minZ(0.f), maxZ(1.f) {}

		Viewport(float width, float height) : minX(0.f), maxX(width), minY(0.f), maxY(height), minZ(0.f), maxZ(1.f) {}

		bool operator ==(const Viewport& b) const
		{
			return minX == b.minX
				&& minY == b.minY
				&& minZ == b.minZ
				&& maxX == b.maxX
				&& maxY == b.maxY
				&& maxZ == b.maxZ;
		}
		bool operator !=(const Viewport& b) const { return !(*this == b); }

		float getWidth() const { return maxX - minX; }
		float getHeight() const { return maxY - minY; }
	};

	struct Rect
	{
		int minX, maxX;
		int minY, maxY;
		Rect() : minX(0), maxX(0), minY(0), maxY(0) {}
		Rect(int width, int height) : minX(0), maxX(width), minY(0), maxY(height) {}
		Rect(int _minX, int _maxX, int _minY, int _maxY) : minX(_minX), maxX(_maxX), minY(_minY), maxY(_maxY) {}
		explicit Rect(const Viewport& viewport)
			: minX(int(floorf(viewport.minX)))
			, maxX(int(ceilf(viewport.maxX)))
			, minY(int(floorf(viewport.minY)))
			, maxY(int(ceilf(viewport.maxY)))
		{
		}
		int getWidth() const { return maxX - minX; }
		int getHeight() const { return maxY - minY; }
		bool operator ==(const Rect& b) const
		{
			return minX == b.minX
				&& maxX == b.maxX
				&& minY == b.minY
				&& maxY == b.maxY;
		}
		bool operator !=(const Rect& b) const { return !(*this == b); }
	};

	inline bool operator == (const Viewport& viewport, const Rect& rect)
	{
		return (int)viewport.minX == rect.minX &&
			(int)viewport.minY == rect.maxY &&
			(int)viewport.getWidth() == rect.getWidth() &&
			(int)viewport.getHeight() == rect.getHeight();
	}

	inline bool operator != (const Viewport& viewport, const Rect& rect)
	{
		return !(viewport == rect);
	}

	struct VertexBufferBinding
	{
		IBuffer* buffer = nullptr;
		uint32_t bindingSlot = 0;
		uint64_t offset = 0;

		bool operator ==(const VertexBufferBinding& b) const
		{
			return buffer == b.buffer
				&& bindingSlot == b.bindingSlot
				&& offset == b.offset;
		}

		bool operator !=(const VertexBufferBinding& b) const { return !(*this == b); }
		VertexBufferBinding& setBuffer(IBuffer* value) { buffer = value; return *this; }
		VertexBufferBinding& setSlot(uint32_t value) { bindingSlot = value; return *this; }
		VertexBufferBinding& setOffset(uint64_t value) { offset = value; return *this; }
	};

	struct IndexBufferBinding
	{
		IBuffer* buffer = nullptr;
		Format format = Format::UNKNOWN;
		uint32_t offset = 0;

		bool operator ==(const IndexBufferBinding& b) const
		{
			return buffer == b.buffer
				&& format == b.format
				&& offset == b.offset;
		}
		bool operator !=(const IndexBufferBinding& b) const { return !(*this == b); }

		IndexBufferBinding& setBuffer(IBuffer* value) { buffer = value; return *this; }
		IndexBufferBinding& setFormat(Format value) { format = value; return *this; }
		IndexBufferBinding& setOffset(uint32_t value) { offset = value; return *this; }
	};

	struct PushConstantDesc
	{
		ShaderType stage = ShaderType::Unknown;
		uint32_t size = 0;
	};

	struct GraphicsPipelineCreateInfo
	{
		PrimitiveType primType = PrimitiveType::TriangleList;

		VertexInputAttribute* vertexInputAttributes;
		uint32_t vertexInputAttributeCount = 0;

		IResourceSetLayout* const* resourceSetLayouts;
		uint32_t resourceSetLayoutCount = 0;

		const PushConstantDesc* pushConstantDescs;
		uint32_t pushConstantCount = 0;

		IShader* vertexShader = nullptr;
		IShader* fragmentShader = nullptr;
		IShader* tessControlShader = nullptr;
		IShader* tessEvaluationShader = nullptr;
		IShader* geometryShader = nullptr;

		BlendState blendState;
		RasterState rasterState;
		DepthStencilState depthStencilState;

		Format renderTargetFormats[g_MaxColorAttachments];
		uint32_t renderTargetFormatCount = 0;
		Format depthStencilFormat = Format::UNKNOWN;

		uint8_t sampleCount = 1;
		uint32_t patchControlPoints = 0;
		uint32_t viewportCount = 1;

		const void* cacheData = nullptr;
		uint64_t cacheSize = 0;
	};

	struct ComputePipelineCreateInfo
	{
		IShader* computeShader = nullptr;

		IResourceSetLayout** resourceSetLayouts;
		uint32_t resourceSetLayoutCount = 0;

		const PushConstantDesc* pushConstantDescs;
		uint32_t pushConstantCount = 0;

		const void* cacheData = nullptr;
		uint64_t cacheSize = 0;
	};

	struct GraphicsPipelineDesc
	{
		PrimitiveType primType = PrimitiveType::TriangleList;

		BlendState blendState;
		RasterState rasterState;
		DepthStencilState depthStencilState;

		Format renderTargetFormats[g_MaxColorAttachments]{};
		uint32_t renderTargetFormatCount = 0;
		Format depthStencilFormat = Format::UNKNOWN;

		uint8_t sampleCount = 1;
		uint32_t patchControlPoints = 0;
		uint32_t viewportCount = 1;
	};

	struct GraphicsState
	{
		IGraphicsPipeline* pipeline = nullptr;

		uint32_t renderTargetCount = 0;
		ITextureView* renderTargetViews[g_MaxColorAttachments]{};
		ITextureView* depthStencilView = nullptr;

		uint32_t viewportCount = 0;
		Viewport viewports[g_MaxViewPorts];

		IResourceSet* resourceSets[g_MaxBoundDescriptorSets]{};
		uint32_t resourceSetCount = 0;

		VertexBufferBinding vertexBuffers[g_MaxVertexInputBindings]{};
		uint32_t vertexBufferCount = 0;

		IndexBufferBinding indexBuffer;

		IBuffer* indirectBuffer = nullptr;
		// clear all rendertaget
		bool clearRenderTarget = false;
		bool clearDepthStencil = false;
	};

	struct ComputeState
	{
		IComputePipeline* pipeline = nullptr;

		IBuffer* indirectBuffer = nullptr;

		IResourceSet* resourceSets[g_MaxBoundDescriptorSets]{};
		uint32_t resourceSetCount = 0;
	};

	// command list

	struct DrawIndirectCommand
	{
		uint32_t    vertexCount;
		uint32_t    instanceCount;
		uint32_t    firstVertex;
		uint32_t    firstInstance;
	};

	struct DrawIndexedIndirectCommand
	{
		uint32_t    indexCount;
		uint32_t    instanceCount;
		uint32_t    firstIndex;
		int32_t     vertexOffset;
		uint32_t    firstInstance;
	};

	struct DispatchIndirectCommand
	{
		uint32_t    x;
		uint32_t    y;
		uint32_t    z;
	};

	struct ClearColor
	{
		union
		{
			float float32[4];
			int32_t int32[4];
			uint32_t uint32[4]{};
		};

		explicit ClearColor(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
		{
			uint32[0] = r;
			uint32[1] = g;
			uint32[2] = b;
			uint32[3] = a;
		}
		explicit ClearColor(int32_t r, int32_t g, int32_t b, int32_t a)
		{
			int32[0] = r;
			int32[1] = g;
			int32[2] = b;
			int32[3] = a;
		}
		explicit ClearColor(float r, float g, float b, float a)
		{
			float32[0] = r;
			float32[1] = g;
			float32[2] = b;
			float32[3] = a;
		}
	};

	enum class ClearDepthStencilFlag : uint8_t
	{
		Depth = 0 << 0,
		Stencil = 1 << 0
	};
	ENUM_CLASS_FLAG_OPERATORS(ClearDepthStencilFlag);

	// render device

	enum class MessageSeverity : uint8_t
	{
		Verbose,
		Info,
		Warning,
		Error,
		Fatal
	};

	typedef void(_stdcall* DebugMessageCallbackFunc) (MessageSeverity severity, const char* msg);
	//using DebugMessageCallbackFunc = std::function<void(MessageSeverity severity, const char* msg)>;

	struct RenderDeviceCreateInfo
	{
		DebugMessageCallbackFunc messageCallback;
		bool enableValidationLayer;
		// enable instance features
		bool enableMultiViewport;
		bool enableSampleRateShading;
		bool enableTextureCompressionETC2;
		bool enableSamplerAnisotropy;
		bool enableDepthClamp;
		bool enableDepthBiasClamp;
	};

	// swap chain

	struct SwapChainCreateInfo
	{
		IRenderDevice* renderDevice;
		void* windowHandle;
		Format preferredColorFormat = Format::BGRA8_SRGB;
		// use Format::UNKOWN to create swapChain without depth buffer.
		Format preferredDepthStencilFormat = Format::D32_UNORM_S8_UINT;
		uint32_t initialWidth = 0;
		uint32_t initialHeight = 0;
		bool enableVSync = true;
	};
}