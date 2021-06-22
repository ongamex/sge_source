#pragma once

#include "sge_utils/math/mat4.h"
#include "sge_utils/math/vec2.h"

#include <array>
#include <cfloat>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

namespace sge {

#define SGE_GPRAHICS_COMMON_ENUM_HIDE virtual void operator()() = 0;

enum APICommand {
	APICommand_DrawCall,
	APICommand_MapDiscardCmd,
	APICommand_ClearColorCmd,
	APICommand_ClearDepthStencilCmd,

	APICommand_Num,
};

//-------------------------------------------------------------------
// Static Device and Graphics limitations.
//-------------------------------------------------------------------
namespace GraphicsCaps {
	enum : int {
#if SGE_RENDERER_D3D11
		// TODO: Real values.
		kVertexBufferSlotsCount = 8,   // D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
		kConstantBufferSlotsCount = 8, // 14
		kRenderTargetSlotsCount = 8,

		kD3D11_SRV_Count = 8,  //[TODO]
		kSampleSlotsCount = 8, //[TODO]

#elif SGE_RENDERER_GL

		// TODO: Real values.
		// https://www.opengl.org/sdk/docs/man2/xhtml/glVertexAttribPointer.xml
		// GL_INVALID_VALUE is generated if index is greater than or equal to GL_MAX_VERTEX_ATTRIBS.

		kVertexBufferSlotsCount = 8,
		kConstantBufferSlotsCount = 8,
		kRenderTargetSlotsCount = 8,

#endif
	};
}; // namespace GraphicsCaps

//-------------------------------------------------------------------
//
//-------------------------------------------------------------------
struct ShadingLanguage {
	enum Enum {
		Unknown,
		Common,
		HLSL,
		GLSL,

#if SGE_RENDERER_D3D11
		ApiNative = HLSL,
#elif SGE_RENDERER_GL
		ApiNative = GLSL,
#else
	// Not implemented.
#endif
	};

	SGE_GPRAHICS_COMMON_ENUM_HIDE;
};

//-------------------------------------------------------------------
//
//-------------------------------------------------------------------
struct SampleDesc {
	SampleDesc(const int _count = 1, const int _quality = 0)
	    : Count(_count)
	    , Quality(_quality) {}

	int Count;   // default is 1
	int Quality; // default is 0
};

//-------------------------------------------------------------------
//
//-------------------------------------------------------------------
struct Rect2s {
	Rect2s() = default;
	Rect2s(const short width, const short height)
	    : width(width)
	    , height(height)
	    , x(0)
	    , y(0) {}

	Rect2s(const short width, const short height, const short x, const short y)
	    : width(width)
	    , height(height)
	    , x(x)
	    , y(y) {}

	bool isEmpty() const { return width != 0 && height != 0; }
	float ratioWbyH() const { return (float)width / (float)height; }

	vec2f getSizeFloats() const { return vec2f(width, height); }

	short width = 0;
	short height = 0;
	short x = 0;
	short y = 0;
};

//-------------------------------------------------------------------
//
//-------------------------------------------------------------------
struct ResourceBindFlags {
	enum Enum {
		VertexBuffer = 1,
		IndexBuffer = 2,
		ConstantBuffer = 4,
		ShaderResource = 8,
		StreamOutput = 16,
		UnorderedAccess = 128,

		Force32bit = 0xffffffff
	};

	SGE_GPRAHICS_COMMON_ENUM_HIDE;
};

//-------------------------------------------------------------------
// struct ResourceUsage
//-------------------------------------------------------------------
struct ResourceUsage {
	enum Enum {

		Immutable, // Once created those resource can only be read.
		Default,   // Gpu can read and write from/to those resources
		Dynamic,   // Gpu can read those resources, Host application can write
		           // Staging,
	};

	SGE_GPRAHICS_COMMON_ENUM_HIDE;
};

struct ResourceType {
	enum Enum {
		Unknown,
		Buffer,
		Texture,
		Sampler,
		FrameTarget, // Frame buffer or a collection of render targets + depth buffer
		Shader,
		ShadingProgram,
		Query,
		VertexMapper,
		RasterizerState,
		DepthStencilState,
		BlendState,

