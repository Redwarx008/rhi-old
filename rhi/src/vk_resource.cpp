#include "rhi/common/Error.h"
#include "vk_resource.h"

#include <array>
#include <unordered_map>
#include "vk_errors.h"

namespace rhi
{
	// texture 
	struct TexFmtToVkFmtMapping
	{
		Format rhiFormat;
		VkFormat vkFormat;
	};

	static const std::array<TexFmtToVkFmtMapping, size_t(Format::COUNT)> _texFmtToVkFmtMap = { {
		{ Format::UNKNOWN,           VK_FORMAT_UNDEFINED                },
		{ Format::R8_UINT,           VK_FORMAT_R8_UINT                  },
		{ Format::R8_SINT,           VK_FORMAT_R8_SINT                  },
		{ Format::R8_UNORM,          VK_FORMAT_R8_UNORM                 },
		{ Format::R8_SNORM,          VK_FORMAT_R8_SNORM                 },
		{ Format::RG8_UINT,          VK_FORMAT_R8G8_UINT                },
		{ Format::RG8_SINT,          VK_FORMAT_R8G8_SINT                },
		{ Format::RG8_UNORM,         VK_FORMAT_R8G8_UNORM               },
		{ Format::RG8_SNORM,         VK_FORMAT_R8G8_SNORM               },
		{ Format::R16_UINT,          VK_FORMAT_R16_UINT                 },
		{ Format::R16_SINT,          VK_FORMAT_R16_SINT                 },
		{ Format::R16_UNORM,         VK_FORMAT_R16_UNORM                },
		{ Format::R16_SNORM,         VK_FORMAT_R16_SNORM                },
		{ Format::R16_FLOAT,         VK_FORMAT_R16_SFLOAT               },
		{ Format::BGRA4_UNORM,       VK_FORMAT_B4G4R4A4_UNORM_PACK16    },
		{ Format::B5G6R5_UNORM,      VK_FORMAT_B5G6R5_UNORM_PACK16      },
		{ Format::B5G5R5A1_UNORM,    VK_FORMAT_B5G5R5A1_UNORM_PACK16    },
		{ Format::RGBA8_UINT,        VK_FORMAT_R8G8B8A8_UINT            },
		{ Format::RGBA8_SINT,        VK_FORMAT_R8G8B8A8_SINT            },
		{ Format::RGBA8_UNORM,       VK_FORMAT_R8G8B8A8_UNORM           },
		{ Format::RGBA8_SNORM,       VK_FORMAT_R8G8B8A8_SNORM           },
		{ Format::BGRA8_UNORM,       VK_FORMAT_B8G8R8A8_UNORM           },
		{ Format::RGBA8_SRGB,        VK_FORMAT_R8G8B8A8_SRGB            },
		{ Format::BGRA8_SRGB,		VK_FORMAT_B8G8R8A8_SRGB            },
		{ Format::R10G10B10A2_UNORM, VK_FORMAT_A2B10G10R10_UNORM_PACK32 },
		{ Format::R11G11B10_FLOAT,   VK_FORMAT_B10G11R11_UFLOAT_PACK32  },
		{ Format::RG16_UINT,         VK_FORMAT_R16G16_UINT              },
		{ Format::RG16_SINT,         VK_FORMAT_R16G16_SINT              },
		{ Format::RG16_UNORM,        VK_FORMAT_R16G16_UNORM             },
		{ Format::RG16_SNORM,        VK_FORMAT_R16G16_SNORM             },
		{ Format::RG16_FLOAT,        VK_FORMAT_R16G16_SFLOAT            },
		{ Format::R32_UINT,          VK_FORMAT_R32_UINT                 },
		{ Format::R32_SINT,          VK_FORMAT_R32_SINT                 },
		{ Format::R32_FLOAT,         VK_FORMAT_R32_SFLOAT               },
		{ Format::RGBA16_UINT,       VK_FORMAT_R16G16B16A16_UINT        },
		{ Format::RGBA16_SINT,       VK_FORMAT_R16G16B16A16_SINT        },
		{ Format::RGBA16_FLOAT,      VK_FORMAT_R16G16B16A16_SFLOAT      },
		{ Format::RGBA16_UNORM,      VK_FORMAT_R16G16B16A16_UNORM       },
		{ Format::RGBA16_SNORM,      VK_FORMAT_R16G16B16A16_SNORM       },
		{ Format::RG32_UINT,         VK_FORMAT_R32G32_UINT              },
		{ Format::RG32_SINT,         VK_FORMAT_R32G32_SINT              },
		{ Format::RG32_FLOAT,        VK_FORMAT_R32G32_SFLOAT            },
		{ Format::RGB32_UINT,        VK_FORMAT_R32G32B32_UINT           },
		{ Format::RGB32_SINT,        VK_FORMAT_R32G32B32_SINT           },
		{ Format::RGB32_FLOAT,       VK_FORMAT_R32G32B32_SFLOAT         },
		{ Format::RGBA32_UINT,       VK_FORMAT_R32G32B32A32_UINT        },
		{ Format::RGBA32_SINT,       VK_FORMAT_R32G32B32A32_SINT        },
		{ Format::RGBA32_FLOAT,      VK_FORMAT_R32G32B32A32_SFLOAT      },
		{ Format::D16_UNORM,         VK_FORMAT_D16_UNORM                },
		{ Format::D24_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT        },
		{ Format::D32_UNORM,         VK_FORMAT_D32_SFLOAT               },
		{ Format::D32_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT       },
		{ Format::BC1_UNORM,         VK_FORMAT_BC1_RGBA_UNORM_BLOCK     },
		{ Format::BC1_UNORM_SRGB,    VK_FORMAT_BC1_RGBA_SRGB_BLOCK      },
		{ Format::BC2_UNORM,         VK_FORMAT_BC2_UNORM_BLOCK          },
		{ Format::BC2_UNORM_SRGB,    VK_FORMAT_BC2_SRGB_BLOCK           },
		{ Format::BC3_UNORM,         VK_FORMAT_BC3_UNORM_BLOCK          },
		{ Format::BC3_UNORM_SRGB,    VK_FORMAT_BC3_SRGB_BLOCK           },
		{ Format::BC4_UNORM,         VK_FORMAT_BC4_UNORM_BLOCK          },
		{ Format::BC4_SNORM,         VK_FORMAT_BC4_SNORM_BLOCK          },
		{ Format::BC5_UNORM,         VK_FORMAT_BC5_UNORM_BLOCK          },
		{ Format::BC5_SNORM,         VK_FORMAT_BC5_SNORM_BLOCK          },
		{ Format::BC6H_UFLOAT,       VK_FORMAT_BC6H_UFLOAT_BLOCK        },
		{ Format::BC6H_SFLOAT,       VK_FORMAT_BC6H_SFLOAT_BLOCK        },
		{ Format::BC7_UNORM,         VK_FORMAT_BC7_UNORM_BLOCK          },
		{ Format::BC7_UNORM_SRGB,    VK_FORMAT_BC7_SRGB_BLOCK           },

	} };

