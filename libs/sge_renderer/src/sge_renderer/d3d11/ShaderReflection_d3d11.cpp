#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "GraphicsCommon_d3d11.h"
#include "Shader_d3d11.h"
#include "ShadingProgram_d3d11.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/utils/strings.h"
#include <d3dcompiler.h>

#include "sge_renderer/renderer/ShaderReflection.h"

namespace sge {

//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
BindLocation NumericUniformRefl::computeBindLocation() const {
	return BindLocation(d3d11_shaderType, short(byteOffset_d3d11), short(uniformType), (short)sizeBytes_d3d11);
}

BindLocation CBufferRefl::computeBindLocation() const {
	return BindLocation(d3d11_shaderType, short(d3d11_bindingSlot), short(UniformType::ConstantBuffer));
}

BindLocation TextureRefl::computeBindLocation() const {
	return BindLocation(d3d11_shaderType, short(d3d11_bindingSlot), short(textureType), short(arraySize));
}


BindLocation SamplerRefl::computeBindLocation() const {
	return BindLocation(d3d11_shaderType, short(d3d11_bindingSlot), short(UniformType::SamplerState), short(arraySize));
}

//-------------------------------------------------------------------------------
// ShaderRefl
//-------------------------------------------------------------------------------
bool ShadingProgramRefl::create(ShadingProgram* const shadingProgram) {
	ShaderD3D11* shaders[] = {
	    (ShaderD3D11*)shadingProgram->getVertexShader(),
	    (ShaderD3D11*)shadingProgram->getPixelShader(),
	};

	SGEDevice* const device = shadingProgram->getDevice();

	for (int iShader = 0; iShader < SGE_ARRSZ(shaders); ++iShader) {
		ShaderD3D11* const shader = shaders[iShader];
		if (shader == nullptr) {
			continue;
		}

		TComPtr<ID3D11ShaderReflection> pD3DReflection;
		const HRESULT reflResult = D3DReflect(shader->D3D11_GetByteCode()->GetBufferPointer(), shader->D3D11_GetByteCode()->GetBufferSize(),
		                                      IID_ID3D11ShaderReflection, (void**)&pD3DReflection);

		D3D11_SHADER_DESC d3dShaderDesc;
		pD3DReflection->GetDesc(&d3dShaderDesc);

		for (UINT iBoundRes = 0; iBoundRes < d3dShaderDesc.BoundResources; ++iBoundRes) {
			D3D11_SHADER_INPUT_BIND_DESC d3dResourceDesc;
			pD3DReflection->GetResourceBindingDesc(iBoundRes, &d3dResourceDesc);

			if (d3dResourceDesc.Type == D3D_SIT_CBUFFER) {
				// This is a constant buffer.
				const char* bufferName = d3dResourceDesc.Name;
				ID3D11ShaderReflectionConstantBuffer* d3dCbRefl = (pD3DReflection->GetConstantBufferByName(bufferName));

				D3D11_SHADER_BUFFER_DESC cbufferDesc;
				d3dCbRefl->GetDesc(&cbufferDesc);

				bool const isGlobal = strncmp("$Globals", bufferName, 8) == 0;

				CBufferRefl cbRefl;

				cbRefl.name = bufferName;
				cbRefl.nameStrIdx = (unsigned)device->getStringIndex(cbRefl.name);
				cbRefl.d3d11_shaderType = shader->getShaderType();
				cbRefl.d3d11_bindingSlot = d3dResourceDesc.BindPoint;
				cbRefl.sizeBytes = cbufferDesc.Size;

				cbRefl.variables.resize(cbufferDesc.Variables);
				for (UINT t = 0; t < cbufferDesc.Variables; ++t) {
					// obtain d3d11 native cbuffer variables reflection
					ID3D11ShaderReflectionVariable* d3dVarRefl = (d3dCbRefl->GetVariableByIndex(t));
					ID3D11ShaderReflectionType* d3dVarType = d3dVarRefl->GetType();

					D3D11_SHADER_VARIABLE_DESC d3dVarDesc;
					d3dVarRefl->GetDesc(&d3dVarDesc);

					D3D11_SHADER_TYPE_DESC d3dTypeDesc;
					d3dVarType->GetDesc(&d3dTypeDesc);

					CBufferVariableRefl& var = cbRefl.variables[t];
					var.name = d3dVarDesc.Name;
					var.nameStrIdx = device->getStringIndex(var.name);
					var.arraySize = d3dTypeDesc.Elements;
					var.offset = d3dVarDesc.StartOffset;

					// Convert the matrix row/col majory to registers and lanes used in those registers
					const bool isColumnMajor = d3dTypeDesc.Class == D3D_SVC_MATRIX_COLUMNS;
					int registers = (isColumnMajor) ? d3dTypeDesc.Columns : d3dTypeDesc.Rows;
					int lanes = (isColumnMajor) ? d3dTypeDesc.Rows : d3dTypeDesc.Columns; // the arity

					UniformType::Enum typeFormat;

					switch (d3dTypeDesc.Type) {
						case D3D_SVT_INT:
							typeFormat = UniformType::Int;
							break;
						case D3D_SVT_FLOAT:
							typeFormat = UniformType::Float;
							break;

						default:
							sgeAssert(false);
							typeFormat = UniformType::Unknown;
							break;
					};

					var.type = UniformType::PickType(typeFormat, lanes, registers);

					// If this is the global CBuffer add this as a numeric uniform.
					if (isGlobal) {
						NumericUniformRefl numericRefl;

						numericRefl.nameStrIdx = var.nameStrIdx;
						numericRefl.name = var.name;
						numericRefl.uniformType = var.type;

						numericRefl.d3d11_shaderType = shader->getShaderType();
						numericRefl.byteOffset_d3d11 = var.offset;
						numericRefl.sizeBytes_d3d11 = d3dVarDesc.Size; // UniformType::GetSizeBytes(var.type) * std::max(1, var.arraySize);

						numericUnforms.add(numericRefl);
					}
				}

				cbuffers.add(cbRefl);
			} else if (d3dResourceDesc.Type == D3D_SIT_TEXTURE) {
				// This is a texture.
				TextureRefl textureRefl;
				textureRefl.name = d3dResourceDesc.Name;
				textureRefl.nameStrIdx = device->getStringIndex(textureRefl.name);
				textureRefl.d3d11_shaderType = shader->getShaderType();
				textureRefl.d3d11_bindingSlot = d3dResourceDesc.BindPoint;
				textureRefl.arraySize = 1;

				const size_t openBraceLocation = textureRefl.name.find('[');
				const bool isArrayElement = openBraceLocation != std::string::npos;

				// Caution:
				// Direct3D 11 reports each array element as a separate uniform instead of 1 variable with some array size.
				// In order to have them repored as one uniform in our API we need to unify them together as an array.
				// If a texture with the same name has already been reported just increase it's array size.
				// All array elements are going to be consequive
				bool addedToExisting = false;
				if (isArrayElement) {
					// Drop the element index from the name of the variable.
					textureRefl.name = textureRefl.name.substr(0, openBraceLocation);

					for (auto& existingTexPair : textures.m_uniforms) {
						if (existingTexPair.second.name == textureRefl.name) {
							existingTexPair.second.arraySize++;
							existingTexPair.first = existingTexPair.second.computeBindLocation();
							addedToExisting = true;
							break;
						}
					}
				}

				if (addedToExisting == false) {
					switch (d3dResourceDesc.Dimension) {
						case D3D_SRV_DIMENSION_TEXTURE1D:
							textureRefl.textureType = UniformType::Texture1D;
							break;
						case D3D_SRV_DIMENSION_TEXTURE2D:
							textureRefl.textureType = UniformType::Texture2D;
							break;
						case D3D_SRV_DIMENSION_TEXTURECUBE:
							textureRefl.textureType = UniformType::TextureCube;
							break;
						case D3D_SRV_DIMENSION_TEXTURE3D:
							textureRefl.textureType = UniformType::Texture3D;
							break;

						default: {
							sgeAssert(false);
						}
					}

					textures.add(textureRefl);
				}
			}

			// Samplers.
			if (d3dResourceDesc.Type == D3D_SIT_SAMPLER) {
				SamplerRefl samplerRefl;
				samplerRefl.name = d3dResourceDesc.Name;
				samplerRefl.nameStrIdx = device->getStringIndex(samplerRefl.name);
				samplerRefl.d3d11_shaderType = shader->getShaderType();
				samplerRefl.d3d11_bindingSlot = d3dResourceDesc.BindPoint;
				samplerRefl.arraySize = d3dResourceDesc.BindCount == 1 ? 0 : d3dResourceDesc.BindCount;

				samplers.add(samplerRefl);
			}
		}

		// Vertex shaders input signature.
		if (D3D11_SHVER_GET_TYPE(d3dShaderDesc.Version) == D3D11_SHVER_VERTEX_SHADER) {
			// HLSL reflector gives us multi register values in multiple InputParameters for example
			// float2x3 will result in 2 register values with arity of 3
			// HLSL reflection will give us 2 InputParameters that share the same name(aka. refer to the same variable)
			VertShaderAttrib accum;
			int accumPass = 0;
			char accumSemantic[32] = {
			    '\0',
			};

			for (UINT i = 0; i < d3dShaderDesc.InputParameters; i++) {
				D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
				pD3DReflection->GetInputParameterDesc(i, &paramDesc);

				// If we've just started accumulationg a new variable, but we had another one from the previous iteration
				// store it(if valid) and start accumulating the next one.
				if (strcmp(paramDesc.SemanticName, accumSemantic) != 0) {
					sge_strcpy(accumSemantic, paramDesc.SemanticName);

					// check if there is unaculmulated vertex attribute
					if (!accum.name.empty()) {
						inputVertices.push_back(accum);
					}

					// we are starting to accumulate new variable
					accumPass = 0;
					char fullSemantic[32]; // combined semantic and semantic index
					sge_snprintf(fullSemantic, SGE_ARRSZ(fullSemantic), "%s%d", paramDesc.SemanticName, paramDesc.SemanticIndex);
					accum.name = fullSemantic;
				}

				// Todo: Doesn't work for multiple register variables(like matrices).
				// Deduce the data arity.
				int arity = 0;
				if (paramDesc.Mask & 1)
					arity++;
				if (paramDesc.Mask & 2)
					arity++;
				if (paramDesc.Mask & 4)
					arity++;
				if (paramDesc.Mask & 8)
					arity++;

				// Deduce the data type
				switch (paramDesc.ComponentType) {
					case D3D_REGISTER_COMPONENT_FLOAT32:
						accum.type = (UniformType::Enum)((int)UniformType::Float + (arity - 1));
						break;
					case D3D_REGISTER_COMPONENT_SINT32:
						accum.type = (UniformType::Enum)((int)UniformType::Int + (arity - 1));
						break;
					case D3D_REGISTER_COMPONENT_UINT32:
						accum.type = (UniformType::Enum)((int)UniformType::Uint + (arity - 1));
						break;
					default:
						accum.type = UniformType::Unknown;
				}

				// advance the acculumation pass
				accumPass++;
			}

			// Check if there is unaculmulated vertex attribute.
			if (!accum.name.empty()) {
				inputVertices.push_back(accum);
			}
		}
	}

	return true;
}

} // namespace sge