		NumElements,
	};

	SGE_GPRAHICS_COMMON_ENUM_HIDE;
};

//-------------------------------------------------------------------
//
//-------------------------------------------------------------------
struct Map {
	enum Enum { Read, Write, ReadWrite, WriteDiscard };

	SGE_GPRAHICS_COMMON_ENUM_HIDE;
};

//-------------------------------------------------------------------
// struct TextureFormat
//[TODO]http://msdn2.microsoft.com/en-us/library/bb694531(VS.85).aspx
//-------------------------------------------------------------------
struct TextureFormat {
	enum Enum {
		Unknown,

		R32G32B32A32_FLOAT,
		R32G32B32A32_UINT,
		R32G32B32A32_SINT,

		R32G32B32_FLOAT,
		R32G32B32_UINT,
		R32G32B32_SINT,

		R16G16B16A16_FLOAT,
		R16G16B16A16_UNORM,
		R16G16B16A16_UINT,
		R16G16B16A16_SNORM,
		R16G16B16A16_SINT,

		R32G32_FLOAT,
		R32G32_UINT,
		R32G32_SINT,

		R10G10B10A2_UNORM,
		R10G10B10A2_UINT,
		R11G11B10_FLOAT,

		R8G8B8A8_UNORM,
		R8G8B8A8_UNORM_SRGB,
		R8G8B8A8_UINT,
		R8G8B8A8_SNORM,
		R8G8B8A8_SINT,

		R16G16_FLOAT,
		R16G16_UNORM,
		R16G16_UINT,
		R16G16_SNORM,
		R16G16_SINT,

		R32_FLOAT,
		R32_UINT,
		R32_SINT,

		R8G8_UNORM,
		R8G8_UINT,
		R8G8_SNORM,
		R8G8_SINT,

		R16_FLOAT,
		R16_UNORM,
		R16_UINT,
		R16_SNORM,
		R16_SINT,

		R8_UNORM,
		R8_UINT,
		R8_SNORM,
		R8_SINT,

		A8_UNORM,

		// Block Compression formats.
		MARKER_BC_COMPRESSION_BEGIN,

		BC1_UNORM, // DXT1
		BC2_UNORM, // DXT3 and DXT2 (DXT2 is with premultiplied alpha as far as I know)
		BC3_UNORM, // DXT5 and DXT4 (DXT4 is with premultippied alpha as far as I know)

		BC4_UNORM, // ATI1 and BC5U
		BC4_SNORM, // BC4S

		BC5_UNORM, // ATI2 and BC5U
		BC5_SNORM, // BC5S

		MARKER_BC_COMPRESSION_END,


		// Depth and depth stencil formats.
		MARKER_DEPTH_BEGIN,

		D24,

		MARKER_DEPTH_HAS_STENCIL_BEGIN,

		D24_UNORM_S8_UINT, // On D3D11 this is only RED channel is set. On OpenGL All channels should be 1.f
		D32_FLOAT,

		MARKER_DEPTH_HAS_STENCIL_END,
		MARKER_DEPTH_END,
	};

	// Returns the corresponding format size in bits
	static size_t GetSizeBits(const TextureFormat::Enum format);

	// Returns the corresponding format size in bytes
	static size_t GetSizeBytes(const TextureFormat::Enum format);

	// Check if the texture format is suitable for depth buffer
	static bool IsDepth(const TextureFormat::Enum format) { return (format > MARKER_DEPTH_BEGIN) && (format < MARKER_DEPTH_END); }

	// Check if the texture format is a BC format.
	static bool IsBC(const TextureFormat::Enum format) {
		return (format > MARKER_BC_COMPRESSION_BEGIN) && (format < MARKER_BC_COMPRESSION_END);
	}

