#pragma once

#include "include_d3d11.h"
#include "sge_renderer/renderer/renderer.h"

namespace sge {

//-----------------------------------------------------------------
// RasterizerStateD3D11
//-----------------------------------------------------------------
struct RasterizerStateD3D11 : public RasterizerState {
	RasterizerStateD3D11() {}
	~RasterizerStateD3D11() { destroy(); }

	bool create(const RasterDesc& desc) final;

	virtual void destroy() final;
	virtual bool isValid() const final;

	const RasterDesc& getDesc() const final { return m_rasterStateDesc; }

	ID3D11RasterizerState* D3D11_GetResource() { return m_dx11state; }

  private:
	RasterDesc m_rasterStateDesc;
	TComPtr<ID3D11RasterizerState> m_dx11state;
};

//-----------------------------------------------------------------
// DepthStencilStateD3D11
//-----------------------------------------------------------------
struct DepthStencilStateD3D11 : public DepthStencilState {
	DepthStencilStateD3D11() {}
	~DepthStencilStateD3D11() { destroy(); }

	bool create(const DepthStencilDesc& desc) final;

	virtual void destroy() final;
	virtual bool isValid() const final;

	const DepthStencilDesc& getDesc() const final { return m_bufferedDesc; }
	ID3D11DepthStencilState* D3D11_GetResource() { return m_dx11state; }

  private:
	DepthStencilDesc m_bufferedDesc;
	TComPtr<ID3D11DepthStencilState> m_dx11state;
};

//-----------------------------------------------------------------
// BlendStateD3D11
//-----------------------------------------------------------------
struct BlendStateD3D11 : public BlendState {
	BlendStateD3D11() {}
	~BlendStateD3D11() {}

	bool create(const BlendStateDesc& desc) final;

	virtual void destroy() final;
	virtual bool isValid() const final;

	const BlendStateDesc& getDesc() const final { return m_bufferedDesc; }
	ID3D11BlendState* D3D11_GetResource() { return m_dx11state; }

  private:
	BlendStateDesc m_bufferedDesc;
	TComPtr<ID3D11BlendState> m_dx11state;
};

} // namespace sge
