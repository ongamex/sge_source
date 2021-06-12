#include "GraphicsInterface_d3d11.h"
#include "sge_utils/utils/timer.h"

#include "Buffer_d3d11.h"
#include "FrameTarget_d3d11.h"
#include "Query_d3d11.h"
#include "RenderState_d3d11.h"
#include "SamplerState_d3d11.h"
#include "Shader_d3d11.h"
#include "ShadingProgram_d3d11.h"
#include "Texture_d3d11.h"

namespace sge {

static ID3D11Device* g_d3d11Device = nullptr;
static ID3D11DeviceContext* g_d3d11ImmDeviceContext = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////
// Create/Destroy
//////////////////////////////////////////////////////////////////////////////////////////
bool SGEDeviceD3D11::Create(const MainFrameTargetDesc& mainFrameDesc) {
#if defined(SGE_USE_DEBUG)
	//#define SGE_USE_D3D_DEBUG
#endif

	Destroy();

	m_windowFrameTargetDesc = mainFrameDesc;

	const D3D_FEATURE_LEVEL featureLevels[] = {
	    D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_9_3,
	};

	DWORD deviceFlags = 0;
#ifdef SGE_USE_D3D_DEBUG
	// TODO: Concider adding an andditional flag that enables the debug layers.
	// TODO: What about D3D11_CREATE_DEVICE_DEBUGGABLE ?
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	if (!g_d3d11Device) {
		for (int itr = 0; true; itr++) {
			const HRESULT hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, deviceFlags, NULL,
			                                     NULL, // featureLevels, _ARRAYSIZE(featureLevels),
			                                     D3D11_SDK_VERSION, &g_d3d11Device, &m_workingFeatureLevel, &g_d3d11ImmDeviceContext);

			if (FAILED(hr)) {
				if (deviceFlags | D3D11_CREATE_DEVICE_DEBUG) {
					sgeAssert(false && "Failed to create D3D11 Device with debug layers enabled!");

					// Try again with disabled debug layers.
					deviceFlags = deviceFlags & ~(DWORD)(D3D11_CREATE_DEVICE_DEBUG);
					continue;
				}

				return false;
			}

			break;
		}
	}

	m_d3d11Device = g_d3d11Device;
	m_d3d11Context = g_d3d11ImmDeviceContext;

#ifdef SGE_USE_D3D_DEBUG
	m_d3d11Device->QueryInterface(__uuidof(ID3D11Debug), (void**)(&m_d3d11Debug));
	TComPtr<ID3D11InfoQueue> d3dInfoQueue;
	m_d3d11Debug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&d3dInfoQueue);
	if (d3dInfoQueue) {
		d3dInfoQueue->SetBreakOnCategory(D3D11_MESSAGE_CATEGORY_COMPILATION, TRUE);
		d3dInfoQueue->SetBreakOnCategory(D3D11_MESSAGE_CATEGORY_INITIALIZATION, TRUE);
		d3dInfoQueue->SetBreakOnCategory(D3D11_MESSAGE_CATEGORY_STATE_CREATION, TRUE);
		d3dInfoQueue->SetBreakOnCategory(D3D11_MESSAGE_CATEGORY_RESOURCE_MANIPULATION, TRUE);
		// d3dInfoQueue->SetBreakOnCategory(D3D11_MESSAGE_CATEGORY_EXECUTION, TRUE);

		// D3D11 WARNING : ID3D11DeviceContext::DrawIndexed : The Pixel Shader unit expects a Sampler to be set at Slot 0, but none is
		// bound.This is perfectly valid, as a NULL Sampler maps to default Sampler state.However, the developer may not want to rely on the
		// defaults.[EXECUTION WARNING #352: DEVICE_DRAW_SAMPLER_NOT_SET]
		// d3dInfoQueue->SetBreakOnID(D3D11_MESSAGE_ID_DEVICE_DRAW_SAMPLER_NOT_SET, FALSE);
	}
#endif

	// Everything went fine
	m_immContext = new SGEContextImmediateD3D11();
	m_immContext->SetSGEDevice(this);
	setVsync(mainFrameDesc.vSync);

	// SGE_DEBUG_LOG("D3D11 Device Created with FeatureLevel = 0x%x\n", m_workingFeatureLevel);

	// Create the swapchain to the target window and allocate the default FrameTargetD3D11
	const bool succeedeCreatingSC = D3D11_CreateSwapChain(mainFrameDesc);
	sgeAssert(succeedeCreatingSC);

	// Create globally used constant buffers
	// This buffers are used for emulation of D3D9/GL numerical uniform binding
	{
		m_globalUniformsVS = requestResource(ResourceType::Buffer);
		m_globalUniformsPS = requestResource(ResourceType::Buffer);

		const BufferDesc bd = BufferDesc::GetDefaultConstantBuffer(16 * 64, ResourceUsage::Dynamic);

		m_globalUniformsVS->create(bd, nullptr);
		m_globalUniformsPS->create(bd, nullptr);
	}

	m_d3d11_contextStateCache.m_d3dcon = D3D11_GetImmContext();

	m_default_RasterizerState = requestResource(ResourceType::RasterizerState);
	m_default_RasterizerState->create(RasterDesc());

	m_default_DepthStencilState = requestResource(ResourceType::DepthStencilState);
	m_default_DepthStencilState->create(DepthStencilDesc());

	m_default_blendState = requestResource(ResourceType::BlendState);
	m_default_blendState->create(BlendDesc());

	return true;
}

