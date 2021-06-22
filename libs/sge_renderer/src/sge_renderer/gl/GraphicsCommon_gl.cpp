#include "GraphicsCommon_gl.h"

namespace sge {

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
GLenum ResourceUsage_GetGLNative(const ResourceUsage::Enum type) {
	switch (type) {
		case ResourceUsage::Immutable:
			return GL_DYNAMIC_DRAW;
		case ResourceUsage::Default:
			return GL_DYNAMIC_DRAW;
		case ResourceUsage::Dynamic:
			return GL_DYNAMIC_DRAW;
	}

	sgeAssert(false); // Unknown usage.
	return GL_DYNAMIC_DRAW;
}

//------------------------------------------------------------------------------
//
// GL_READ_ONLY, GL_WRITE_ONLY, or GL_READ_WRITE
//------------------------------------------------------------------------------
#if !defined(__EMSCRIPTEN__)
GLenum Map_GetGLNative(const Map::Enum map) {
	switch (map) {
		case Map::Read:
			return GL_READ_ONLY;
		case Map::Write:
			return GL_WRITE_ONLY;
		case Map::ReadWrite:
			return GL_READ_WRITE;
		case Map::WriteDiscard:
			return GL_WRITE_ONLY; // [TODO]
	}

	sgeAssert(false); // Unknown type
	return GL_READ_ONLY;
}
#endif

//------------------------------------------------------------------------------
// TextureFormat
//------------------------------------------------------------------------------
void TextureFormat_GetGLNative(const TextureFormat::Enum format, GLint& glInternalFormat, GLenum& glFormat, GLenum& glType) {
	glInternalFormat = glFormat = glType = GL_NONE;

	switch (format) {
		case TextureFormat::Unknown:
			glInternalFormat = glFormat = glType = GL_NONE;
			return;
		case TextureFormat::R32G32B32A32_FLOAT:
			glInternalFormat = GL_RGBA32F;
			glFormat = GL_RGBA;
			glType = GL_FLOAT;
			return;
		case TextureFormat::R32G32B32A32_UINT:
			glInternalFormat = GL_RGBA32UI;
			glFormat = GL_RGBA_INTEGER;
			glType = GL_UNSIGNED_INT;
			return;
		case TextureFormat::R32G32B32A32_SINT:
			glInternalFormat = GL_RGBA32I;
			glFormat = GL_RGBA_INTEGER;
			glType = GL_INT;
			return;
		case TextureFormat::R32G32B32_FLOAT:
			glInternalFormat = GL_RGB32F;
			glFormat = GL_RGB;
			glType = GL_FLOAT;
			return;
		case TextureFormat::R32G32B32_UINT:
			glInternalFormat = GL_RGB32UI;
			glFormat = GL_RGB_INTEGER;
			glType = GL_UNSIGNED_INT;
			return;
		case TextureFormat::R32G32B32_SINT:
			glInternalFormat = GL_RGB32I;
			glFormat = GL_RGB_INTEGER;
			glType = GL_INT;
			return;
		case TextureFormat::R16G16B16A16_FLOAT:
			glInternalFormat = GL_RGBA16F;
			glFormat = GL_RGBA;
			glType = GL_HALF_FLOAT;
			return;
		case TextureFormat::R16G16B16A16_UNORM:
#if !defined(__EMSCRIPTEN__)
			glInternalFormat = GL_RGBA16;
			glFormat = GL_RGBA;
			glType = GL_UNSIGNED_SHORT;
#endif
			return;
		case TextureFormat::R16G16B16A16_UINT:
			glInternalFormat = GL_RGBA16UI;
			glFormat = GL_RGBA_INTEGER;
			glType = GL_UNSIGNED_SHORT;
			return;
		case TextureFormat::R16G16B16A16_SNORM:
#if !defined(__EMSCRIPTEN__)
			glInternalFormat = GL_RGBA16_SNORM;
			glFormat = GL_RGBA;
			glType = GL_SHORT;
#endif
			return;
		case TextureFormat::R16G16B16A16_SINT:
			glInternalFormat = GL_RGBA16I;
			glFormat = GL_RGBA_INTEGER;
			glType = GL_SHORT;
			return;
		case TextureFormat::R32G32_FLOAT:
			glInternalFormat = GL_RG32F;
			glFormat = GL_RG;
			glType = GL_FLOAT;
			return;
		case TextureFormat::R32G32_UINT:
			glInternalFormat = GL_RG32UI;
			glFormat = GL_RG_INTEGER;
			glType = GL_UNSIGNED_INT;
			return;
		case TextureFormat::R32G32_SINT:
			glInternalFormat = GL_RG32I;
			glFormat = GL_RG_INTEGER;
			glType = GL_INT;
			return;
#if !defined(__EMSCRIPTEN__)
		case TextureFormat::R10G10B10A2_UNORM:
			glInternalFormat = GL_RGB10_A2;
			glFormat = GL_RGBA;
			glType = GL_UNSIGNED_INT_10_10_10_2;
			return;
		case TextureFormat::R10G10B10A2_UINT:
			glInternalFormat = GL_RGB10_A2UI;
			glFormat = GL_RGBA_INTEGER;
			glType = GL_UNSIGNED_INT_10_10_10_2;
			return;
#endif
		case TextureFormat::R11G11B10_FLOAT:
			glInternalFormat = GL_R11F_G11F_B10F;
			glFormat = GL_RGB;
			glType = GL_UNSIGNED_INT_10F_11F_11F_REV;
			return;
		case TextureFormat::R8G8B8A8_UNORM:
			glInternalFormat = GL_RGBA; // GL_RGBA8 works on Desktop but not on WebGL for some alien reason.
			glFormat = GL_RGBA;
			glType = GL_UNSIGNED_BYTE;
			return;
		case TextureFormat::R8G8B8A8_UNORM_SRGB:
			glInternalFormat = GL_SRGB8_ALPHA8;
			glFormat = GL_RGBA;
			glType = GL_UNSIGNED_BYTE;
			return;
		case TextureFormat::R8G8B8A8_UINT:
			glInternalFormat = GL_RGBA8UI;
			glFormat = GL_RGBA_INTEGER;
			glType = GL_UNSIGNED_BYTE;
			return;
		case TextureFormat::R8G8B8A8_SNORM:
			glInternalFormat = GL_RGBA8_SNORM;
			glFormat = GL_RGBA;
			glType = GL_BYTE;
			return;
		case TextureFormat::R8G8B8A8_SINT:
			glInternalFormat = GL_RGBA8I;
			glFormat = GL_RGBA_INTEGER;
			glType = GL_BYTE;
			return;
#if !defined(__EMSCRIPTEN__)
		case TextureFormat::R16G16_FLOAT:
			glInternalFormat = GL_RG16F;
			glFormat = GL_RG;
			glType = GL_SHORT;
			return;
		case TextureFormat::R16G16_UNORM:
			glInternalFormat = GL_RG16;
			glFormat = GL_RG;
			glType = GL_UNSIGNED_SHORT;
			return;
		case TextureFormat::R16G16_UINT:
			glInternalFormat = GL_RG16UI;
			glFormat = GL_RG_INTEGER;
			glType = GL_UNSIGNED_SHORT;
			return;
		case TextureFormat::R16G16_SNORM:
			glInternalFormat = GL_RG16_SNORM;
			glFormat = GL_RG;
			glType = GL_SHORT;
			return;
		case TextureFormat::R16G16_SINT:
			glInternalFormat = GL_RG16I;
			glFormat = GL_RG_INTEGER;
			glType = GL_SHORT;
			return;
#endif
		case TextureFormat::R32_FLOAT:
			glInternalFormat = GL_R32F;
			glFormat = GL_RED;
			glType = GL_FLOAT;
			return;
		case TextureFormat::R32_UINT:
			glInternalFormat = GL_R32UI;
			glFormat = GL_RED_INTEGER;
			glType = GL_UNSIGNED_INT;
			return;
		case TextureFormat::R32_SINT:
			glInternalFormat = GL_R32I;
			glFormat = GL_RED_INTEGER;
			glType = GL_INT;
			return;
		case TextureFormat::R8G8_UNORM:
			glInternalFormat = GL_RG8;
			glFormat = GL_RG;
			glType = GL_UNSIGNED_BYTE;
			return;
		case TextureFormat::R8G8_UINT:
			glInternalFormat = GL_RG8UI;
			glFormat = GL_RG_INTEGER;
			glType = GL_UNSIGNED_BYTE;
			return;
		case TextureFormat::R8G8_SNORM:
			glInternalFormat = GL_RG8_SNORM;
			glFormat = GL_RG;
			glType = GL_BYTE;
			return;
		case TextureFormat::R8G8_SINT:
			glInternalFormat = GL_RG8I;
			glFormat = GL_RG_INTEGER;
			glType = GL_BYTE;
			return;
#if !defined(__EMSCRIPTEN__)
		case TextureFormat::R16_FLOAT:
			glInternalFormat = GL_R16F;
			glFormat = GL_RED;
			glType = GL_HALF_FLOAT;
			return;
		case TextureFormat::R16_UNORM:
			glInternalFormat = GL_R16;
			glFormat = GL_RED;
			glType = GL_UNSIGNED_SHORT;
			return;
		case TextureFormat::R16_UINT:
			glInternalFormat = GL_R16UI;
			glFormat = GL_RED_INTEGER;
			glType = GL_UNSIGNED_SHORT;
			return;
		case TextureFormat::R16_SNORM:
			glInternalFormat = GL_R16_SNORM;
			glFormat = GL_RED;
			glType = GL_SHORT;
			return;
		case TextureFormat::R16_SINT:
			glInternalFormat = GL_R16I;
			glFormat = GL_RED_INTEGER;
			glType = GL_SHORT;
			return;
#endif
		case TextureFormat::R8_UNORM:
			glInternalFormat = GL_R8;
			glFormat = GL_RED;
			glType = GL_UNSIGNED_BYTE;
			return;
		case TextureFormat::R8_UINT:
			glInternalFormat = GL_R8UI;
			glFormat = GL_RED_INTEGER;
			glType = GL_UNSIGNED_BYTE;
			return;
		case TextureFormat::R8_SNORM:
			glInternalFormat = GL_R8_SNORM;
			glFormat = GL_RED;
			glType = GL_BYTE;
			return;
		case TextureFormat::R8_SINT:
			glInternalFormat = GL_R8I;
			glFormat = GL_RED_INTEGER;
			glType = GL_BYTE;
			return;
#if !defined(__EMSCRIPTEN__)
		case TextureFormat::A8_UNORM:
			glInternalFormat = GL_ALPHA8;
			glFormat = GL_ALPHA;
			glType = GL_UNSIGNED_BYTE;
			return;
		case TextureFormat::BC1_UNORM:
			glInternalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
			glFormat = GL_NONE;
			glType = GL_NONE;
			return;
		case TextureFormat::BC2_UNORM:
			glInternalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
			glFormat = GL_NONE;
			glType = GL_NONE;
			return;
		case TextureFormat::BC3_UNORM:
			glInternalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			glFormat = GL_NONE;
			glType = GL_NONE;
			return;
#endif

		case TextureFormat::D24:
			glInternalFormat = GL_DEPTH_COMPONENT24;
			glFormat = GL_DEPTH_COMPONENT;
			glType = GL_UNSIGNED_INT;
			return;

		case TextureFormat::D24_UNORM_S8_UINT:
			glInternalFormat = GL_DEPTH24_STENCIL8;
			glFormat = GL_DEPTH_STENCIL;
			glType = GL_UNSIGNED_INT_24_8;
			return;
		case TextureFormat::D32_FLOAT:
			glInternalFormat = GL_DEPTH32F_STENCIL8;
			glFormat = GL_DEPTH_STENCIL;
			glType = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
			return;
	}

	// Unimplemented texture format.
	sgeAssert(false);
}

//------------------------------------------------------------------------------
// TextureDesc
//------------------------------------------------------------------------------
GLenum TextureDesc_GetGLNativeTextureTartget(const TextureDesc& desc) {
	if (desc.textureType == UniformType::Texture1D) {
#if !defined(__EMSCRIPTEN__)
		return desc.texture1D.arraySize == 1 ? GL_TEXTURE_1D : GL_TEXTURE_1D_ARRAY;
#endif
	} else if (desc.textureType == UniformType::Texture2D) {
		if (desc.texture2D.numSamples == 1) {
			return desc.texture2D.arraySize == 1 ? GL_TEXTURE_2D : GL_TEXTURE_2D_ARRAY;
		} else {
#if !defined(__EMSCRIPTEN__)
			return desc.texture2D.arraySize == 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
#endif
		}
	} else if (desc.textureType == UniformType::Texture3D) {
		return GL_TEXTURE_3D;
	} else if (desc.textureType == UniformType::TextureCube) {
#if !defined(__EMSCRIPTEN__)
		return (desc.textureCube.arraySize == 1) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_CUBE_MAP_ARRAY;
#else
		if (desc.textureCube.arraySize == 1) {
			return GL_TEXTURE_CUBE_MAP;
		}
#endif
	}

	// Should never happen.
	sgeAssert(false);
	return SGE_GL_UNKNOWN;
}

//------------------------------------------------------------------------------
// UniformType
//------------------------------------------------------------------------------

// [TODO] http://www.opengl.org/sdk/docs/man/html/glGetActiveUniform.xhtml
UniformType::Enum UniformType_FromGLNumericUniformType(const GLenum uniformType) {
	switch (uniformType) {
		case GL_FLOAT:
			return UniformType::Float;
		case GL_FLOAT_VEC2:
			return UniformType::Float2;
		case GL_FLOAT_VEC3:
			return UniformType::Float3;
		case GL_FLOAT_VEC4:
			return UniformType::Float4;

#if !defined(__EMSCRIPTEN__)
		case GL_DOUBLE:
			return UniformType::Double;
		case GL_DOUBLE_VEC2:
			return UniformType::Double2;
		case GL_DOUBLE_VEC3:
			return UniformType::Double3;
		case GL_DOUBLE_VEC4:
			return UniformType::Double4;
#endif

		case GL_INT:
			return UniformType::Int;
		case GL_INT_VEC2:
			return UniformType::Int2;
		case GL_INT_VEC3:
			return UniformType::Int3;
		case GL_INT_VEC4:
			return UniformType::Int4;

		case GL_UNSIGNED_INT:
			return UniformType::Uint;
		case GL_UNSIGNED_INT_VEC2:
			return UniformType::Uint2;
		case GL_UNSIGNED_INT_VEC3:
			return UniformType::Uint3;
		case GL_UNSIGNED_INT_VEC4:
			return UniformType::Uint4;

		case GL_FLOAT_MAT3:
			return UniformType::Float3x3;
		case GL_FLOAT_MAT4:
			return UniformType::Float4x4;
	}

	// sgeAssert(false);
	return UniformType::Unknown;
}

// Resturns a suitable data for glVertexAttribPointer.
// Example: for arg (Float3), the resuilt is (GL_FLOAT, 3, GL_FALSE).
void UniformType_ToGLUniformType(const UniformType::Enum type, GLenum& glType, GLint& elemCnt, GLboolean& normalized) {
	// Set this here to save space as it is usually false
	normalized = GL_FALSE;

	switch (type) {
		case UniformType::Unknown:
			glType = SGE_GL_UNKNOWN;
			elemCnt = 0;
			return;

		case UniformType::Float:
			glType = GL_FLOAT;
			elemCnt = 1;
			return;
		case UniformType::Float2:
			glType = GL_FLOAT;
			elemCnt = 2;
			return;
		case UniformType::Float3:
			glType = GL_FLOAT;
			elemCnt = 3;
			return;
		case UniformType::Float4:
			glType = GL_FLOAT;
			elemCnt = 4;
			return;

#if !defined(__EMSCRIPTEN__)
		case UniformType::Double:
			glType = GL_DOUBLE;
			elemCnt = 1;
			return;
		case UniformType::Double2:
			glType = GL_DOUBLE;
			elemCnt = 2;
			return;
		case UniformType::Double3:
			glType = GL_DOUBLE;
			elemCnt = 3;
			return;
		case UniformType::Double4:
			glType = GL_DOUBLE;
			elemCnt = 4;
			return;
#endif

		case UniformType::Int:
			glType = GL_INT;
			elemCnt = 1;
			return;
		case UniformType::Int2:
			glType = GL_INT;
			elemCnt = 2;
			return;
		case UniformType::Int3:
			glType = GL_INT;
			elemCnt = 3;
			return;
		case UniformType::Int4:
			glType = GL_INT;
			elemCnt = 4;
			return;

		case UniformType::Uint:
			glType = GL_UNSIGNED_INT;
			elemCnt = 1;
			return;
		case UniformType::Uint2:
			glType = GL_UNSIGNED_INT;
			elemCnt = 2;
			return;
		case UniformType::Uint3:
			glType = GL_UNSIGNED_INT;
			elemCnt = 3;
			return;
		case UniformType::Uint4:
			glType = GL_UNSIGNED_INT;
			elemCnt = 4;
			return;

		case UniformType::Uint16:
			glType = GL_UNSIGNED_SHORT;
			elemCnt = 1;
			return;

		case UniformType::Float3x3:
			glType = GL_FLOAT;
			elemCnt = 9;
			return;
		case UniformType::Float4x4:
			glType = GL_FLOAT;
			elemCnt = 16;
			return;

		case UniformType::Int_RGBA_Unorm_IA:
			glType = GL_UNSIGNED_BYTE;
			elemCnt = 4;
			normalized = GL_TRUE;
			return;
	}

	sgeAssert(false);
	glType = SGE_GL_UNKNOWN;
	elemCnt = 0;
	return;
}

//------------------------------------------------------------------------------
// ShaderType
//------------------------------------------------------------------------------
GLenum ShaderType_GetGLNative(const ShaderType::Enum type) {
	switch (type) {
		case ShaderType::VertexShader:
			return GL_VERTEX_SHADER;
		case ShaderType::PixelShader:
			return GL_FRAGMENT_SHADER;
//#ifdef GL_COMPUTE_SHADER
//		case ShaderType::ComputeShader:
//			return GL_COMPUTE_SHADER;
//#endif
	}

	sgeAssert(false);
	return SGE_GL_UNKNOWN;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
#if !defined(__EMSCRIPTEN__)
GLenum FillMode_GetGLNative(const FillMode::Enum& fillMode) {
	if (fillMode == FillMode::Solid)
		return GL_FILL;
	return GL_LINE;
}
#endif

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
GLenum CullMode_GetGLNative(const CullMode::Enum& cullMode) {
	switch (cullMode) {
		case CullMode::Back:
			return GL_BACK;
		case CullMode::Front:
			return GL_FRONT;
		case CullMode::None:
			return GL_NONE;
	}

	sgeAssert(false);
	return GL_NONE;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
GLenum DepthComparisonFunc_GetGLNative(const DepthComparisonFunc::Enum& comaprisonFunc) {
	switch (comaprisonFunc) {
		case DepthComparisonFunc::Never:
			return GL_NEVER;
		case DepthComparisonFunc::Always:
			return GL_ALWAYS;
		case DepthComparisonFunc::Less:
			return GL_LESS;
		case DepthComparisonFunc::LessEqual:
			return GL_LEQUAL;
		case DepthComparisonFunc::Greater:
			return GL_GREATER;
		case DepthComparisonFunc::GreaterEqual:
			return GL_GEQUAL;
		case DepthComparisonFunc::Equal:
			return GL_EQUAL;
		case DepthComparisonFunc::NotEqual:
			return GL_NOTEQUAL;
	}

	sgeAssert(false);
	return GL_ALWAYS;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
GLenum PrimitiveTopology_GetGLNative(const PrimitiveTopology::Enum& topology) {
	switch (topology) {
		case PrimitiveTopology::Unknown:
			return GL_NONE;
		case PrimitiveTopology::TriangleList:
			return GL_TRIANGLES;
		case PrimitiveTopology::TriangleStrip:
			return GL_TRIANGLE_STRIP;
		case PrimitiveTopology::LineList:
			return GL_LINES;
		case PrimitiveTopology::LineStrip:
			return GL_LINE_STRIP;
		case PrimitiveTopology::PointList:
			return GL_POINTS;
	}

	// Unknown primitive toplogy.
	sgeAssert(false);
	return GL_NONE;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
GLenum Blend_GetGLNative(Blend::Enum blend) {
	if (blend == Blend::Zero)
		return GL_ZERO;
	else if (blend == Blend::One)
		return GL_ONE;

	else if (blend == Blend::SrcColor)
		return GL_SRC_COLOR;
	else if (blend == Blend::InvSrcColor)
		return GL_ONE_MINUS_SRC_COLOR;

	else if (blend == Blend::DestColor)
		return GL_DST_COLOR;
	else if (blend == Blend::InvDestColor)
		return GL_ONE_MINUS_DST_COLOR;

	// Alpha only.
	else if (blend == Blend::Alpha_Src)
		return GL_SRC_ALPHA;
	else if (blend == Blend::Alpha_InvSrc)
		return GL_ONE_MINUS_SRC_ALPHA;
	else if (blend == Blend::Alpha_Dest)
		return GL_DST_ALPHA;
	else if (blend == Blend::Alpha_InvDest)
		return GL_ONE_MINUS_DST_ALPHA;

	// Unknown BlendType
	sgeAssert(false);
	return GL_ONE;
}

GLenum BlendOp_GetGLNative(BlendOp::Enum blendOp) {
	if (blendOp == BlendOp::Add)
		return GL_FUNC_ADD;
	if (blendOp == BlendOp::Sub)
		return GL_FUNC_SUBTRACT;
	if (blendOp == BlendOp::RevSub)
		return GL_FUNC_REVERSE_SUBTRACT;
	if (blendOp == BlendOp::Min)
		return GL_MIN;
	if (blendOp == BlendOp::Max)
		return GL_MAX;

	// Unknown BlendOp.
	sgeAssert(false);
	return GL_FUNC_ADD;
}

GLenum QueryType_GetGLnative(QueryType::Enum const queryType) {
	switch (queryType) {
		case QueryType::NumSamplesPassedDepthStencilTest:
#if !defined(__EMSCRIPTEN__)
			return GL_SAMPLES_PASSED;
#else
			break;
#endif
		case QueryType::AnySamplePassedDepthStencilTest:
			return GL_ANY_SAMPLES_PASSED;
	}

	// Unknown query type.
	sgeAssert(false);
	return SGE_GL_UNKNOWN;
}

GLenum signedAxis_toTexCubeFaceIdx_OpenGL(const SignedAxis signedAxis) {
	switch (signedAxis) {
		case axis_x_pos:
			return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
		case axis_x_neg:
			return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
		case axis_y_pos:
			return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
		case axis_y_neg:
			return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
		case axis_z_pos:
			return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
		case axis_z_neg:
			return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
	}

	sgeAssert(false);
	return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
}

} // namespace sge