	// Checks if the texture format has both depth and stencil
	static bool IsDepthWithStencil(const TextureFormat::Enum format) {
		return (format > MARKER_DEPTH_HAS_STENCIL_BEGIN) && (format < MARKER_DEPTH_HAS_STENCIL_END);
	}

	SGE_GPRAHICS_COMMON_ENUM_HIDE;
};

//-------------------------------------------------------------------
// struct UniformType
//-------------------------------------------------------------------
struct UniformType {
	enum Enum {
		Unknown = 0,

		MARKER_NumericUniformsBegin,

		Float,
		Float2,
		Float3,
		Float4,

		Double,
		Double2,
		Double3,
		Double4,

		Int,
		Int2,
		Int3,
		Int4,

		Uint,
		Uint2,
		Uint3,
		Uint4,

		Uint16,

		Float3x3,
		Float4x4,

		MARKER_NumericUniformsEnd,

		Texture1D,
		Texture2D,
		TextureCube,
		Texture3D,
		ConstantBuffer,
		SamplerState,

		// Caution: Usable only by the input assambler.
		// An int that gets expanded to 4 floats when used as an vertex attribute.
		Int_RGBA_Unorm_IA,
	};

	static bool isNumeric(Enum const e) { return e > MARKER_NumericUniformsBegin && e < MARKER_NumericUniformsEnd; }

	// [NOTE][TODO] This is a GL only function don't use it elsewhere.
	// Pick the correct type by given basic type, num rows and columns
	// for example PickType(Float, 3, 1) will output Float3
	static UniformType::Enum PickType(UniformType::Enum type, const int lanes, const int registers);

	// Returns the CPU size of the uniform data in that type.
	static int GetSizeBytes(const UniformType::Enum uniformType);

	SGE_GPRAHICS_COMMON_ENUM_HIDE;
};


//-------------------------------------------------------------------
// struct TextureDesc
//-------------------------------------------------------------------
struct TextureData {
	TextureData(const void* data = nullptr, size_t rowByteSize = 0, size_t sliceByteSize = 0)
	    : data(data)
	    , rowByteSize(rowByteSize)
	    , sliceByteSize(sliceByteSize) {}

	const void* data;
	size_t rowByteSize;
	size_t sliceByteSize;
};

struct Texture1DDesc {
	Texture1DDesc() = default;

	Texture1DDesc(int width, const int numMips, int arraySize)
	    : width(width)
	    , numMips(numMips)
	    , arraySize(arraySize) {}

	int width;
	int numMips;
	int arraySize;
};

struct Texture2DDesc {
	Texture2DDesc() = default;

	Texture2DDesc(int width, int height, int numMips = 1, int arraySize = 1, int numSamples = 1, int sampleQuality = 0)
	    : width(width)
	    , height(height)
	    , numMips(numMips)
	    , arraySize(arraySize)
	    , numSamples(numSamples)
	    , sampleQuality(sampleQuality) {}

	int width;
	int height;
	int numMips;
	int arraySize;
	int numSamples, sampleQuality;
};

struct TextureCubeDesc {
	TextureCubeDesc() = default;

	TextureCubeDesc(int width, int height, int numMips = 1, int arraySize = 1, int numSamples = 1, int sampleQuality = 0)
	    : width(width)
	    , height(height)
	    , numMips(numMips)
	    , arraySize(arraySize)
	    , numSamples(numSamples)
	    , sampleQuality(sampleQuality) {}

	int width;
	int height;
	int numMips;
	int arraySize; // The number of cube textures described.
	int numSamples, sampleQuality;
};

struct Texture3DDesc {
	Texture3DDesc() = default;

	Texture3DDesc(int width, int height, int depth, int numMips)
	    : width(width)
	    , height(height)
	    , depth(depth)
	    , numMips(numMips) {}

	int width;
	int height;
	int depth;
	int numMips;
};

struct TextureUsage {
	enum Enum {
		ImmutableResource,
		DynamicResource,
		RenderTargetResource,
		RenderTargetOnly, // cannot be used as shader resource
		DepthStencilResource,
		DepthStencilOnly, // cannot be used as shader resource
	};