//--------------------------------------------------------------------------------------
bool SGEDeviceD3D11::D3D11_CreateSwapChain(const MainFrameTargetDesc& desc) {
	TComPtr<IDXGIDevice> DXGIDevice;
	const HRESULT hr = m_d3d11Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&DXGIDevice);

	if (hr != S_OK) {
		sgeAssert(false && "Failed to create D3D Swap Chain");
		return false;
	}

	TComPtr<IDXGIAdapter> DXGIAdapter;
	DXGIDevice->GetAdapter(&DXGIAdapter);

	DXGI_ADAPTER_DESC adapterDesc;
	if (FAILED(DXGIAdapter->GetDesc(&adapterDesc))) {
		// [TODO] Something...
		sgeAssert(false);
	}

	char descVideoMbcs[512] = {0};

	static const int ttt = SGE_ARRSZ(descVideoMbcs);

	// wcstombs(descVideoMbcs, adapterDesc.Description, SGE_ARRSZ(descVideoMbcs));
	size_t numCharsConverted;
	wcstombs_s(&numCharsConverted, descVideoMbcs, adapterDesc.Description, SGE_ARRSZ(descVideoMbcs));

	// SGE_DEBUG_LOG("%s\n", descVideoMbcs);

	TComPtr<IDXGIFactory> DXGIFactory;
	DXGIAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&DXGIFactory);

	// build the swapchain desc
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {0};

	// convert SGE texture format to DXGI_FORMAT and use the SRV(shader resource view format)
	DXGI_FORMAT srv, dsv, typeless;
	TextureFormat_D3D11_Native(TextureFormat::R8G8B8A8_UNORM, srv, dsv, typeless);

	// convert SGE swapchain desc to dx11
	swapChainDesc.BufferDesc.Format = srv;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Width = desc.width;
	swapChainDesc.BufferDesc.Height = desc.height;
	swapChainDesc.OutputWindow = (HWND)desc.hWindow;
	swapChainDesc.SampleDesc.Count = desc.sampleDesc.Count;
	swapChainDesc.SampleDesc.Quality = desc.sampleDesc.Quality;
	swapChainDesc.Windowed = desc.bWindowed;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	// swapChainDesc.BufferCount:
	// http://msdn.microsoft.com/en-us/library/windows/desktop/bb173075(v=vs.85).aspx
	swapChainDesc.BufferCount = desc.numBuffers + ((desc.bWindowed) ? 0 : 1);
	swapChainDesc.Flags = 0;

	if (FAILED(DXGIFactory->CreateSwapChain(m_d3d11Device, &swapChainDesc, &m_d3d11SwapChain))) {
		return false;
	}

	// Create the render target
	ID3D11Texture2D* backBufferRenderTarget = nullptr;

	if (FAILED((m_d3d11SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBufferRenderTarget)))) {
		return false;
	}

	TextureDesc textureDesc;

	textureDesc.textureType = UniformType::Texture2D;
	textureDesc.format = TextureFormat::R8G8B8A8_UNORM;
	textureDesc.usage = TextureUsage::RenderTargetOnly;
	textureDesc.texture2D.arraySize = 1;
	textureDesc.texture2D.numMips = 1;
	textureDesc.texture2D.numSamples = desc.sampleDesc.Count;
	textureDesc.texture2D.sampleQuality = desc.sampleDesc.Quality;
	textureDesc.texture2D.width = desc.width;
	textureDesc.texture2D.height = desc.height;

	m_windowRenderTarget = requestResource(ResourceType::Texture);
	((TextureD3D11*)m_windowRenderTarget.GetPtr())->D3D11_WrapOverD3D11TextureResource(this, backBufferRenderTarget, textureDesc);
	backBufferRenderTarget->Release();

	// Create the default depth buffer
	TextureDesc depthStencilDesc;

	depthStencilDesc.textureType = UniformType::Texture2D;
	depthStencilDesc.format = TextureFormat::D24_UNORM_S8_UINT;
	depthStencilDesc.usage = TextureUsage::DepthStencilOnly;
	depthStencilDesc.texture2D.arraySize = 1;
	depthStencilDesc.texture2D.numMips = 1;
	depthStencilDesc.texture2D.numSamples = desc.sampleDesc.Count;
	depthStencilDesc.texture2D.sampleQuality = desc.sampleDesc.Quality;
	depthStencilDesc.texture2D.width = desc.width;
	depthStencilDesc.texture2D.height = desc.height;

	m_depthStencil = requestResource(ResourceType::Texture);
	m_depthStencil->create(depthStencilDesc, 0);

	// Create the screen frame target
	m_screenTarget = requestResource(ResourceType::FrameTarget);
	m_screenTarget->create();
	m_screenTarget->setRenderTarget(0, m_windowRenderTarget, TargetDesc::FromTex2D());
	m_screenTarget->setDepthStencil(m_depthStencil, TargetDesc::FromTex2D());

	return true;
}

