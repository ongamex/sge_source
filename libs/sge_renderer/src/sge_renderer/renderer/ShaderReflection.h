#pragma once

#include "sge_utils/sge_utils.h"
#include "sge_utils/utils/vector_map.h"
#include <unordered_map>

#include "GraphicsCommon.h"

namespace sge {

struct ShadingProgram;


//---------------------------------------------------
//
//---------------------------------------------------
struct BindLocation {
	BindLocation()
	    : raw(0) {
		static_assert(sizeof(BindLocation) == sizeof(uint64), "");
	}

	bool isNull() const { return raw == 0; }

#ifdef SGE_RENDERER_GL
	BindLocation(short const bindLocation, short const uniformType, short const arraySize, short bindUnitTexture) {
		raw = 0;
		this->bindLocation = bindLocation;
		this->uniformType = uniformType;
		this->glArraySize = arraySize;
		this->glTextureUnit = bindUnitTexture;
	}
#endif

	BindLocation(const BindLocation& ref)
	    : raw(ref.raw) {}

#ifdef SGE_RENDERER_D3D11
	BindLocation(ShaderType::Enum shaderFreq,
	             short bindLocation,
	             short const uniformType,
	             short const texArraySize_or_numericUniformSizeBytes = 0) {
		raw = 0;
		this->shaderFreq = short(shaderFreq);
		this->bindLocation = bindLocation;
		this->uniformType = uniformType;
		this->texArraySize_or_numericUniformSizeBytes = texArraySize_or_numericUniformSizeBytes;
	}
#endif

	union {
		uint64 raw;
		struct {
			short bindLocation; // OpenGL BindLocation, D3D11 slot. If this is a Numeric Uniform under D3D11, then this is a byteOffset of
			                    // the variable in the buffer!
			short uniformType;  // Element of UniformType::enum describing what tyoe of uniform is this.
#ifdef SGE_RENDERER_D3D11
			short shaderFreq; // Is tha a vertex, geometry, pixel or so on shader.
			short texArraySize_or_numericUniformSizeBytes;
#else
			short glTextureUnit;
			short glArraySize;
#endif
		};
	};

	bool operator<(const BindLocation r) const { return raw < r.raw; }
	bool operator>(const BindLocation r) const { return raw > r.raw; }
	bool operator==(const BindLocation r) const { return raw == r.raw; }
	bool operator!=(const BindLocation r) const { return raw != r.raw; }
};

} // namespace sge

template <>
struct std::hash<sge::BindLocation> {
	std::size_t operator()(const sge::BindLocation& k) const {
		std::hash<decltype(k.raw)> h;
		return h(k.raw);
	}
};

namespace sge {

//---------------------------------------------------
//
//---------------------------------------------------
struct NumericUniformRefl {
	BindLocation computeBindLocation() const;

  public:
	unsigned nameStrIdx = 0;
	std::string name;
	UniformType::Enum uniformType;
	int arraySize = 0;

#ifdef SGE_RENDERER_D3D11
	// In D3D11 there aren't numeric uniforms.
	// However the $(Global) cbuffer are used are such.
	// So numeric uniforms in that case are just variables from a cbuffer.
	ShaderType::Enum d3d11_shaderType = ShaderType::VertexShader;
	int byteOffset_d3d11 = 0;
	int sizeBytes_d3d11 = 0;
#elif defined(SGE_RENDERER_GL)
	int bindLocation = 0;
#endif
};

//---------------------------------------------------
//
//---------------------------------------------------
struct CBufferVariableRefl {
	unsigned nameStrIdx = 0;
	std::string name;
	UniformType::Enum type; // Todo: what about compound types>
	int offset = 0;         // byte offset in the buffer.
	int arraySize = 0;      // Zero if not applicable.
};

struct CBufferRefl {
	BindLocation computeBindLocation() const;

  public:
	unsigned nameStrIdx = 0;
	std::string name;

#ifdef SGE_RENDERER_D3D11
	ShaderType::Enum d3d11_shaderType = ShaderType::VertexShader;
	int d3d11_bindingSlot;
#elif defined(SGE_RENDERER_GL)
	int gl_bindLocation;
#endif