	static bool CanBeShaderResource(const TextureUsage::Enum e) {
		return e == ImmutableResource || e == DynamicResource || e == RenderTargetResource || e == DepthStencilResource;
	}

	static bool CanBeRenderTarget(const TextureUsage::Enum e) { return e == RenderTargetResource || e == RenderTargetOnly; }

	static bool CanBeDepthStencil(const TextureUsage::Enum e) { return e == DepthStencilResource || e == DepthStencilOnly; }

	SGE_GPRAHICS_COMMON_ENUM_HIDE;
};

struct TextureDesc {
	TextureUsage::Enum usage;
	UniformType::Enum textureType;
	TextureFormat::Enum format;

	union {
		Texture1DDesc texture1D;
		Texture2DDesc texture2D;
		TextureCubeDesc textureCube;
		Texture3DDesc texture3D;
	};

	bool hasMipMaps() const {
		if (textureType == UniformType::Texture1D)
			return texture1D.numMips > 1;
		if (textureType == UniformType::Texture2D)
			return texture2D.numMips > 1;
		if (textureType == UniformType::TextureCube)
			return textureCube.numMips > 1;
		if (textureType == UniformType::Texture3D)
			return texture3D.numMips > 1;

		return false;
	}

	/// @brief Creates a TextureDesc for a color render target.
	/// @param width of the texture.
	/// @param height of the texture.
	/// @param pixel format of the texture. Good default is TextureFormat::R8G8B8A8_UNORM.
	static TextureDesc GetDefaultRenderTarget(int width, int height, TextureFormat::Enum format);

	/// @brief Create a TextureDFesc for depth render target (depth buffer, depth texure).
	/// @param width of the texture.
	/// @param height of the texture.
	/// @param pixel format of the texture. Good default is TextureFormat::D24_UNORM_S8_UINT
	static TextureDesc GetDefaultDepthStencil(int width, int height, TextureFormat::Enum format);
};

// [TODO] Concider arrays for RT and DB
struct TargetDesc {
	UniformType::Enum baseTextureType = UniformType::Unknown;

	struct {
		int arrayIdx = 0;
		int mipLevel = 0;
	} texture1D;

	struct {
		int arrayIdx = 0;
		int mipLevel = 0; // Ignored for MS textures.
	} texture2D;

	struct {
		SignedAxis face = axis_x_pos;
		int mipLevel = 0;
	} textureCube;


	bool operator==(const TargetDesc& other) const {
		if (baseTextureType == UniformType::Texture1D) {
			if (texture1D.arrayIdx != other.texture1D.arrayIdx)
				return false;
			if (texture1D.mipLevel != other.texture1D.mipLevel)
				return false;
			return true;
		} else if (baseTextureType == UniformType::Texture2D) {
			if (texture2D.arrayIdx != other.texture2D.arrayIdx)
				return false;
			if (texture2D.mipLevel != other.texture2D.mipLevel)
				return false;
			return true;
		} else if (baseTextureType == UniformType::TextureCube) {
			if (textureCube.face != other.textureCube.face)
				return false;
			if (textureCube.mipLevel != other.textureCube.mipLevel)
				return false;
			return true;
		} else {
			// Not Implemented Yet.
			sgeAssert(false);
		}

		return false;
	}

	bool operator!=(const TargetDesc& other) const { return !(*this == other); }

	static TargetDesc FromTex2D(int arrayIdx = 0, int mipLevel = 0) {
		TargetDesc result;
		result.baseTextureType = UniformType::Texture2D;
		result.texture2D.arrayIdx = arrayIdx;
		result.texture2D.mipLevel = mipLevel;
		return result;
	}

