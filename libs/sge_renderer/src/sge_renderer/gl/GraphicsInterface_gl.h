#pragma once

#include "sge_renderer/renderer/renderer.h"

#include <sge_utils/utils/StringRegister.h>

#include "GLContextStateCache.h"

#include "Buffer_gl.h"
#include "FrameTarget_gl.h"
#include "Query_gl.h"
#include "RenderState_gl.h"
#include "SamplerState_gl.h"
#include "Shader_gl.h"
#include "ShadingProgram_gl.h"
#include "Texture_gl.h"

namespace sge {

struct SGEContextImmediate;

//---------------------------------------------------------------
// SGEDeviceImpl
//
// The lameness about that class it that it represents both
// the "D3D11" device and the SwapChan for the target window.
//
// I could esily fix that for D3D11, but OpenGL wouldnt give
// this without a fight, as it relyies on an active contexts and
// in addition to that each window must use it's own context and
// in addition not all resources cuold be shared between all context.
//
// Lame OpenGL ... lame...
//
//---------------------------------------------------------------
struct SGEDeviceImpl : public SGEDevice {
	friend SGEContextImmediate;

	SGEDeviceImpl()
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

	GLContextStateCache* GL_GetContextStateCache() { return &m_gl_contextStateCache; }

	RasterizerState* requestRasterizerState(const RasterDesc& desc) final;
	DepthStencilState* requestDepthStencilState(const DepthStencilDesc& desc) final;
	BlendState* requestBlendState(const BlendStateDesc& desc) final;

	const FrameStatistics& getFrameStatistics() const final { return m_frameStatistics; }

  private:
	FrameStatistics m_frameStatistics;
	bool m_VSyncEnabled;


	// A cache of DepthStencilStateGL, RasterizerStateGL, BlendStateGL.
	std::vector<RasterizerState*> rasterizerStateCache;
	std::vector<DepthStencilState*> depthStencilStateCache;
	std::vector<BlendState*> blendStateCache;

	std::map<std::vector<VertexDecl>, VertexDeclIndex> m_vertexDeclIndexMap;

	StringRegister stringRegister;

	SGEContextImmediate* m_immContext = nullptr;

	MainFrameTargetDesc m_windowFrameTargetDesc;
	GpuHandle<FrameTarget> m_screenTarget;


	GLContextStateCache m_gl_contextStateCache;
#if defined(WIN32)
	void* m_gl_hdc; // actually void*
#endif
};

//--------------------------------------------------------
// SGEContextGL
//--------------------------------------------------------
struct SGEContextGL : public SGEContext {
	SGEContextGL()
	    : m_device(nullptr) {}

	SGEDevice* getDevice() final { return m_device; }
	SGEDeviceImpl* getDeviceImpl() { return m_device; }
	void SetSGEDevice(SGEDevice* device) { m_device = static_cast<SGEDeviceImpl*>(device); }

	GLContextStateCache* GL_GetContextStateCache() { return static_cast<SGEDeviceImpl*>(m_device)->GL_GetContextStateCache(); }

  protected:
	SGEDeviceImpl* m_device;
};

//---------------------------------------------------------------------
// [TODO] Rename to SGEImmediateContext or something like that.
// SGEContextImmediate
//---------------------------------------------------------------------
struct SGEContextImmediate : public SGEContextGL {
	friend SGEDeviceImpl;

	SGEContextImmediate() = default;

	void clearColor(FrameTarget* target, int index, const float rgba[4]) final;
	void clearDepth(FrameTarget* target, float depth) final;

	void* map(Buffer* buffer, const Map::Enum map) final;
	void unMap(Buffer* buffer) final;

	void executeDrawCall(DrawCall& drawCall,
	                     FrameTarget* frameTarget,
	                     const Rect2s* const pViewport = nullptr,
	                     const Rect2s* const pScissorsRect = nullptr) final;

	void beginQuery(Query* const query) final;
	void endQuery(Query* const query) final;
	bool isQueryReady(Query* const query) final;
	bool getQueryData(Query* const query, uint64& queryData) final;

	void updateTextureData(Texture* texture,  const TextureData& td) override;
};

//---------------------------------------------------------------------
// SGERecordingContext
//---------------------------------------------------------------------

#if 0
struct SGERecordingContext : public SGEContextGL
{

	void Create(SGEDeviceImpl* device) { m_device = device; }

	void ClearColor(FrameTargetGL* target, int index, const float rgba[4]) override;
	void ClearDepth(FrameTargetGL* target, float depth) override;
	void ExecuteDrawCall(DrawCall& drawCall) override;
	void* Map(BufferGL* buffer, const Map::Enum map) override; // [CAUTION] Only MapDiscard is supported on recording contexts.
	void UnMap(BufferGL* buffer) override;

	void Execute(SGEContextImmediate* context); //temp function ofc.

protected :

	std::vector<APICommand> commandIDs;

	std::vector<DrawCall> drawCalls;
	std::vector<BufferMapCmd> bufferMaps;
	std::vector<ClearColorCmd> clearColors;
	std::vector<ClearDepthStencilCmd> clearDepthStencils;
};
#endif

} // namespace sge

extern template class std::vector<sge::APICommand>;
extern template class std::vector<sge::DrawCall>;
extern template class std::vector<sge::BufferMapCmd>;
extern template class std::vector<sge::ClearColorCmd>;
extern template class std::vector<sge::ClearDepthStencilCmd>;
