#include <algorithm>

#include "GraphicsInterface_d3d11.h"
#include "Shader_d3d11.h"
#include "sge_renderer/renderer/HLSLTranslator.h"
#include <d3dcompiler.h>

namespace sge {

//-----------------------------------------------------------------------
// ShaderD3D11
//-----------------------------------------------------------------------
CreateShaderResult ShaderD3D11::createNative(const ShaderType::Enum type, const char* pCode, const char* const entryPoint) {
	destroy();

	ID3D11Device* const d3ddev = getDevice<SGEDeviceD3D11>()->D3D11_GetDevice();

	m_cachedCode = pCode;
	m_shaderType = type;

	// Just an interestiong flag to be remembered -> D3DCOMPILE_PACK_MATRIX_ROW_MAJOR
	const DWORD compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
	// const DWORD compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_OPTIMIZATION_LEVEL0; // For exrternal shader debugging in RenderDoc or PIX
	// and so on.

	// Compile the shader.
	TComPtr<ID3D10Blob> compilationErrorBlob;

	const HRESULT compilatonResult = D3DCompile(pCode, strlen(pCode), NULL, NULL, NULL, entryPoint,
	                                            getDevice<SGEDeviceD3D11>()->D3D11_GetWorkingShaderModel(type).c_str(), // shader model
	                                            compileFlags,                                                           // flags1
	                                            0,                                                                      // flags2
	                                            &m_compiledBlob, &compilationErrorBlob);

	if (FAILED(compilatonResult)) {
		[[maybe_unused]] const char* const errors = (char*)compilationErrorBlob->GetBufferPointer();
		if (compilationErrorBlob != nullptr) {
			sgeAssert(false);
		}

		destroy();
		return CreateShaderResult(false, errors);
	}

	HRESULT createShaderResult = E_FAIL;

	if (type == ShaderType::VertexShader) {
		createShaderResult = d3ddev->CreateVertexShader(m_compiledBlob->GetBufferPointer(), m_compiledBlob->GetBufferSize(), NULL,
		                                                (ID3D11VertexShader**)&m_dx11Shader);
	} else if (type == ShaderType::PixelShader) {
		createShaderResult = d3ddev->CreatePixelShader(m_compiledBlob->GetBufferPointer(), m_compiledBlob->GetBufferSize(), NULL,
		                                               (ID3D11PixelShader**)&m_dx11Shader);
	} else {
		// Unknown shader type.
		sgeAssert(false);
	}

	if (FAILED(createShaderResult)) {
		return CreateShaderResult(false, "ID3D11Device::Create*Shader failed!");
	}

	// Finally obtain the shader reflation...
	TComPtr<ID3D11ShaderReflection> d3dRefl;
	const HRESULT reflResult =
	    D3DReflect(m_compiledBlob->GetBufferPointer(), m_compiledBlob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&d3dRefl);

	if (FAILED(reflResult) || (d3dRefl == NULL)) {
		destroy();
		return CreateShaderResult(false, "D3DReflect failed!");
	}

	return CreateShaderResult(true, "");
}

bool ShaderD3D11::isValid() const {
	return m_dx11Shader != nullptr;
}

ID3D11InputLayout* ShaderD3D11::D3D11_GetInputLayoutForVertexDeclIndex(const VertexDeclIndex vertexDeclIdx) {
	sgeAssert(m_shaderType == ShaderType::VertexShader);

	ID3D11InputLayout* const foundLayout = m_inputLayouts[vertexDeclIdx];

	if (foundLayout != NULL) {
		return foundLayout;
	}

	TComPtr<ID3D11InputLayout>& inputLayout = m_inputLayouts[vertexDeclIdx];

	// The layout hasn't been found we have to create a brand new one.
	const std::vector<VertexDecl>& vertexDecl = getDevice()->getVertexDeclFromIndex(vertexDeclIdx);

	if (vertexDecl.size() == 0) {
		// Should never happen.
		sgeAssert(false);
		return nullptr;
	}

	// Normalize (compute the byte offset for declarations where its value is -1) the vertex declarations.
	std::vector<D3D11_INPUT_ELEMENT_DESC> d3d11InputDesc(vertexDecl.size());

	// Create the D3D11_INPUT_ELEMENT_DESC array.
	for (unsigned t = 0; t < vertexDecl.size(); ++t) {
		D3D11_INPUT_ELEMENT_DESC& currentDesc = d3d11InputDesc[t];

		currentDesc.SemanticName = vertexDecl[t].semantic.c_str();
		currentDesc.SemanticIndex = 0;
		currentDesc.Format = UniformType_GetDX_DXGI_FORMAT(vertexDecl[t].format);
		currentDesc.InputSlot = vertexDecl[t].bufferSlot;
		currentDesc.AlignedByteOffset = (UINT)vertexDecl[t].byteOffset;
		currentDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		currentDesc.InstanceDataStepRate = 0;
	}

	// Create the InputLayout object.
	const HRESULT hr = getDevice<SGEDeviceD3D11>()->D3D11_GetDevice()->CreateInputLayout(
	    d3d11InputDesc.data(),       // D3D11_INPUT_ELEMENT_DESC[]
	    (UINT)d3d11InputDesc.size(), // Num D3D11_INPUT_ELEMENT_DESC
	    m_compiledBlob->GetBufferPointer(), m_compiledBlob->GetBufferSize(), &inputLayout);

	sgeAssert(SUCCEEDED(hr));

	return inputLayout.p;
}

void ShaderD3D11::destroy() {
	m_dx11Shader.Release();
	m_compiledBlob.Release();
	m_cachedCode = std::string();
	m_inputLayouts.clear();
}

} // namespace sge