	int sizeBytes = 0; // this is != sum(variables.size) because of alignment requerments
	std::vector<CBufferVariableRefl> variables;
};

//---------------------------------------------------
//
//---------------------------------------------------
struct TextureRefl {
	BindLocation computeBindLocation() const;

  public:
	unsigned nameStrIdx = 0;
	std::string name;

#ifdef SGE_RENDERER_D3D11
	ShaderType::Enum d3d11_shaderType = ShaderType::VertexShader;
	int d3d11_bindingSlot = -1;
#elif defined(SGE_RENDERER_GL)
	int gl_bindLocation = -1;
	int gl_bindUnit = 0;               // should be used this way: "GL_TEXTURE0 + gl_bindUnit"
	unsigned int gl_textureTarget = 0; // Reqired target of the texture, see GLUniformTypeToTextureType for more details.
#endif
	int arraySize = 0; // This value must be > 0 in order for the reflection to be valid.
	UniformType::Enum textureType = UniformType::Unknown;
};


//---------------------------------------------------
//
//---------------------------------------------------
struct SamplerRefl {
	BindLocation computeBindLocation() const;

  public:
	unsigned nameStrIdx = 0; // A runtime mapping string->int for the "name" string.
	std::string name;
	int arraySize = 0; // 0 means not an array.

#ifdef SGE_RENDERER_D3D11
	ShaderType::Enum d3d11_shaderType = ShaderType::VertexShader;
	int d3d11_bindingSlot = -1;
#elif defined(SGE_RENDERER_GL)
	int gl_bindLocation = 0;
#endif
};

//---------------------------------------------------
//
//---------------------------------------------------
struct VertShaderAttrib {
	unsigned nameStrIdx = 0;
	std::string name; // Semantic+index for D3D11, attribute name for OpenGL.
	UniformType::Enum type = UniformType::Unknown;
#ifdef SGE_RENDERER_GL
	int attributeLocation = 0; // The binding location of the attribute.
#endif
};

template <typename T>
struct UniformContainer {
	typedef std::pair<BindLocation, T> UniformPair;
	std::vector<UniformPair> m_uniforms;

	typedef std::pair<unsigned, BindLocation> UniformLUTPair;
	std::vector<UniformLUTPair> m_nameStrIdxLUT;

	void add(const T& uniform) {
		BindLocation loc = uniform.computeBindLocation();
		m_uniforms.emplace_back(UniformPair(loc, uniform));
		m_nameStrIdxLUT.emplace_back(UniformLUTPair(uniform.nameStrIdx, loc));
	}

	BindLocation findUniform(const char* const name, [[maybe_unused]] ShaderType::Enum shaderType) const {
		for (const auto& itr : m_uniforms) {
			// @shaderType is ignored on OpenGL as all shader reflection there cannot 
			// tell us which shader stage needs them, they are all uniforms to the shading program.
			// OpenGL manages to handle shader uniforms (those used by multiple shader stages at once)
			// internally.
#ifdef SGE_RENDERER_D3D11
			if (itr.second.d3d11_shaderType == shaderType && itr.second.name == name) {
				return itr.first;
			}
#elif defined(SGE_RENDERER_GL)
			if (itr.second.name == name) {
				return itr.first;
			}
#endif
		}

		return BindLocation();
	}

	BindLocation findUniform(unsigned const nameStrIdx, ShaderType::Enum shaderType) const {
		for (const auto& pair : m_nameStrIdxLUT) {
			if (pair.first == nameStrIdx) {
				return pair.second;
			}
		}

		return BindLocation();
	}
};

//---------------------------------------------------
// ShadingProgramRefl
//---------------------------------------------------
struct ShadingProgramRefl {
	bool create(ShadingProgram* const shadingProgram);
	BindLocation findUniform(const char* const uniformName, [[maybe_unused]] ShaderType::Enum shaderType) const;

  public:
	std::vector<VertShaderAttrib> inputVertices;

	UniformContainer<NumericUniformRefl> numericUnforms;
	UniformContainer<CBufferRefl> cbuffers;
	UniformContainer<TextureRefl> textures;
	UniformContainer<SamplerRefl> samplers;
};

} // namespace sge