	static TargetDesc fromTexCubeFace(const SignedAxis face, int mipLevel = 0) {
		TargetDesc result;
		result.baseTextureType = UniformType::TextureCube;
		result.textureCube.face = face;
		result.textureCube.mipLevel = mipLevel;

		return result;
	}
};

//-------------------------------------------------------------------
// Samplers
//-------------------------------------------------------------------
enum class ESamplerType { Unknown, Sampler1D, Sampler2D, Sampler3D, SamplerCube };

//-------------------------------------------------------------------
//
//-------------------------------------------------------------------
struct TextureAddressMode {
	enum Enum { Repeat, ClampEdge, ClampBorder };

	SGE_GPRAHICS_COMMON_ENUM_HIDE;
};

struct TextureFilter {
	enum Enum {
		Min_Mag_Mip_Point,
		Min_Mag_Mip_Linear,
		Anisotropic,
	};

	SGE_GPRAHICS_COMMON_ENUM_HIDE
};

//-------------------------------------------------------------------
// [NOTE] OpenGL Sampler State specs.
// https://www.opengl.org/wiki/GLAPI/glSamplerParameter
// https://www.opengl.org/wiki/GLAPI/glTexParameter
//-------------------------------------------------------------------
struct SamplerDesc {
	SamplerDesc() {
		filter = TextureFilter::Min_Mag_Mip_Linear;
		for (auto& v : addressModes) {
			v = TextureAddressMode::Repeat;
		}
		colorBorder[0] = 0.f;
		colorBorder[1] = 0.f;
		colorBorder[2] = 0.f;
		colorBorder[3] = 1.f;
		minLOD = -FLT_MAX;
		maxLOD = FLT_MAX;
		maxAnisotropy = 1;
	}

	TextureFilter::Enum filter;
	TextureAddressMode::Enum addressModes[3];
	float colorBorder[4];
	float minLOD;
	float maxLOD;
	uint32 maxAnisotropy; // 1- 16

	bool operator==(const SamplerDesc& other) const {
		// clang-format off
		return 
			   filter == other.filter 
			&& addressModes[0] == other.addressModes[0] 
			&& addressModes[1] == other.addressModes[1] 
			&& addressModes[2] == other.addressModes[2] 
			&& colorBorder[0] == other.colorBorder[0] 
			&& colorBorder[1] == other.colorBorder[1] 
			&& colorBorder[2] == other.colorBorder[2] 
			&& colorBorder[3] == other.colorBorder[3] 
			&& minLOD == other.minLOD && maxLOD == other.maxLOD;
		// clang-format on
	}

	bool operator!=(const SamplerDesc& other) const { return !(*this == other); }
};

//-------------------------------------------------------------------
// Buffers
//-------------------------------------------------------------------
struct BufferDesc {
	size_t sizeBytes;
	ResourceUsage::Enum usage;
	int bindFlags;
	size_t structByteStride; // size in bytes of a single <strctured buffer> element

	//[TODO]some useful functions
	static BufferDesc GetDefaultVertexBuffer(const size_t sizeBytes, const ResourceUsage::Enum usage = ResourceUsage::Immutable);
	static BufferDesc GetDefaultIndexBuffer(const size_t sizeBytes, const ResourceUsage::Enum usage = ResourceUsage::Immutable);
	static BufferDesc GetDefaultConstantBuffer(const size_t sizeBytes, const ResourceUsage::Enum usage);
};

//-------------------------------------------------------------------
// ShaderImpl Types
//-------------------------------------------------------------------
struct ShaderType {
	enum Enum {
		VertexShader,
		PixelShader,
		// ComputeShader,

		NumElems
	};

	SGE_GPRAHICS_COMMON_ENUM_HIDE;
};

//-------------------------------------------------------------------
// Primitive topology
//-------------------------------------------------------------------
struct PrimitiveTopology {
	enum Enum {
		Unknown,
		TriangleList,
		TriangleStrip,
		LineList,
		LineStrip,
		PointList,
	};

	// returns the maximum amount primitives constructed by given topology
	static int GetNumPrimitivesByPoints(const PrimitiveTopology::Enum topology, const int numPoints);

