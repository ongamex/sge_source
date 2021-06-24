#include "RenderState_d3d11.h"
#include "GraphicsInterface_d3d11.h"

namespace sge {

//////////////////////////////////////////////////////////////////////////////
// RasterizerStateD3D11
//////////////////////////////////////////////////////////////////////////////
bool RasterizerStateD3D11::create(const RasterDesc& desc) {
	destroy();

	m_rasterStateDesc = desc;

	// Create the D3D11 native rasterizer desc structure.
	D3D11_RASTERIZER_DESC d3d11NativeRasterizerDesc = RasterDesc_GetD3D11Nativce(m_rasterStateDesc);
	ID3D11Device* const d3ddev = getDevice<SGEDeviceD3D11>()->D3D11_GetDevice();

	// attempt the create the object
	HRESULT const hr = d3ddev->CreateRasterizerState(&d3d11NativeRasterizerDesc, &m_dx11state);
	if (FAILED(hr))
		return false;

	return true;
}

void RasterizerStateD3D11::destroy() {
	getDevice<SGEDeviceD3D11>()->D3D11_GetContextStateCache()->SetRasterizerState(NULL);
	m_dx11state.Release();
}

bool RasterizerStateD3D11::isValid() const {
	return m_dx11state != NULL;
}

//////////////////////////////////////////////////////////////////////////////
// DepthStencilStateD3D11
//////////////////////////////////////////////////////////////////////////////
bool DepthStencilStateD3D11::create(const DepthStencilDesc& desc) {
	destroy();

	m_bufferedDesc = desc;

	// create the dx11 native depth stencil desc structure
	D3D11_DEPTH_STENCIL_DESC d3d11NativeRasterizerDesc = DepthStencilDesc_D3D11_Native(m_bufferedDesc);
	ID3D11Device* d3ddev = getDevice<SGEDeviceD3D11>()->D3D11_GetDevice();

	// attempt the create the object
	HRESULT const hr = d3ddev->CreateDepthStencilState(&d3d11NativeRasterizerDesc, &m_dx11state);
	if (FAILED(hr))
		return false;

	return true;
}

void DepthStencilStateD3D11::destroy() {
	getDevice<SGEDeviceD3D11>()->D3D11_GetContextStateCache()->SetDepthStencilState(NULL);
	m_dx11state.Release();
}

bool DepthStencilStateD3D11::isValid() const {
	return m_dx11state != NULL;
}

//////////////////////////////////////////////////////////////////////////////
// BlendStateD3D11
//////////////////////////////////////////////////////////////////////////////
bool BlendStateD3D11::create(const BlendStateDesc& desc) {
	destroy();

	m_bufferedDesc = desc;

	D3D11_BLEND_DESC const d3d11NativeBlendDesc = BlendStateDesc_D3D11_Native(m_bufferedDesc);

	ID3D11Device* const d3ddev = getDevice<SGEDeviceD3D11>()->D3D11_GetDevice();
	HRESULT const hr = d3ddev->CreateBlendState(&d3d11NativeBlendDesc, &m_dx11state);


	if (FAILED(hr)) {
		return false;
	}

	return true;
}

void BlendStateD3D11::destroy() {
	// [TODO] It may be better to first check if the reference count of the
	// ComPtr is 0 and then unbind the resource from the context state cache.
	// This is actually applicable to all resuorces.
	getDevice<SGEDeviceD3D11>()->D3D11_GetContextStateCache()->SetBlendState(NULL);
	m_dx11state.Release();
}

bool BlendStateD3D11::isValid() const {
	return m_dx11state != NULL;
}

} // namespace sge