void SGEDeviceD3D11::resizeBackBuffer(int width, int height) {
	if (m_windowFrameTargetDesc.width == width && m_windowFrameTargetDesc.height == height) {
		return;
	}

	m_screenTarget->destroy();
	m_windowRenderTarget->destroy();
	m_depthStencil->destroy();

	m_windowFrameTargetDesc.width = width;
	m_windowFrameTargetDesc.height = height;

	auto& desc = m_windowFrameTargetDesc;

	m_d3d11_contextStateCache.SetRenderTargetsAndDepthStencil(0, 0, 0, 0);

	[[maybe_unused]] HRESULT hr = m_d3d11SwapChain->ResizeBuffers(
	    m_windowFrameTargetDesc.numBuffers + ((m_windowFrameTargetDesc.bWindowed) ? 0 : 1), width, height, DXGI_FORMAT_UNKNOWN, 0);
	sgeAssert(SUCCEEDED(hr));

	// Create the render target
	ID3D11Texture2D* backBufferRenderTarget = nullptr;

	if (FAILED((m_d3d11SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBufferRenderTarget)))) {
		sgeAssert(false);
	}

	TextureDesc textureDesc;

	textureDesc.textureType = UniformType::Texture2D;
	textureDesc.format = TextureFormat::R8G8B8A8_UNORM;
	textureDesc.usage = TextureUsage::RenderTargetOnly;
	textureDesc.texture2D.arraySize = 1;
	textureDesc.texture2D.numMips = 1;
	textureDesc.texture2D.numSamples = desc.sampleDesc.Count;
	textureDesc.texture2D.sampleQuality = desc.sampleDesc.Quality;
	textureDesc.texture2D.width = desc.width;
	textureDesc.texture2D.height = desc.height;

	((TextureD3D11*)(m_windowRenderTarget.GetPtr()))->D3D11_WrapOverD3D11TextureResource(this, backBufferRenderTarget, textureDesc);
	backBufferRenderTarget->Release();

	// Create the default depth buffer
	TextureDesc depthStencilDesc;

	depthStencilDesc.textureType = UniformType::Texture2D;
	depthStencilDesc.format = TextureFormat::D24_UNORM_S8_UINT;
	depthStencilDesc.usage = TextureUsage::DepthStencilOnly;
	depthStencilDesc.texture2D.arraySize = 1;
	depthStencilDesc.texture2D.numMips = 1;
	depthStencilDesc.texture2D.numSamples = desc.sampleDesc.Count;
	depthStencilDesc.texture2D.sampleQuality = desc.sampleDesc.Quality;
	depthStencilDesc.texture2D.width = desc.width;
	depthStencilDesc.texture2D.height = desc.height;

	m_depthStencil->create(depthStencilDesc, 0);

	m_screenTarget->destroy();
	m_screenTarget->create();
	m_screenTarget->setRenderTarget(0, m_windowRenderTarget, TargetDesc::FromTex2D());
	m_screenTarget->setDepthStencil(m_depthStencil, TargetDesc::FromTex2D());
}

//--------------------------------------------------------------------------------------
void SGEDeviceD3D11::setVsync(const bool enabled) {
	m_VSyncEnabled = enabled;
}

//--------------------------------------------------------------------------------------
void SGEDeviceD3D11::Destroy() {
	m_d3d11Device.Release();
	m_d3d11Context.Release();
	m_d3d11SwapChain.Release();
	m_screenTarget.Release();
	m_windowRenderTarget.Release();
}

