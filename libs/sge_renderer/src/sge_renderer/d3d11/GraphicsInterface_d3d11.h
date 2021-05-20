#pragma once

#include "D3D11ContextStateCache.h"
#include "GraphicsCommon_d3d11.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/utils/StringRegister.h"

namespace sge {

struct SGEContextImmediateD3D11;

struct SGEDeviceD3D11 : public SGEDevice {
	friend SGEContextImmediateD3D11;

	SGEDeviceD3D11()
	    : m_immContext(nullptr)
	    , m_VSyncEnabled(false) {}

	bool Create(const MainFrameTargetDesc& frameTargetDesc);

	void Destroy();
	void present() final;

	RAIResource* requestResource(const ResourceType::Enum resourceType) final;

	void releaseResource(RAIResource* resource) final { delete resource; }

	int getStringIndex(const std::string& str) final { return (int)stringRegister.getIndex(str); }

	SGEContext* getContext() final { return (SGEContext*)m_immContext; }
	const MainFrameTargetDesc& getWindowFrameTargetDesc() const { return m_windowFrameTargetDesc; }
	FrameTarget* getWindowFrameTarget() final { return m_screenTarget; }

	void resizeBackBuffer(int width, int height) final;
	void setVsync(const bool enabled) final;
	bool getVsync() const final { return m_VSyncEnabled; }

	VertexDeclIndex getVertexDeclIndex(const VertexDecl* const declElems, const int declElemsCount) final;
	const std::vector<VertexDecl>& getVertexDeclFromIndex(const VertexDeclIndex index) const final;
	const std::map<std::vector<VertexDecl>, VertexDeclIndex>& getVertexDeclMap() const final { return m_vertexDeclIndexMap; }

	SamplerState* requestSamplerState(const SamplerDesc& desc);
	RasterizerState* requestRasterizerState(const RasterDesc& desc) final;
	DepthStencilState* requestDepthStencilState(const DepthStencilDesc& desc) final;
	BlendState* requestBlendState(const BlendStateDesc& desc) final;

	const FrameStatistics& getFrameStatistics() const final { return m_frameStatistics; }

	bool D3D11_CreateSwapChain(const MainFrameTargetDesc& desc);
	std::string D3D11_GetWorkingShaderModel(const ShaderType::Enum shaderType) const;
	FrameTarget* D3D11_GetScreenTarget() { return m_screenTarget; }

	D3D_FEATURE_LEVEL D3D11_GetWorkingFeatureLevel() const { return m_workingFeatureLevel; }
	ID3D11Device* D3D11_GetDevice() { return m_d3d11Device; }
	ID3D11Debug* D3D11_GetDebug() { return m_d3d11Debug; }
	ID3D11DeviceContext* D3D11_GetImmContext() { return m_d3d11Context; }
	ID3D11DeviceContext* D3D11_GetContext() { return m_d3d11Context; }
	IDXGISwapChain* D3D11_GetSwapChain() { return m_d3d11SwapChain; }
	IDXGISwapChain* D3D11_GetDXGISwapChain() { return D3D11_GetSwapChain(); }

	D3D11ContextStateCache* D3D11_GetContextStateCache() { return &m_d3d11_contextStateCache; }

	Buffer* D3D11_GetGlobalUniformsBuffer(ShaderType::Enum shaderType) {
		if (shaderType == ShaderType::VertexShader)
			return m_globalUniformsVS;
		else if (shaderType == ShaderType::PixelShader)
			return m_globalUniformsPS;

		sgeAssert(false);
		return nullptr;
	}

  private:
	FrameStatistics m_frameStatistics;
	bool m_VSyncEnabled;

	// A cache of DepthStencilState, RasterizerState, BlendState.
	std::vector<RasterizerState*> rasterizerStateCache;
	std::vector<DepthStencilState*> depthStencilStateCache;
	std::vector<BlendState*> blendStateCache;
	std::vector<SamplerState*> samplerStateCache;

	std::map<std::vector<VertexDecl>, VertexDeclIndex> m_vertexDeclIndexMap;
	StringRegister stringRegister;

	SGEContextImmediateD3D11* m_immContext = nullptr;

	MainFrameTargetDesc m_windowFrameTargetDesc;
	GpuHandle<FrameTarget> m_screenTarget;

	// D3D11 specific members.
	D3D11ContextStateCache m_d3d11_contextStateCache;

	D3D_FEATURE_LEVEL m_workingFeatureLevel;
	TComPtr<ID3D11Device> m_d3d11Device;
	TComPtr<ID3D11DeviceContext> m_d3d11Context;
	TComPtr<IDXGISwapChain> m_d3d11SwapChain;
	TComPtr<ID3D11Debug> m_d3d11Debug;

	GpuHandle<Texture> m_windowRenderTarget;
	GpuHandle<Texture> m_depthStencil;

	GpuHandle<Buffer> m_globalUniformsVS;
	GpuHandle<Buffer> m_globalUniformsPS;

	GpuHandle<RasterizerState> m_default_RasterizerState;
	GpuHandle<DepthStencilState> m_default_DepthStencilState;
	GpuHandle<BlendState> m_default_blendState;
};

//---------------------------------------------------------------------.
// SGEContextImmediateD3D11
//---------------------------------------------------------------------
struct SGEContextImmediateD3D11 : public SGEContext {
	friend SGEDeviceD3D11;

	SGEContextImmediateD3D11()
	    : m_device(nullptr) {}

	SGEDevice* getDevice() final { return m_device; }
	SGEDeviceD3D11* getDeviceD3D11() { return m_device; }
	void SetSGEDevice(SGEDevice* device) { m_device = static_cast<SGEDeviceD3D11*>(device); }

	void* map(Buffer* buffer, const Map::Enum map) final;
	void unMap(Buffer* buffer) final;

	void updateTextureData(Texture* texture,  const TextureData& td) override;

	void clearColor(FrameTarget* target, int index, const float rgba[4]) final;
	void clearDepth(FrameTarget* target, float depth) final;

	void executeDrawCall(DrawCall& drawCall,
	                     FrameTarget* frameTarget,
	                     const Rect2s* const pViewport = nullptr,
	                     const Rect2s* const pScissorsRect = nullptr) final;

	void beginQuery(Query* const query) final;
	void endQuery(Query* const query) final;

	bool isQueryReady(Query* const query) final;
	bool getQueryData(Query* const query, uint64& queryData) final;



	ID3D11DeviceContext* D3D11_GetImmContext() { return m_device->D3D11_GetImmContext(); }
	D3D11ContextStateCache* D3D11_GetContextStateCache() { return m_device->D3D11_GetContextStateCache(); }

  private:
	SGEDeviceD3D11* m_device;

	D3D11ContextStateCache m_stateCache;
};

} // namespace sge