	// returns the need amount of points to construct numPrimitives by given topology
	static int GetNumPointsByPrimitives(const PrimitiveTopology::Enum topology, const int numPrimitives);

	SGE_GPRAHICS_COMMON_ENUM_HIDE;
};

//-------------------------------------------------------------------
// VertexDeclaration
//
// Used to specify the shader inputs, vertex shader layout ect...
// Currently MATRICES aren't supported. You can use up to 4 component vector here!
//-------------------------------------------------------------------
struct VertexDecl {
	short bufferSlot;
	std::string semantic;
	UniformType::Enum format;
	int byteOffset;

	VertexDecl() = default;
	~VertexDecl() = default;

	VertexDecl(short bufferSlot, const char* semantic, UniformType::Enum format, short byteOffset)
	    : bufferSlot(bufferSlot)
	    , semantic(semantic ? semantic : "")
	    , format(format)
	    , byteOffset(byteOffset) {}

	bool operator==(const VertexDecl& other) const {
		return (bufferSlot == other.bufferSlot) && (semantic == other.semantic) && (byteOffset == other.byteOffset) &&
		       (format == other.format);
	}

	bool operator!=(const VertexDecl& other) const { return !operator==(other); }

	bool operator<(const VertexDecl& ref) const {
		return ref.bufferSlot > bufferSlot || ref.format > format || ref.byteOffset > byteOffset ||
		       strcmp(ref.semantic.c_str(), semantic.c_str()) < 0;
	}

	// Reorders the vertex declaration.
	// All vertex elements that share the same vertex buffer slot are continious
	// and sorted by byte offset. if byte offset is -1 it's computed based on the previous elements.
	static std::vector<VertexDecl> NormalizeDecl(const VertexDecl* pDecl, const int numDeclElems);
};

enum { VertexDeclIndex_Null = 0 };
typedef int VertexDeclIndex;

//-------------------------------------------------------------------
// FillMode
//-------------------------------------------------------------------
struct FillMode {
	enum Enum { Solid, Wireframe };

	SGE_GPRAHICS_COMMON_ENUM_HIDE;
};

//-------------------------------------------------------------------
// CullMode
//-------------------------------------------------------------------
struct CullMode {
	enum Enum { Back, Front, None };

	SGE_GPRAHICS_COMMON_ENUM_HIDE;
};

//-------------------------------------------------------------------
// DepthComparisonFunc
//-------------------------------------------------------------------
struct DepthComparisonFunc {
	enum Enum {
		Never,
		Always,
		Less,
		LessEqual,
		Greater,
		GreaterEqual,
		Equal,
		NotEqual,
	};

	SGE_GPRAHICS_COMMON_ENUM_HIDE;
};

//-------------------------------------------------------------------
// RasterDesc
//-------------------------------------------------------------------
struct RasterDesc {
	bool backFaceCCW; // Non-Zero if the Back faces are Clock-Wise oriented.
	CullMode::Enum cullMode;
	FillMode::Enum fillMode;
	bool useScissor;
	float depthBiasAdd = 0.f;
	float depthBiasSlope = 0.f;

	RasterDesc(const bool backFaceCW = false,
	           const CullMode::Enum cullMode = CullMode::Back,
	           const FillMode::Enum fillMode = FillMode::Solid,
	           const bool useScissor = false)
	    : backFaceCCW(backFaceCW)
	    , cullMode(cullMode)
	    , fillMode(fillMode)
	    , useScissor(useScissor) {}

	bool operator==(const RasterDesc& other) const {
		return backFaceCCW == other.backFaceCCW && cullMode == other.cullMode && fillMode == other.fillMode &&
		       useScissor == other.useScissor && depthBiasAdd == other.depthBiasAdd && depthBiasSlope == other.depthBiasSlope;
	}

	bool operator!=(const RasterDesc& other) const { return !(*this == other); }
};

//-------------------------------------------------------------------
// DepthStencilDesc
//-------------------------------------------------------------------
struct DepthStencilDesc {
	bool depthTestEnabled;
	bool depthWriteEnabled;
	DepthComparisonFunc::Enum comparisonFunc;