//--------------------------------------------------------------------------------------
std::string SGEDeviceD3D11::D3D11_GetWorkingShaderModel(const ShaderType::Enum shaderType) const {
	std::string shaderModel;

	switch (shaderType) {
		// case ShaderType::ComputeShader:
		//	shaderModel += "cs_";
		//	break;
		case ShaderType::VertexShader:
			shaderModel += "vs_";
			break;
		case ShaderType::PixelShader:
			shaderModel += "ps_";
			break;

		default: {
			sgeAssert(false);
			return "";
		}
	}

	switch (m_workingFeatureLevel) {
		case D3D_FEATURE_LEVEL_11_1:
		case D3D_FEATURE_LEVEL_11_0:
			shaderModel += "5_0";
			break;

		case D3D_FEATURE_LEVEL_10_1:
		case D3D_FEATURE_LEVEL_10_0:
			shaderModel += "4_0";
			break;

		case D3D_FEATURE_LEVEL_9_3:
			shaderModel += "4_0_level_9_3";
			break;

		default: {
			sgeAssert(false);
			return "";
		}
	}

	return shaderModel;
}

void SGEDeviceD3D11::present() {
	m_d3d11SwapChain->Present(m_VSyncEnabled ? 1 : 0, 0);
	m_frameStatistics.Reset();
	float const now = Timer::now_seconds();
	m_frameStatistics.lastPresentDt = now - m_frameStatistics.lastPresentTime;
	m_frameStatistics.lastPresentTime = now;
}

///////////////////////////////////////////////////////////////////////////////////////////
// SGEContextImmediateD3D11
///////////////////////////////////////////////////////////////////////////////////////////
void SGEContextImmediateD3D11::clearColor(FrameTarget* target, int index, const float rgba[4]) {
	if (target == NULL) {
		target = ((SGEDeviceD3D11*)getDevice())->D3D11_GetScreenTarget();
	}

	int loopUpIdx = GraphicsCaps::kRenderTargetSlotsCount;
	if (index == -1) {
		index = 0;
		loopUpIdx = index + 1;
	}

	//[TODO] hardcoded
	for (; index < loopUpIdx; ++index) {
		ID3D11RenderTargetView* rtv = ((FrameTargetD3D11*)target)->D3D11_GetRTV(index);
		if (rtv != NULL)
			D3D11_GetImmContext()->ClearRenderTargetView(rtv, rgba);
	}
}

//--------------------------------------------------------------------------
void SGEContextImmediateD3D11::clearDepth(FrameTarget* target, float depth) {
	if (target == NULL) {
		target = ((SGEDeviceD3D11*)getDevice())->D3D11_GetScreenTarget();
	}
	ID3D11DepthStencilView* dsv = ((FrameTargetD3D11*)target)->D3D11_GetDSV();
	if (dsv)
		D3D11_GetImmContext()->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, depth, 0);
}

void* SGEContextImmediateD3D11::map(Buffer* buffer, const Map::Enum map) {
	return ((BufferD3D11*)buffer)->map(map, this);
}
void SGEContextImmediateD3D11::unMap(Buffer* buffer) {
	((BufferD3D11*)buffer)->unMap(this);
}