	static const std::unordered_map<VkFormat, Format> _VkFmtToTexFmtMap =
	{
		{ VK_FORMAT_UNDEFINED                ,Format::UNKNOWN          },
		{ VK_FORMAT_R8_UINT                  ,Format::R8_UINT          },
		{ VK_FORMAT_R8_SINT                  ,Format::R8_SINT          },
		{ VK_FORMAT_R8_UNORM                 ,Format::R8_UNORM         },
		{ VK_FORMAT_R8_SNORM                 ,Format::R8_SNORM         },
		{ VK_FORMAT_R8G8_UINT                ,Format::RG8_UINT         },
		{ VK_FORMAT_R8G8_SINT                ,Format::RG8_SINT         },
		{ VK_FORMAT_R8G8_UNORM               ,Format::RG8_UNORM        },
		{ VK_FORMAT_R8G8_SNORM               ,Format::RG8_SNORM        },
		{ VK_FORMAT_R16_UINT                 ,Format::R16_UINT         },
		{ VK_FORMAT_R16_SINT                 ,Format::R16_SINT         },
		{ VK_FORMAT_R16_UNORM                ,Format::R16_UNORM        },
		{ VK_FORMAT_R16_SNORM                ,Format::R16_SNORM        },
		{ VK_FORMAT_R16_SFLOAT               ,Format::R16_FLOAT        },
		{ VK_FORMAT_B4G4R4A4_UNORM_PACK16    ,Format::BGRA4_UNORM      },
		{ VK_FORMAT_B5G6R5_UNORM_PACK16      ,Format::B5G6R5_UNORM     },
		{ VK_FORMAT_B5G5R5A1_UNORM_PACK16    ,Format::B5G5R5A1_UNORM   },
		{ VK_FORMAT_R8G8B8A8_UINT            ,Format::RGBA8_UINT       },
		{ VK_FORMAT_R8G8B8A8_SINT            ,Format::RGBA8_SINT       },
		{ VK_FORMAT_R8G8B8A8_UNORM           ,Format::RGBA8_UNORM      },
		{ VK_FORMAT_R8G8B8A8_SNORM           ,Format::RGBA8_SNORM      },
		{ VK_FORMAT_B8G8R8A8_UNORM           ,Format::BGRA8_UNORM      },
		{ VK_FORMAT_R8G8B8A8_SRGB            ,Format::RGBA8_SRGB       },
		{ VK_FORMAT_B8G8R8A8_SRGB            ,Format::BGRA8_SRGB	   },
		{ VK_FORMAT_A2B10G10R10_UNORM_PACK32 ,Format::R10G10B10A2_UNORM},
		{ VK_FORMAT_B10G11R11_UFLOAT_PACK32  ,Format::R11G11B10_FLOAT  },
		{ VK_FORMAT_R16G16_UINT              ,Format::RG16_UINT        },
		{ VK_FORMAT_R16G16_SINT              ,Format::RG16_SINT        },
		{ VK_FORMAT_R16G16_UNORM             ,Format::RG16_UNORM       },
		{ VK_FORMAT_R16G16_SNORM             ,Format::RG16_SNORM       },
		{ VK_FORMAT_R16G16_SFLOAT            ,Format::RG16_FLOAT       },
		{ VK_FORMAT_R32_UINT                 ,Format::R32_UINT         },
		{ VK_FORMAT_R32_SINT                 ,Format::R32_SINT         },
		{ VK_FORMAT_R32_SFLOAT               ,Format::R32_FLOAT        },
		{ VK_FORMAT_R16G16B16A16_UINT        ,Format::RGBA16_UINT      },
		{ VK_FORMAT_R16G16B16A16_SINT        ,Format::RGBA16_SINT      },
		{ VK_FORMAT_R16G16B16A16_SFLOAT      ,Format::RGBA16_FLOAT     },
		{ VK_FORMAT_R16G16B16A16_UNORM       ,Format::RGBA16_UNORM     },
		{ VK_FORMAT_R16G16B16A16_SNORM       ,Format::RGBA16_SNORM     },
		{ VK_FORMAT_R32G32_UINT              ,Format::RG32_UINT        },
		{ VK_FORMAT_R32G32_SINT              ,Format::RG32_SINT        },
		{ VK_FORMAT_R32G32_SFLOAT            ,Format::RG32_FLOAT       },
		{ VK_FORMAT_R32G32B32_UINT           ,Format::RGB32_UINT       },
		{ VK_FORMAT_R32G32B32_SINT           ,Format::RGB32_SINT       },
		{ VK_FORMAT_R32G32B32_SFLOAT         ,Format::RGB32_FLOAT      },
		{ VK_FORMAT_R32G32B32A32_UINT        ,Format::RGBA32_UINT      },
		{ VK_FORMAT_R32G32B32A32_SINT        ,Format::RGBA32_SINT      },
		{ VK_FORMAT_R32G32B32A32_SFLOAT      ,Format::RGBA32_FLOAT     },
		{ VK_FORMAT_D16_UNORM                ,Format::D16_UNORM        },
		{ VK_FORMAT_D24_UNORM_S8_UINT        ,Format::D24_UNORM_S8_UINT},
		{ VK_FORMAT_D32_SFLOAT               ,Format::D32_UNORM        },
		{ VK_FORMAT_D32_SFLOAT_S8_UINT       ,Format::D32_UNORM_S8_UINT},
		{ VK_FORMAT_BC1_RGBA_UNORM_BLOCK     ,Format::BC1_UNORM        },
		{ VK_FORMAT_BC1_RGBA_SRGB_BLOCK      ,Format::BC1_UNORM_SRGB   },
		{ VK_FORMAT_BC2_UNORM_BLOCK          ,Format::BC2_UNORM        },
		{ VK_FORMAT_BC2_SRGB_BLOCK           ,Format::BC2_UNORM_SRGB   },
		{ VK_FORMAT_BC3_UNORM_BLOCK          ,Format::BC3_UNORM        },
		{ VK_FORMAT_BC3_SRGB_BLOCK           ,Format::BC3_UNORM_SRGB   },
		{ VK_FORMAT_BC4_UNORM_BLOCK          ,Format::BC4_UNORM        },
		{ VK_FORMAT_BC4_SNORM_BLOCK          ,Format::BC4_SNORM        },
		{ VK_FORMAT_BC5_UNORM_BLOCK          ,Format::BC5_UNORM        },
		{ VK_FORMAT_BC5_SNORM_BLOCK          ,Format::BC5_SNORM        },
		{ VK_FORMAT_BC6H_UFLOAT_BLOCK        ,Format::BC6H_UFLOAT      },
		{ VK_FORMAT_BC6H_SFLOAT_BLOCK        ,Format::BC6H_SFLOAT      },
		{ VK_FORMAT_BC7_UNORM_BLOCK          ,Format::BC7_UNORM        },
		{ VK_FORMAT_BC7_SRGB_BLOCK           ,Format::BC7_UNORM_SRGB   },

	};