	DepthStencilDesc() {
		depthTestEnabled = true;
		depthWriteEnabled = true;
		comparisonFunc = DepthComparisonFunc::Less;
	}

	bool operator==(const DepthStencilDesc& other) const {
		return depthTestEnabled == other.depthTestEnabled && depthWriteEnabled == other.depthWriteEnabled &&
		       comparisonFunc == other.comparisonFunc;
	}

	bool operator!=(const DepthStencilDesc& other) const { return !(*this == other); }
};

//-------------------------------------------------------------------
// Alpha Blending
//-------------------------------------------------------------------
struct Blend {
	enum Enum {
		Zero,
		One,

		SrcColor,
		InvSrcColor,

		DestColor,
		InvDestColor,

		// Alpha only.
		Alpha_Src,
		Alpha_InvSrc,
		Alpha_Dest,
		Alpha_InvDest,

	};

	SGE_GPRAHICS_COMMON_ENUM_HIDE
};

struct BlendOp {
	enum Enum {
		Add,
		Sub,
		RevSub,
		Min,
		Max,
	};

	SGE_GPRAHICS_COMMON_ENUM_HIDE
};

struct BlendDesc {
	// [TODO]
	// https://msdn.microsoft.com/en-us/library/windows/desktop/ff476087(v=vs.85).aspx
	// https://msdn.microsoft.com/en-us/library/windows/desktop/ff476200(v=vs.85).aspx
	// Alpha-to-Coverage
	// MRT Independant Blending !!!
	// Write masks !!!

	// Dest means the existing value in the renderer value.
	// Src is the output value of the currently run pixel shader.

	bool enabled = false;
	Blend::Enum srcBlend = Blend::One;
	Blend::Enum destBlend = Blend::Zero;
	BlendOp::Enum blendOp = BlendOp::Add;

	Blend::Enum alphaSrcBlend = Blend::One;
	Blend::Enum alphaDestBlend = Blend::Zero;
	BlendOp::Enum alphaBlendOp = BlendOp::Add;

	bool operator==(const BlendDesc& other) const;
	bool operator!=(const BlendDesc& other) const;

	static BlendDesc GetDefaultBackToFrontAlpha();
	static BlendDesc getColorAdditiveBlending();
};

struct BlendStateDesc {
	BlendStateDesc(const BlendDesc& blend) {
		independentBlend = false;
		blendDesc[0] = blend;
	}

	BlendStateDesc() {}

	// If undependant blending is enabled. Each render target
	// wiil use own blend desc. Otherwise all render targets will use blendDesc[0].
	bool independentBlend = false;
	std::array<BlendDesc, GraphicsCaps::kRenderTargetSlotsCount> blendDesc;

	bool operator==(const BlendStateDesc& other) const;
	bool operator!=(const BlendStateDesc& other) const;

	static BlendStateDesc GetDefaultBackToFrontAlpha();
	static BlendStateDesc getColorAdditiveBlending();
};

//-------------------------------------------------------------------
//
//-------------------------------------------------------------------
struct QueryType {
	enum Enum : int {
		NumSamplesPassedDepthStencilTest,
		AnySamplePassedDepthStencilTest,
	};
};

//-------------------------------------------------------------------
//
//-------------------------------------------------------------------
struct MainFrameTargetDesc {
	int width;
	int height;
	int numBuffers; // Front + num back buffers.
	bool vSync;

	//[TODO] unused for OpenGL currently.
	SampleDesc sampleDesc;

#if defined(WIN32)
	void* hWindow; // actually the type is HWND
	bool bWindowed;
#endif
};

struct FrameStatistics {
	FrameStatistics() = default;

	void Reset() { *this = FrameStatistics(); }

	int numDrawCalls = 0;
	size_t numPrimitiveDrawn = 0;
	float lastPresentTime = 0;
	float lastPresentDt = 0;
};

} // namespace sge