void SGEContextImmediateD3D11::updateTextureData(Texture* texture, const TextureData& texData) {
	if (texture && texture->getDesc().textureType == UniformType::Texture2D && texData.data) {
		TextureD3D11* texD3D = ((TextureD3D11*)texture);

		D3D11_MAPPED_SUBRESOURCE mapped;
		HRESULT hr = D3D11_GetImmContext()->Map(texD3D->D3D11_GetTextureResource(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

		if (SUCCEEDED(hr)) {
			char* rowWriteLocation = (char*)mapped.pData;
			for (int t = 0; t < texture->getDesc().texture2D.height; ++t) {
				memcpy(rowWriteLocation, (char*)texData.data + t * texData.rowByteSize, texData.rowByteSize);
				rowWriteLocation += mapped.RowPitch;
			}



			D3D11_GetImmContext()->Unmap(texD3D->D3D11_GetTextureResource(), 0);
		}
	}
}

//--------------------------------------------------------------------------
void SGEContextImmediateD3D11::beginQuery(Query* const query) {
	QueryD3D11* const queryImpl = (QueryD3D11*)query;

	D3D11_GetImmContext()->Begin(queryImpl->D3D11_GetResource());
}

void SGEContextImmediateD3D11::endQuery(Query* const query) {
	QueryD3D11* const queryImpl = (QueryD3D11*)query;
	D3D11_GetImmContext()->End(queryImpl->D3D11_GetResource());
}

bool SGEContextImmediateD3D11::isQueryReady(Query* const query) {
	if (!query || !query->isValid()) {
		sgeAssert(false);
		return false;
	}

	QueryD3D11* const queryImpl = (QueryD3D11*)query;
	HRESULT const hr = D3D11_GetImmContext()->GetData(queryImpl->D3D11_GetResource(), nullptr, 0, 0);
	return hr == S_OK;
}

bool SGEContextImmediateD3D11::getQueryData(Query* const query, uint64& queryData) {
	if (!query || !query->isValid()) {
		sgeAssert(false);
		return false;
	}

	QueryD3D11* const queryImpl = (QueryD3D11*)query;
	HRESULT const hr = D3D11_GetImmContext()->GetData(queryImpl->D3D11_GetResource(), &queryData, sizeof(queryData), 0);
	return hr == S_OK;
}

//--------------------------------------------------------------------------
void SGEContextImmediateD3D11::executeDrawCall(DrawCall& drawCall,
                                               FrameTarget* frameTarget,
                                               const Rect2s* const pViewport,
                                               const Rect2s* const pScissorsRect) {
	if (drawCall.m_pStateGroup == nullptr) {
		sgeAssert(false && "Aborting draw call! No state group is specified!\n");
		return;
	}

	D3D11ContextStateCache* const stateCache = this->D3D11_GetContextStateCache();
	StateGroup& stateGroup = *drawCall.m_pStateGroup;

	ID3D11Buffer* cbuffers[ShaderType::NumElems][GraphicsCaps::kConstantBufferSlotsCount] = {nullptr};
	ID3D11ShaderResourceView* srvs[ShaderType::NumElems][GraphicsCaps::kD3D11_SRV_Count] = {nullptr};
	ID3D11SamplerState* samplers[ShaderType::NumElems][GraphicsCaps::kSampleSlotsCount] = {nullptr};

	const Rect2s viewport = (pViewport != nullptr) ? *pViewport : frameTarget->getViewport();
	const D3D11_VIEWPORT d3d11_viewport = Viewport_D3D11_Native(viewport);

	// Render targets and depth stencil.
	FrameTargetD3D11* frameTargetD3D11 = (FrameTargetD3D11*)frameTarget;

	{
		StaticArray<ID3D11RenderTargetView*, GraphicsCaps::kRenderTargetSlotsCount> rtvs;

		// render targets
		ID3D11DepthStencilView* dsv = frameTargetD3D11->D3D11_GetDSV();

		for (int t = 0; t < GraphicsCaps::kRenderTargetSlotsCount; ++t) {
			const bool wasPushBackSuccessful = rtvs.push_back(frameTargetD3D11->D3D11_GetRTV(t));
			sgeAssert(wasPushBackSuccessful);
		}

		stateCache->SetRenderTargetsAndDepthStencil(0, rtvs.size(), rtvs.data(), dsv);
	}

	// Shaders and shader resources.
	sgeAssert(drawCall.m_pStateGroup->m_shadingProg && drawCall.m_pStateGroup->m_shadingProg->isValid());
	{
		ShadingProgramD3D11* const shadingProgram = ((ShadingProgramD3D11*)stateGroup.m_shadingProg);

		ID3D11VertexShader* const vs = shadingProgram->D3D11_GetVertexShader();
		ID3D11PixelShader* const ps = shadingProgram->D3D11_GetPixelShader();

		stateCache->SetVS(vs);
		stateCache->SetPS(ps);
	}

	// Bind the uniforms.
	{
		// Check if $Globals cbuffer are going to be used. Map the used ones.
		void* globalCBufferMappedMem[ShaderType::NumElems] = {nullptr};
		int globalCBufferSlot[ShaderType::NumElems] = {-1};

		for (int t = 0; t < ShaderType::NumElems; ++t) {
			globalCBufferSlot[t] = ((ShadingProgramD3D11*)(stateGroup.m_shadingProg))->getGlobalCBufferSlot((ShaderType::Enum)t);
			if (globalCBufferSlot[t] >= 0) {
				BufferD3D11* const cbuffer = (BufferD3D11*)getDeviceD3D11()->D3D11_GetGlobalUniformsBuffer((ShaderType::Enum)t);
				globalCBufferMappedMem[t] = cbuffer->map(Map::WriteDiscard, this);
			}
		}

		for (int iUniform = 0; iUniform < drawCall.numUniforms; ++iUniform) {
			BoundUniform& uniform = drawCall.uniforms[iUniform];
			const BindLocation& bindLocation = uniform.bindLocation;

			switch (bindLocation.uniformType) {
				case UniformType::Texture1D:
				case UniformType::Texture2D:
				case UniformType::TextureCube:
				case UniformType::Texture3D: {
					// Caution:
					// Currently if the array size is 1, we assume that a single texture has been bound via
					// uniform.texture and not an array with uniform.textures with 1 element specified.
					if (bindLocation.texArraySize_or_numericUniformSizeBytes == 1) {
						TextureD3D11* texture = reinterpret_cast<TextureD3D11*>(uniform.texture);
						srvs[bindLocation.shaderFreq][bindLocation.bindLocation] = texture ? texture->D3D11_GetSRV() : nullptr;
					} else {
						for (int t = 0; t < bindLocation.texArraySize_or_numericUniformSizeBytes; ++t) {
							TextureD3D11* const d3d11Tex = static_cast<TextureD3D11*>(uniform.textures[t]);
							srvs[bindLocation.shaderFreq][bindLocation.bindLocation + t] = d3d11Tex ? d3d11Tex->D3D11_GetSRV() : nullptr;
						}
					}
				} break;

				case UniformType::SamplerState: {
					// Caution:
					// Currently if the array size is 1, we assume that a single sampler has been bound via
					// uniform.sampler and not an array with uniform.samplers with 1 element specified.
					if (bindLocation.texArraySize_or_numericUniformSizeBytes == 0) {
						samplers[bindLocation.shaderFreq][bindLocation.bindLocation] =
						    ((SamplerStateD3D11*)(uniform.sampler))->D3D11_GetResource();
					} else {
						for (int t = 0; t < bindLocation.texArraySize_or_numericUniformSizeBytes; ++t) {
							SamplerStateD3D11* const d3d11Samp = static_cast<SamplerStateD3D11*>(uniform.samplers[t]);
							samplers[bindLocation.shaderFreq][bindLocation.bindLocation + t] =
							    d3d11Samp ? d3d11Samp->D3D11_GetResource() : nullptr;
						}
					}
				} break;

				case UniformType::ConstantBuffer: {
					cbuffers[bindLocation.shaderFreq][bindLocation.bindLocation] = ((BufferD3D11*)(uniform.buffer))->D3D11_GetResource();
				} break;

				default: {
					if (UniformType::isNumeric((UniformType::Enum)bindLocation.uniformType)) {
						char* const cBufferMem = (char*)globalCBufferMappedMem[bindLocation.shaderFreq];
						if_checked(cBufferMem != nullptr) {
							memcpy(cBufferMem + bindLocation.bindLocation, uniform.data,
							       bindLocation.texArraySize_or_numericUniformSizeBytes);
						}
					} else {
						sgeAssert(false);
					}

				} break;
			}
		}

		// Now unmap the previously mapped buffers and bind them.
		for (int t = 0; t < ShaderType::NumElems; ++t) {
			if (globalCBufferMappedMem[t] != nullptr) {
				BufferD3D11* const cbuffer = (BufferD3D11*)getDeviceD3D11()->D3D11_GetGlobalUniformsBuffer((ShaderType::Enum)t);
				cbuffer->unMap(this);
				cbuffers[t][globalCBufferSlot[t]] = cbuffer->D3D11_GetResource();
			}
		}

		// Now bind the resources.
		for (int t = 0; t < ShaderType::NumElems; ++t) {
			ShaderType::Enum const shaderType = (ShaderType::Enum)t;
			stateCache->BindConstantBuffers(shaderType, 0, SGE_ARRSZ(cbuffers[t]), cbuffers[t]);
			stateCache->BindSRVs(shaderType, 0, SGE_ARRSZ(srvs[t]), srvs[t]);
			stateCache->BindSamplers(shaderType, 0, SGE_ARRSZ(samplers[t]), samplers[t]);
		}
	}

	// Input layout.
	ID3D11InputLayout* const inputLayout =
	    ((ShaderD3D11*)stateGroup.m_shadingProg->getVertexShader())->D3D11_GetInputLayoutForVertexDeclIndex(stateGroup.m_vertDeclIndex);

#ifdef SGE_USE_DEBUG
	// In order to simplfy debugging, find the vertex declaration in debug builds.
	[[maybe_unused]] const std::vector<VertexDecl>& debug_vertexDecl =
	    getDeviceD3D11()->getVertexDeclFromIndex(drawCall.m_pStateGroup->m_vertDeclIndex);
#endif

	sgeAssert(inputLayout != nullptr);
	stateCache->SetInputLayout(inputLayout);

	// vertex buffers
	{
		// int startSlot = -1, endSlot = 0;

		ID3D11Buffer* vb[GraphicsCaps::kVertexBufferSlotsCount] = {nullptr};
		UINT vbByteOffsets[GraphicsCaps::kVertexBufferSlotsCount] = {0};
		UINT vbStrides[GraphicsCaps::kVertexBufferSlotsCount] = {0};

		for (int t = 0; t < GraphicsCaps::kVertexBufferSlotsCount; ++t) {
			if (stateGroup.m_vertexBuffers[t]) {
				vb[t] = ((BufferD3D11*)stateGroup.m_vertexBuffers[t])->D3D11_GetResource();
				vbByteOffsets[t] = stateGroup.m_vbOffsets[t];
				vbStrides[t] = stateGroup.m_vbStrides[t];
			}
		}

		stateCache->BindVertexBuffers(vb, vbStrides, vbByteOffsets, SGE_ARRSZ(vb), 0);
	}

	// index buffer
	if (stateGroup.m_indexBuffer) {
		ID3D11Buffer* const indexBuffer = ((BufferD3D11*)stateGroup.m_indexBuffer)->D3D11_GetResource();
		DXGI_FORMAT const fmt = UniformType_GetDX_DXGI_FORMAT(stateGroup.m_indexBufferFormat);
		UINT const byteOffset = stateGroup.m_indexBufferByteOffset;

		stateCache->BindIndexBuffer(indexBuffer, fmt, byteOffset);
	}



	// Blending
	if (stateGroup.m_blendState && stateGroup.m_blendState->isValid()) {
		stateCache->SetBlendState(((BlendStateD3D11*)stateGroup.m_blendState)->D3D11_GetResource());
	} else {
		stateCache->SetBlendState(((BlendStateD3D11*)(getDeviceD3D11()->m_default_blendState.GetPtr()))->D3D11_GetResource());
	}

	// Rasterizer state with scissors.
	if (stateGroup.m_rasterState && stateGroup.m_rasterState->isValid()) {
		stateCache->SetRasterizerState(((RasterizerStateD3D11*)stateGroup.m_rasterState)->D3D11_GetResource());

		// The scissors rect.
		if (stateGroup.m_rasterState->getDesc().useScissor) {
			sgeAssert(pScissorsRect != NULL);

			const Rect2s scissorsRect = [&]() -> Rect2s {
				if (pScissorsRect != nullptr) {
					return *pScissorsRect;
				}

				sgeAssert(false);

				// Just bind a large rect to make debugging easier.
				return Rect2s(viewport.width, viewport.height);
			}();

			const CD3D11_RECT d3d11_rect(scissorsRect.x, scissorsRect.y, scissorsRect.x + scissorsRect.width,
			                             scissorsRect.y + scissorsRect.height);

			stateCache->SetScissors(&d3d11_rect, 1);
		}
	} else {
		// [TODO] Cache this.
		stateCache->SetRasterizerState(
		    ((RasterizerStateD3D11*)(getDeviceD3D11()->m_default_RasterizerState.GetPtr()))->D3D11_GetResource());
	}

	// Depth stencil state.
	if (stateGroup.m_depthStencilState && stateGroup.m_depthStencilState->isValid()) {
		stateCache->SetDepthStencilState(((DepthStencilStateD3D11*)stateGroup.m_depthStencilState)->D3D11_GetResource());
	} else {
		stateCache->SetDepthStencilState(
		    ((DepthStencilStateD3D11*)(getDeviceD3D11()->m_default_DepthStencilState.GetPtr()))->D3D11_GetResource());
	}

	// Viewport.
	stateCache->setViewport(d3d11_viewport);

	// Primitive topolgy.
	const D3D11_PRIMITIVE_TOPOLOGY d3d11_primitiveTopology = PrimitiveTopology_D3D11_Native(stateGroup.m_primTopology);
	stateCache->setPrimitiveTopology(d3d11_primitiveTopology);

	// Execute the draw call.
	size_t numPrimitivesDrawn = 0;

	sgeAssert(drawCall.m_drawExec.IsValid());

	if (drawCall.m_drawExec.GetType() == DrawExecDesc::Type_Linear) {
		const auto& call = drawCall.m_drawExec.LinearCall();

		numPrimitivesDrawn = PrimitiveTopology::GetNumPrimitivesByPoints(stateGroup.m_primTopology, call.numVerts) * call.numInstances;

		if (call.numInstances == 1) {
			stateCache->m_d3dcon->Draw(call.numVerts, call.startVert);
		} else {
			stateCache->m_d3dcon->DrawInstanced(call.numVerts, call.numInstances, call.startVert, 0);
		}
	} else if (drawCall.m_drawExec.GetType() == DrawExecDesc::Type_Indexed) {
		const auto& call = drawCall.m_drawExec.IndexedCall();

		numPrimitivesDrawn = PrimitiveTopology::GetNumPrimitivesByPoints(stateGroup.m_primTopology, call.numIndices) * call.numInstances;

		if (call.numInstances == 1) {
			stateCache->m_d3dcon->DrawIndexed(call.numIndices, call.startIndex, call.startVertex);
		} else {
			stateCache->m_d3dcon->DrawIndexedInstanced(call.numIndices, call.numInstances, call.startIndex, call.startVertex, 0);
		}
	}

	this->getDeviceD3D11()->m_frameStatistics.numDrawCalls += 1;
	this->getDeviceD3D11()->m_frameStatistics.numPrimitiveDrawn += numPrimitivesDrawn;
}

SGEDevice* SGEDevice::create(const MainFrameTargetDesc& frameTargetDesc) {
	SGEDeviceD3D11* s = new SGEDeviceD3D11();
	s->Create(frameTargetDesc);
	return s;
}

RAIResource* SGEDeviceD3D11::requestResource(const ResourceType::Enum resourceType) {
	RAIResource* result = nullptr;

	if (resourceType == ResourceType::Buffer)
		result = new BufferD3D11;
	if (resourceType == ResourceType::Texture)
		result = new TextureD3D11;
	if (resourceType == ResourceType::Sampler)
		result = new SamplerStateD3D11;
	if (resourceType == ResourceType::FrameTarget)
		result = new FrameTargetD3D11;
	if (resourceType == ResourceType::Shader)
		result = new ShaderD3D11;
	if (resourceType == ResourceType::ShadingProgram)
		result = new ShadingProgramD3D11;
	if (resourceType == ResourceType::Query)
		result = new QueryD3D11;
	if (resourceType == ResourceType::RasterizerState)
		result = new RasterizerStateD3D11;
	if (resourceType == ResourceType::DepthStencilState)
		result = new DepthStencilStateD3D11;
	if (resourceType == ResourceType::BlendState)
		result = new BlendStateD3D11;

	if (!result) {
		sgeAssert(false && "Unknown resource type");
		return nullptr;
	}

	result->setDeviceInternal(this);

	return result;
}


RasterizerState* SGEDeviceD3D11::requestRasterizerState(const RasterDesc& desc) {
	// Search if the resource exists.
	auto itr = std::find_if(rasterizerStateCache.begin(), rasterizerStateCache.end(),
	                        [&desc](const RasterizerState* state) -> bool { return state->getDesc() == desc; });

	if (itr != std::end(rasterizerStateCache)) {
		return *itr;
	}


	// Create the new resource;
	RasterizerState* const state = (RasterizerState*)requestResource(ResourceType::RasterizerState);
	state->create(desc);
	sgeAssert(state->isValid());

	// Add the 1 ref to the resource (this cointainer holds it).
	state->addRef();
	rasterizerStateCache.push_back(state);

	return state;
}

DepthStencilState* SGEDeviceD3D11::requestDepthStencilState(const DepthStencilDesc& desc) {
	// Search if the resource exists.
	auto itr = std::find_if(depthStencilStateCache.begin(), depthStencilStateCache.end(),
	                        [&desc](const DepthStencilState* state) -> bool { return state->getDesc() == desc; });

	if (itr != std::end(depthStencilStateCache)) {
		return *itr;
	}

	// Create the new resource;
	DepthStencilState* const state = (DepthStencilState*)requestResource(ResourceType::DepthStencilState);
	state->create(desc);
	sgeAssert(state->isValid());

	// Add the 1 ref to the resource (this cointainer holds it).
	state->addRef();
	depthStencilStateCache.push_back(state);

	return state;
}

BlendState* SGEDeviceD3D11::requestBlendState(const BlendStateDesc& desc) {
	// Search if the resource exists.
	auto itr = std::find_if(blendStateCache.begin(), blendStateCache.end(),
	                        [&desc](const BlendState* state) -> bool { return state->getDesc() == desc; });

	if (itr != std::end(blendStateCache)) {
		return *itr;
	}

	// Create the new resource;
	BlendState* const state = (BlendState*)requestResource(ResourceType::BlendState);
	state->create(desc);
	sgeAssert(state->isValid());

	// Add the 1 ref to the resource (this cointainer holds it).
	state->addRef();
	blendStateCache.push_back(state);

	return state;
}

SamplerState* SGEDeviceD3D11::requestSamplerState(const SamplerDesc& desc) {
	// Search if the resource exists.
	auto itr = std::find_if(samplerStateCache.begin(), samplerStateCache.end(),
	                        [&desc](const SamplerState* state) -> bool { return state->getDesc() == desc; });

	if (itr != std::end(samplerStateCache)) {
		return *itr;
	}

	// Create the new resource;
	SamplerState* const state = (SamplerState*)requestResource(ResourceType::Sampler);
	state->create(desc);
	sgeAssert(state->isValid());

	// Add the 1 ref to the resource (this cointainer holds it).
	state->addRef();
	samplerStateCache.push_back(state);

	return state;
}

VertexDeclIndex SGEDeviceD3D11::getVertexDeclIndex(const VertexDecl* const declElems, const int declElemsCount) {
	const std::vector<VertexDecl> decl = VertexDecl::NormalizeDecl(declElems, declElemsCount);

	VertexDeclIndex& idx = static_cast<VertexDeclIndex>(m_vertexDeclIndexMap[decl]);
	static_assert(VertexDeclIndex_Null == 0, "");
	if (idx == VertexDeclIndex_Null) {
		idx = static_cast<VertexDeclIndex>(m_vertexDeclIndexMap.size());
	}

	return idx;
}

const std::vector<VertexDecl>& SGEDeviceD3D11::getVertexDeclFromIndex(const VertexDeclIndex index) const {
	for (const auto& e : m_vertexDeclIndexMap) {
		if (e.second == index) {
			return e.first;
		}
	}
	static std::vector<VertexDecl> empty = std::vector<VertexDecl>();
	return empty;
}


} // namespace sge