	// Format mapping table. The rows must be in the exactly same order as Format enum members are defined.
	static const FormatInfo c_FormatInfo[] = {
		//  format                   name           bytes blockSize   red   green   blue  alpha  depth  stencl type
	{ Format::UNKNOWN,           "UNKNOWN",           0,   0,  false, false, false, false, false, false,  FormatComponentType::Undefined},
	{ Format::R8_UINT,           "R8_UINT",           1,   1,  true,  false, false, false, false, false,  FormatComponentType::Uint		},
	{ Format::R8_SINT,           "R8_SINT",           1,   1,  true,  false, false, false, false, false,  FormatComponentType::Sint		},
	{ Format::R8_UNORM,          "R8_UNORM",          1,   1,  true,  false, false, false, false, false,  FormatComponentType::Unorm	},
	{ Format::R8_SNORM,          "R8_SNORM",          1,   1,  true,  false, false, false, false, false,  FormatComponentType::Snorm	},
	{ Format::RG8_UINT,          "RG8_UINT",          2,   1,  true,  true,  false, false, false, false,  FormatComponentType::Uint		},
	{ Format::RG8_SINT,          "RG8_SINT",          2,   1,  true,  true,  false, false, false, false,  FormatComponentType::Sint		},
	{ Format::RG8_UNORM,         "RG8_UNORM",         2,   1,  true,  true,  false, false, false, false,  FormatComponentType::Unorm	},
	{ Format::RG8_SNORM,         "RG8_SNORM",         2,   1,  true,  true,  false, false, false, false,  FormatComponentType::Snorm	},
	{ Format::R16_UINT,          "R16_UINT",          2,   1,  true,  false, false, false, false, false,  FormatComponentType::Uint		},
	{ Format::R16_SINT,          "R16_SINT",          2,   1,  true,  false, false, false, false, false,  FormatComponentType::Sint		},
	{ Format::R16_UNORM,         "R16_UNORM",         2,   1,  true,  false, false, false, false, false,  FormatComponentType::Unorm	},
	{ Format::R16_SNORM,         "R16_SNORM",         2,   1,  true,  false, false, false, false, false,  FormatComponentType::Snorm	},
	{ Format::R16_FLOAT,         "R16_FLOAT",         2,   1,  true,  false, false, false, false, false,  FormatComponentType::Float	},
	{ Format::BGRA4_UNORM,       "BGRA4_UNORM",       2,   1,  true,  true,  true,  true,  false, false,  FormatComponentType::Unorm	},
	{ Format::B5G6R5_UNORM,      "B5G6R5_UNORM",      2,   1,  true,  true,  true,  false, false, false,  FormatComponentType::Unorm	},
	{ Format::B5G5R5A1_UNORM,    "B5G5R5A1_UNORM",    2,   1,  true,  true,  true,  true,  false, false,  FormatComponentType::Unorm	},
	{ Format::RGBA8_UINT,        "RGBA8_UINT",        4,   1,  true,  true,  true,  true,  false, false,  FormatComponentType::Uint		},
	{ Format::RGBA8_SINT,        "RGBA8_SINT",        4,   1,  true,  true,  true,  true,  false, false,  FormatComponentType::Sint		},
	{ Format::RGBA8_UNORM,       "RGBA8_UNORM",       4,   1,  true,  true,  true,  true,  false, false,  FormatComponentType::Unorm	},
	{ Format::RGBA8_SNORM,       "RGBA8_SNORM",       4,   1,  true,  true,  true,  true,  false, false,  FormatComponentType::Snorm	},
	{ Format::BGRA8_UNORM,       "BGRA8_UNORM",       4,   1,  true,  true,  true,  true,  false, false,  FormatComponentType::Unorm	},
	{ Format::RGBA8_SRGB,		 "RGBA8_SRGB",		  4,   1,  true,  true,  true,  true,  false, false,  FormatComponentType::UnormSrgb},
	{ Format::BGRA8_SRGB,		 "BGRA8_SRGB",		  4,   1,  true,  true,  true,  true,  false, false,  FormatComponentType::UnormSrgb},
	{ Format::R10G10B10A2_UNORM, "R10G10B10A2_UNORM", 4,   1,  true,  true,  true,  true,  false, false,  FormatComponentType::Unorm	},
	{ Format::R11G11B10_FLOAT,   "R11G11B10_FLOAT",   4,   1,  true,  true,  true,  false, false, false,  FormatComponentType::Float	},
	{ Format::RG16_UINT,         "RG16_UINT",         4,   1,  true,  true,  false, false, false, false,  FormatComponentType::Uint		},
	{ Format::RG16_SINT,         "RG16_SINT",         4,   1,  true,  true,  false, false, false, false,  FormatComponentType::Sint		},
	{ Format::RG16_UNORM,        "RG16_UNORM",        4,   1,  true,  true,  false, false, false, false,  FormatComponentType::Unorm	},
	{ Format::RG16_SNORM,        "RG16_SNORM",        4,   1,  true,  true,  false, false, false, false,  FormatComponentType::Snorm	},
	{ Format::RG16_FLOAT,        "RG16_FLOAT",        4,   1,  true,  true,  false, false, false, false,  FormatComponentType::Float	},
	{ Format::R32_UINT,          "R32_UINT",          4,   1,  true,  false, false, false, false, false,  FormatComponentType::Uint		},
	{ Format::R32_SINT,          "R32_SINT",          4,   1,  true,  false, false, false, false, false,  FormatComponentType::Sint		},
	{ Format::R32_FLOAT,         "R32_FLOAT",         4,   1,  true,  false, false, false, false, false,  FormatComponentType::Float	},
	{ Format::RGBA16_UINT,       "RGBA16_UINT",       8,   1,  true,  true,  true,  true,  false, false,  FormatComponentType::Uint		},
	{ Format::RGBA16_SINT,       "RGBA16_SINT",       8,   1,  true,  true,  true,  true,  false, false,  FormatComponentType::Sint		},
	{ Format::RGBA16_FLOAT,      "RGBA16_FLOAT",      8,   1,  true,  true,  true,  true,  false, false,  FormatComponentType::Float	},
	{ Format::RGBA16_UNORM,      "RGBA16_UNORM",      8,   1,  true,  true,  true,  true,  false, false,  FormatComponentType::Unorm	},
	{ Format::RGBA16_SNORM,      "RGBA16_SNORM",      8,   1,  true,  true,  true,  true,  false, false,  FormatComponentType::Snorm	},
	{ Format::RG32_UINT,         "RG32_UINT",         8,   1,  true,  true,  false, false, false, false,  FormatComponentType::Uint		},
	{ Format::RG32_SINT,         "RG32_SINT",         8,   1,  true,  true,  false, false, false, false,  FormatComponentType::Sint		},
	{ Format::RG32_FLOAT,        "RG32_FLOAT",        8,   1,  true,  true,  false, false, false, false,  FormatComponentType::Float	},
	{ Format::RGB32_UINT,        "RGB32_UINT",        12,  1,  true,  true,  true,  false, false, false,  FormatComponentType::Uint		},
	{ Format::RGB32_SINT,        "RGB32_SINT",        12,  1,  true,  true,  true,  false, false, false,  FormatComponentType::Sint		},
	{ Format::RGB32_FLOAT,       "RGB32_FLOAT",       12,  1,  true,  true,  true,  false, false, false,  FormatComponentType::Float	},
	{ Format::RGBA32_UINT,       "RGBA32_UINT",       16,  1,  true,  true,  true,  true,  false, false,  FormatComponentType::Uint		},
	{ Format::RGBA32_SINT,       "RGBA32_SINT",       16,  1,  true,  true,  true,  true,  false, false,  FormatComponentType::Sint		},
	{ Format::RGBA32_FLOAT,      "RGBA32_FLOAT",      16,  1,  true,  true,  true,  true,  false, false,  FormatComponentType::Float	},
	{ Format::D16_UNORM,         "D16_UNORM",         2,   1,  false, false, false, false, true,  false,  FormatComponentType::Undefined},
	{ Format::D24_UNORM_S8_UINT, "D24_UNORM_S8_UINT", 4,   1,  false, false, false, false, true,  true,   FormatComponentType::Undefined},
	{ Format::D32_UNORM,         "D32_UNORM",         4,   1,  false, false, false, false, true,  false,  FormatComponentType::Undefined},
	{ Format::D32_UNORM_S8_UINT, "D32_UNORM_S8_UINT", 8,   1,  false, false, false, false, true,  true,   FormatComponentType::Undefined},
	{ Format::BC1_UNORM,         "BC1_UNORM",         8,   4,  true,  true,  true,  true,  false, false,  FormatComponentType::Undefined},
	{ Format::BC1_UNORM_SRGB,    "BC1_UNORM_SRGB",    8,   4,  true,  true,  true,  true,  false, false,  FormatComponentType::Undefined},
	{ Format::BC2_UNORM,         "BC2_UNORM",         16,  4,  true,  true,  true,  true,  false, false,  FormatComponentType::Undefined},
	{ Format::BC2_UNORM_SRGB,    "BC2_UNORM_SRGB",    16,  4,  true,  true,  true,  true,  false, false,  FormatComponentType::Undefined},
	{ Format::BC3_UNORM,         "BC3_UNORM",         16,  4,  true,  true,  true,  true,  false, false,  FormatComponentType::Undefined},
	{ Format::BC3_UNORM_SRGB,    "BC3_UNORM_SRGB",    16,  4,  true,  true,  true,  true,  false, false,  FormatComponentType::Undefined},
	{ Format::BC4_UNORM,         "BC4_UNORM",         8,   4,  true,  false, false, false, false, false,  FormatComponentType::Undefined},
	{ Format::BC4_SNORM,         "BC4_SNORM",         8,   4,  true,  false, false, false, false, false,  FormatComponentType::Undefined},
	{ Format::BC5_UNORM,         "BC5_UNORM",         16,  4,  true,  true,  false, false, false, false,  FormatComponentType::Undefined},
	{ Format::BC5_SNORM,         "BC5_SNORM",         16,  4,  true,  true,  false, false, false, false,  FormatComponentType::Undefined},
	{ Format::BC6H_UFLOAT,       "BC6H_UFLOAT",       16,  4,  true,  true,  true,  false, false, false,  FormatComponentType::Undefined},
	{ Format::BC6H_SFLOAT,       "BC6H_SFLOAT",       16,  4,  true,  true,  true,  false, false, false,  FormatComponentType::Undefined},
	{ Format::BC7_UNORM,         "BC7_UNORM",         16,  4,  true,  true,  true,  true,  false, false,  FormatComponentType::Undefined},
	{ Format::BC7_UNORM_SRGB,    "BC7_UNORM_SRGB",    16,  4,  true,  true,  true,  true,  false, false,  FormatComponentType::Undefined},
	};
	const FormatInfo& getFormatInfo(Format format)
	{
		static_assert(sizeof(c_FormatInfo) / sizeof(FormatInfo) == size_t(Format::COUNT),
			"The format info table doesn't have the right number of elements");
		const FormatInfo& info = c_FormatInfo[uint32_t(format)];
		assert(info.format == format);
		return info;
	}

	VkImageUsageFlags getVkImageUsageFlags(const TextureDesc& desc)
	{
		const FormatInfo& formatInfo = getFormatInfo(desc.format);

		VkImageUsageFlags flags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		if ((desc.usage & TextureUsage::ShaderResource) != 0)
		{
			flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
		}

		if ((desc.usage & TextureUsage::UnorderedAccess) != 0)
		{
			flags |= VK_IMAGE_USAGE_STORAGE_BIT;
		}

		if ((desc.usage & TextureUsage::DepthStencil) != 0)
		{
			assert(formatInfo.hasDepth || formatInfo.hasStencil);
			flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}

		if ((desc.usage & TextureUsage::RenderTarget) != 0)
		{
			flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}

		return flags;
	}

	VkImageCreateFlags getVkImageCreateFlags(TextureDimension dimension)
	{
		VkImageCreateFlags flags = 0;
		if (dimension == TextureDimension::TextureCube ||
			dimension == TextureDimension::TextureCubeArray)
		{
			flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		}
		return flags;
	}

	VkSampleCountFlagBits getVkImageSampleCount(const TextureDesc& desc)
	{
		switch (desc.sampleCount)
		{
		case 1:
			return VK_SAMPLE_COUNT_1_BIT;

		case 2:
			return VK_SAMPLE_COUNT_2_BIT;

		case 4:
			return VK_SAMPLE_COUNT_4_BIT;

		case 8:
			return VK_SAMPLE_COUNT_8_BIT;

		case 16:
			return VK_SAMPLE_COUNT_16_BIT;

		case 32:
			return VK_SAMPLE_COUNT_32_BIT;

		case 64:
			return VK_SAMPLE_COUNT_64_BIT;

		default:
			assert(!"Invalid Enumeration Value");
			return VK_SAMPLE_COUNT_1_BIT;
		}
	}

	VkImageType getVkImageType(TextureDimension dimension)
	{
		switch (dimension)
		{
		case TextureDimension::Texture1D:
		case TextureDimension::Texture1DArray:
			return VK_IMAGE_TYPE_1D;

		case TextureDimension::Texture2D:
		case TextureDimension::Texture2DArray:
		case TextureDimension::TextureCube:
		case TextureDimension::TextureCubeArray:
			return VK_IMAGE_TYPE_2D;

		case TextureDimension::Texture3D:
			return VK_IMAGE_TYPE_2D;

		case TextureDimension::Undefined:
		default:
			assert(!"Invalid Enumeration Value");
			return VK_IMAGE_TYPE_2D;
		}
	}

	VkImageViewType getVkImageViewType(TextureDimension dimension)
	{
		switch (dimension)
		{
		case TextureDimension::Texture1D:
			return VK_IMAGE_VIEW_TYPE_1D;

		case TextureDimension::Texture1DArray:
			return VK_IMAGE_VIEW_TYPE_1D_ARRAY;

		case TextureDimension::Texture2D:
			return VK_IMAGE_VIEW_TYPE_2D;

		case TextureDimension::Texture2DArray:
			return VK_IMAGE_VIEW_TYPE_2D_ARRAY;

		case TextureDimension::TextureCube:
			return VK_IMAGE_VIEW_TYPE_CUBE;

		case TextureDimension::TextureCubeArray:
			return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;

		case TextureDimension::Texture3D:
			return VK_IMAGE_VIEW_TYPE_3D;

		case TextureDimension::Undefined:
		default:
			assert(!"Invalid Enumeration Value");
			return VK_IMAGE_VIEW_TYPE_2D;
		}
	}

	VkImageAspectFlags getVkAspectMask(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_S8_UINT:
			return VK_IMAGE_ASPECT_STENCIL_BIT;
			break;
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_D32_SFLOAT:
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		case VK_FORMAT_D16_UNORM_S8_UINT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		default:
			return VK_IMAGE_ASPECT_COLOR_BIT;
		}
	}

	VkFormat formatToVkFormat(Format format)
	{
		assert(format < Format::COUNT);
		assert(_texFmtToVkFmtMap[uint32_t(format)].rhiFormat == format);

		return _texFmtToVkFmtMap[uint32_t(format)].vkFormat;
	}

	Format vkFormatToFormat(VkFormat format)
	{
		auto res = _VkFmtToTexFmtMap.find(format);
		if (res != _VkFmtToTexFmtMap.end())
		{
			return res->second;
		}
		else
		{
			return Format::UNKNOWN;
		}
	}

	TextureCopyInfo getTextureCopyInfo(Format format, const Region3D& region, uint32_t  optimalBufferCopyRowPitchAlignment)
	{
		assert(region.isValid());

		TextureCopyInfo copyInfo;

		FormatInfo formatInfo = getFormatInfo(format);

		uint32_t regionWidth = region.getWidth();
		uint32_t regionHeight = region.getHeight();
		uint32_t regionDepth = region.getDepth();

		if (formatInfo.isCompressed())
		{
			uint32_t blockAlignedRegionWidth = alignUp(regionWidth, formatInfo.blockSize);
			uint32_t blockAlignedRegionHeight = alignUp(regionHeight, formatInfo.blockSize);

			copyInfo.rowBytesCount = blockAlignedRegionWidth / formatInfo.blockSize * formatInfo.bytesPerBlock;
			copyInfo.rowCount = blockAlignedRegionHeight / formatInfo.blockSize;
		}
		else
		{
			copyInfo.rowBytesCount = regionWidth * formatInfo.bytesPerBlock;
			copyInfo.rowCount = regionHeight;
		}

		copyInfo.rowStride = static_cast<uint32_t>(alignUp(copyInfo.rowBytesCount, optimalBufferCopyRowPitchAlignment));
		copyInfo.depthStride = copyInfo.rowCount * copyInfo.rowStride;
		copyInfo.regionBytesCount = regionDepth * copyInfo.depthStride;
		return copyInfo;
	}

	TextureVk::~TextureVk()
	{
		if (managed && image)
		{
			vmaDestroyImage(m_Allocator, image, allocation);
		}

		if (m_DefaultView)
		{
			delete m_DefaultView;
			m_DefaultView = nullptr; 
		}
	}

	ITextureView* TextureVk::createView(TextureViewDesc desc)
	{
		auto textureView = new TextureViewVk(m_Context, *this);

		VkImageViewCreateInfo viewCreateInfo{};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.viewType = getVkImageViewType(desc.dimension);
		viewCreateInfo.format = format;
		viewCreateInfo.image = image;
		viewCreateInfo.subresourceRange.aspectMask = getVkAspectMask(format);
		viewCreateInfo.subresourceRange.baseArrayLayer = desc.baseArrayLayer;
		viewCreateInfo.subresourceRange.layerCount = desc.arrayLayerCount;
		viewCreateInfo.subresourceRange.baseMipLevel = desc.baseMipLevel;
		viewCreateInfo.subresourceRange.levelCount = desc.mipLevelCount;
		VkResult err = vkCreateImageView(m_Context.device, &viewCreateInfo, nullptr, &textureView->imageView);
		CHECK_VK_RESULT(err, "Could not create TextureView");
		if (err != VK_SUCCESS)
		{
			delete textureView;
		}
		return textureView;
	}

	void TextureVk::createDefaultView()
	{
		assert(this->m_DefaultView == nullptr);
		TextureViewDesc desc{};
		desc.dimension = this->m_Desc.dimension;
		desc.baseArrayLayer = 0;
		desc.arrayLayerCount = this->m_Desc.arraySize;
		desc.baseMipLevel = 0;
		desc.mipLevelCount = this->m_Desc.mipLevels;
		this->m_DefaultView = checked_cast<TextureViewVk*>(createView(desc));
	}

	Object TextureVk::getNativeObject(NativeObjectType type) const
	{
		switch (type)
		{
		case NativeObjectType::VK_Image:
			return static_cast<Object>(image);
		case NativeObjectType::VK_ImageView:
			return static_cast<Object>(m_DefaultView);
		default:
			return nullptr;
		}
	}

	TextureViewVk::~TextureViewVk()
	{
		vkDestroyImageView(m_Context.device, imageView, nullptr);
	}

	Object TextureViewVk::getNativeObject(NativeObjectType type) const
	{
		if (type == NativeObjectType::VK_ImageView)
		{
			return static_cast<Object>(imageView);
		}
		return nullptr;
	}

	// buffer

	BufferVk::~BufferVk()
	{
		assert(buffer != VK_NULL_HANDLE);
		vmaDestroyBuffer(m_Allocator, buffer, allocation);
	}

	Object BufferVk::getNativeObject(NativeObjectType type) const
	{
		if (type == NativeObjectType::VK_Buffer)
		{
			return static_cast<Object>(buffer);
		}
		return nullptr;
	}

	// sampler

	SamplerVk::~SamplerVk()
	{
		vkDestroySampler(m_Context.device, sampler, nullptr);
	}

	// resourece state

	static VkAccessFlags2 resourceStateToVkAccessFlags2(ResourceState state)
	{
		// clang-format off
		switch (state)
		{
		case ResourceState::Undefined:			return VK_ACCESS_2_NONE;
		case ResourceState::Common:				return VK_ACCESS_2_NONE;
		case ResourceState::VertexBuffer:		return VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
		case ResourceState::IndexBuffer:		return VK_ACCESS_2_INDEX_READ_BIT;
		case ResourceState::ConstantBuffer:	    return VK_ACCESS_2_UNIFORM_READ_BIT;
		case ResourceState::ShaderResource:		return VK_ACCESS_2_SHADER_READ_BIT;
		case ResourceState::UnorderedAccess:	return VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
		case ResourceState::RenderTarget:		return VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		case ResourceState::IndirectBuffer:		return VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
		case ResourceState::Present:			return VK_ACCESS_2_NONE;
		case ResourceState::DepthRead:			return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		case ResourceState::DepthWrite:			return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		case ResourceState::CopySource:			return VK_ACCESS_2_TRANSFER_READ_BIT;
		case ResourceState::CopyDest:		    return VK_ACCESS_2_TRANSFER_WRITE_BIT;
		case ResourceState::ResolveSource:      return VK_ACCESS_2_TRANSFER_READ_BIT;
		case ResourceState::ResolveDest:	    return VK_ACCESS_2_TRANSFER_WRITE_BIT;
		case ResourceState::InitialRenderTarget:return VK_ACCESS_2_NONE;
		default:
			LOG_ERROR("Unexpected resource state");
			return 0;
		}
		// clang-format on
	}

	static VkPipelineStageFlags2 resourceStateToVkPipelineStageFlags2(ResourceState state)
	{
		constexpr VkPipelineStageFlags2 allShaderStages = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT |
			VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		// clang-format off
		switch (state)
		{
		case ResourceState::Undefined:			return VK_PIPELINE_STAGE_2_NONE;
		case ResourceState::Common:				return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		case ResourceState::VertexBuffer:		return VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
		case ResourceState::IndexBuffer:		return VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
		case ResourceState::ConstantBuffer:	    return allShaderStages;
		case ResourceState::ShaderResource:		return allShaderStages;
		case ResourceState::UnorderedAccess:	return allShaderStages;
		case ResourceState::RenderTarget:		return VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		case ResourceState::IndirectBuffer:		return VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
		case ResourceState::Present:			return VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
		case ResourceState::DepthRead:			return VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		case ResourceState::DepthWrite:			return VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		case ResourceState::CopySource:			return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		case ResourceState::CopyDest:		    return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		case ResourceState::ResolveSource:      return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		case ResourceState::ResolveDest:	    return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		case ResourceState::InitialRenderTarget:return VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		default:
			LOG_ERROR("Unexpected resource state");
			return 0;
		}
		// clang-format on
	}

	VkAccessFlags2 resourceStatesToVkAccessFlags2(ResourceState states)
	{
		const uint32_t numStateBits = 32;
		uint32_t stateTmp = uint32_t(states);
		uint32_t bitIndex = 0;

		VkAccessFlags2 accessflags = 0;
		while (stateTmp != 0 && bitIndex < numStateBits)
		{
			uint32_t bit = (1 << bitIndex);
			if (bit & stateTmp)
			{
				accessflags |= resourceStateToVkAccessFlags2(ResourceState(bit));
				stateTmp &= ~bit;
			}
			bitIndex++;
		}

		return accessflags;
	}

	VkPipelineStageFlags2 resourceStatesToVkPipelineStageFlags2(ResourceState states)
	{
		const uint32_t numStateBits = 32;
		uint32_t stateTmp = uint32_t(states);
		uint32_t bitIndex = 0;

		VkPipelineStageFlags2 pipelineStages = 0;
		while (stateTmp != 0 && bitIndex < numStateBits)
		{
			uint32_t bit = (1 << bitIndex);
			if (bit & stateTmp)
			{
				pipelineStages |= resourceStateToVkPipelineStageFlags2(ResourceState(bit));
				stateTmp &= ~bit;
			}
			bitIndex++;
		}

		return pipelineStages;
	}

	VkImageLayout resourceStateToVkImageLayout(ResourceState state)
	{
		// clang-format off
		switch (state)
		{
		case ResourceState::Undefined:			return VK_IMAGE_LAYOUT_UNDEFINED;
		case ResourceState::Common:				return VK_IMAGE_LAYOUT_GENERAL;
		case ResourceState::ShaderResource:		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case ResourceState::UnorderedAccess:	return VK_IMAGE_LAYOUT_GENERAL;
		case ResourceState::RenderTarget:		return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case ResourceState::Present:			return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		case ResourceState::DepthRead:			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		case ResourceState::DepthWrite:			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case ResourceState::CopySource:			return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		case ResourceState::CopyDest:		    return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		case ResourceState::ResolveSource:      return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		case ResourceState::ResolveDest:	    return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		case ResourceState::InitialRenderTarget:return VK_IMAGE_LAYOUT_UNDEFINED;
		default:
			LOG_ERROR("Unexpected resource state");
			return VK_IMAGE_LAYOUT_UNDEFINED;
		}
		// clang-format on
	}

	Object ResourceSetVk::getNativeObject(NativeObjectType type) const
	{
		// now we use push descriptor, no longer exists VkDescriptorSet.
		return nullptr;
	}

	Object ResourceSetLayoutVk::getNativeObject(NativeObjectType type) const
	{
		if (type == NativeObjectType::VK_DescriptorSetLayout)
		{
			return static_cast<Object>(descriptorSetLayout);
		}
		return nullptr;
	}

	ResourceSetLayoutVk::~ResourceSetLayoutVk()
	{
		assert(descriptorSetLayout != VK_NULL_HANDLE);
		vkDestroyDescriptorSetLayout(m_Context.device, descriptorSetLayout, nullptr);
	}

	ShaderVk::~ShaderVk()
	{
		specializationConstants.clear();
		assert(shaderModule != VK_NULL_HANDLE);
		vkDestroyShaderModule(m_Context.device, shaderModule, nullptr);
	}

	VkShaderStageFlagBits shaderTypeToVkShaderStageFlagBits(ShaderType shaderType)
	{
		assert(shaderType != ShaderType::Unknown);

		uint32_t result = 0;
		// clang-format off
		if ((shaderType & ShaderType::Compute) != 0)                result |= uint32_t(VK_SHADER_STAGE_COMPUTE_BIT);
		if ((shaderType & ShaderType::Vertex) != 0)                 result |= uint32_t(VK_SHADER_STAGE_VERTEX_BIT);
		if ((shaderType & ShaderType::TessellationControl) != 0)    result |= uint32_t(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
		if ((shaderType & ShaderType::TessellationEvaluation) != 0) result |= uint32_t(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
		if ((shaderType & ShaderType::Geometry) != 0)               result |= uint32_t(VK_SHADER_STAGE_GEOMETRY_BIT);
		if ((shaderType & ShaderType::Fragment) != 0)               result |= uint32_t(VK_SHADER_STAGE_FRAGMENT_BIT);
		if ((shaderType & ShaderType::Mesh) != 0)                   result |= uint32_t(VK_SHADER_STAGE_MESH_BIT_EXT);
		if ((shaderType & ShaderType::Task) != 0)                   result |= uint32_t(VK_SHADER_STAGE_TASK_BIT_EXT);
		// clang-format on
		return VkShaderStageFlagBits(result);
	}

	VkDescriptorType shaderResourceTypeToVkDescriptorType(ShaderResourceType type)
	{
		VkDescriptorType descriptorType{};
		switch (type)
		{
		case ShaderResourceType::TextureWithSampler:
			descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			break;
		case ShaderResourceType::Sampler:
			descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			break;
		case ShaderResourceType::UniformBuffer:
			descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			break;
		case ShaderResourceType::StorageBuffer:
			descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			break;
		case ShaderResourceType::SampledTexture:
			descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			break;
		case ShaderResourceType::StorageTexture:
			descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			break;
		case ShaderResourceType::UniformTexelBuffer:
			descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
			break;
		case ShaderResourceType::StorageTexelBuffer:
			descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
			break;
		case ShaderResourceType::Unknown:
		default:
			assert(!"unknown ShaderResourceType");
			break;
		}
		return descriptorType;
	}

	VkSamplerAddressMode convertVkSamplerAddressMode(SamplerAddressMode mode)
	{
		switch (mode)
		{
		case SamplerAddressMode::ClampToEdge:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

		case SamplerAddressMode::Repeat:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;

		case SamplerAddressMode::ClampToBorder:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

		case SamplerAddressMode::MirroredRepeat:
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

		case SamplerAddressMode::MirrorClampToEdge:
			return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;

		default:
			assert(!"Invalid SamplerAddressMode");
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}
	}

	VkBorderColor convertVkBorderColor(BorderColor color)
	{
		switch (color)
		{
		case rhi::BorderColor::FloatOpaqueBlack:
			return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		case rhi::BorderColor::FloatOpaqueWhite:
			return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		case rhi::BorderColor::FloatTransparentBlack:
			return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		default:
			assert(!"Invalid BorderColor");
			return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		}
	}

	VkClearColorValue convertVkClearColor(ClearColor color, Format textureFormat)
	{
		VkClearColorValue val{};
		const FormatInfo& formatInfo = getFormatInfo(textureFormat);

		if (formatInfo.componentType == FormatComponentType::Float)
		{
			for (int i = 0; i < 4; ++i)
			{
				val.float32[i] = color.float32[i];
			}
		}
		else if (formatInfo.componentType == FormatComponentType::Uint)
		{
			for (int i = 0; i < 4; ++i)
			{
				val.uint32[i] = color.uint32[i];
			}
		}
		else 
		{
			for (int i = 0; i < 4; ++i)
			{
				val.int32[i] = color.int32[i];
			}
		}

		return val;
	}
}