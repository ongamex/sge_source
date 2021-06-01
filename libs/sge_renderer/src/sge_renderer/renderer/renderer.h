#pragma once

#include <map>

#include "GraphicsCommon.h"
#include "ShaderReflection.h"

namespace sge {

struct DrawCall;

struct SGEDevice;
struct SGEContext;

struct Buffer;
struct FrameTarget;
struct Texture;
struct SamplerState;
struct Shader;
struct ShadingProgram;
struct Query;
struct RasterizerState;
struct DepthStencilState;
struct BlendState;

#ifdef SGE_RENDERER_D3D11
constexpr bool kIsTexcoordStyleD3D = true;
constexpr float kNDCNear = 0.f;
#endif

#ifdef SGE_RENDERER_GL
constexpr bool kIsTexcoordStyleD3D = false;
constexpr float kNDCNear = -1.f;
#endif

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
struct RAIResource {
	RAIResource() = default;
	virtual ~RAIResource() = default;

	RAIResource(const RAIResource&) = delete;
	RAIResource& operator=(const RAIResource&) = delete;

	void addRef() { m_refcnt++; }
	void releaseRef() { m_refcnt--; }
	int getRefCount() const { return m_refcnt; }

	SGEDevice* getDevice() { return m_device; }
	const SGEDevice* getDevice() const { return m_device; }

	template <typename T>
	T* getDevice() {
		return static_cast<T*>(getDevice());
	}

	void setDeviceInternal(SGEDevice* device) { m_device = device; }

	virtual ResourceType::Enum getResourceType() const = 0;
	virtual void destroy() = 0;
	virtual bool isValid() const = 0;

  protected:
	SGEDevice* m_device = nullptr;
	int m_refcnt = 0;
};

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
struct Buffer : RAIResource {
	Buffer() = default;

	ResourceType::Enum getResourceType() const final { return ResourceType::Buffer; }

	virtual bool create(const BufferDesc& desc, const void* const pInitalData) = 0;
	virtual const BufferDesc& getDesc() const = 0;

	bool isConstantBuffer() const { return (getDesc().bindFlags & ResourceBindFlags::ConstantBuffer) != 0; }
	bool isVertexBuffer() const { return (getDesc().bindFlags & ResourceBindFlags::VertexBuffer) != 0; }
	bool isIndexBuffer() const { return (getDesc().bindFlags & ResourceBindFlags::IndexBuffer) != 0; }
};

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
struct FrameTarget : RAIResource {
	FrameTarget() = default;

	ResourceType::Enum getResourceType() const final { return ResourceType::FrameTarget; }

	// Sets render target and depth stencil elements can be NULL.
	virtual bool create(int numRenderTargets,
	                    Texture* renderTargets[],
	                    TargetDesc renderTargetDescs[],
	                    Texture* depthStencil,
	                    const TargetDesc& depthTargetDesc) = 0;

	virtual bool create() = 0;

	// Just a shortcut that makes a single 2D render target and optionally a 2D depth stencil texture.
	virtual bool create2D(int width,
	                      int height,
	                      TextureFormat::Enum renderTargetFmt = TextureFormat::R8G8B8A8_UNORM,
	                      TextureFormat::Enum depthTextureFmt = TextureFormat::D24_UNORM_S8_UINT) = 0;

	virtual void setRenderTarget(const int slot, Texture* texture, const TargetDesc& targetDesc) = 0;
	virtual void setDepthStencil(Texture* texture, const TargetDesc& targetDesc) = 0;

	virtual Texture* getRenderTarget(const unsigned int index) const = 0;
	virtual Texture* getDepthStencil() const = 0;

	virtual int getWidth() const = 0;
	virtual int getHeight() const = 0;
	float getRatioWH() const { return (float)getWidth() / (float)getHeight(); }

	virtual bool hasAttachment() const = 0;

	// Return the max viewport that will cover this frame target.
	Rect2s getViewport() const { return Rect2s(short(getWidth()), short(getHeight())); }

	//
	bool isSizeEqual(int const width, int const height) { return isValid() && (width == getWidth() && height == getHeight()); }

	bool isSizeEqual(const vec2f& size) { return isValid() && ((int)size.x == getWidth() && (int)size.y == getHeight()); }
};

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
struct SamplerState : public RAIResource {
	SamplerState() = default;

	ResourceType::Enum getResourceType() const final { return ResourceType::Sampler; }

	virtual bool create(const SamplerDesc& desc) = 0;
	virtual const SamplerDesc& getDesc() const = 0;
};

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
struct Texture : public RAIResource {
	Texture() = default;

	ResourceType::Enum getResourceType() const final { return ResourceType::Texture; }

	virtual bool create(const TextureDesc& desc, const TextureData initalData[], const SamplerDesc sampler = SamplerDesc()) = 0;

	virtual const TextureDesc& getDesc() const = 0;
	virtual SamplerState* getSamplerState() = 0;
	virtual void setSamplerState(SamplerState* ss) = 0;

	bool is2DWithSize(const int width, const int height) const {
		return getDesc().textureType == UniformType::Texture2D && getDesc().texture2D.width == width &&
		       getDesc().texture2D.height == height;
	}
};

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
struct Shader : public RAIResource {
	Shader() = default;

	ResourceType::Enum getResourceType() const final { return ResourceType::Shader; }

	// Creates the shader using the native language for the API.
	virtual bool createNative(const ShaderType::Enum type, const char* pCode, const char* const entryPoint) = 0;

	// Create the shader using the custom shading language.
	virtual bool create(const ShaderType::Enum type, const char* pCode, const char* preapendedCode = NULL) = 0;

	virtual const ShaderType::Enum getShaderType() const = 0;
};

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
struct ShadingProgram : public RAIResource {
	ShadingProgram() = default;

	ResourceType::Enum getResourceType() const final { return ResourceType::ShadingProgram; }

	virtual bool create(Shader* vertShdr, Shader* pixelShdr) = 0;
	virtual bool create(const char* const pVSCode, const char* const pPSCode, const char* const preAppendedCode = NULL) = 0;

	virtual Shader* getVertexShader() const = 0;
	virtual Shader* getPixelShader() const = 0;

	virtual const ShadingProgramRefl& getReflection() const = 0;
};

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
struct Query : public RAIResource {
	Query() = default;

	ResourceType::Enum getResourceType() const final { return ResourceType::Query; }

	virtual bool create(QueryType::Enum const type) = 0;
	virtual QueryType::Enum getType() const = 0;
};

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
struct RasterizerState : public RAIResource {
	RasterizerState() = default;

	ResourceType::Enum getResourceType() const final { return ResourceType::RasterizerState; }

	virtual bool create(const RasterDesc& desc) = 0;
	virtual const RasterDesc& getDesc() const = 0;
};

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
struct DepthStencilState : public RAIResource {
	DepthStencilState() = default;

	ResourceType::Enum getResourceType() const final { return ResourceType::DepthStencilState; }

	virtual bool create(const DepthStencilDesc& desc) = 0;
	virtual const DepthStencilDesc& getDesc() const = 0;
};

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
struct BlendState : public RAIResource {
	BlendState() = default;
	BlendState& operator=(const BlendState& ref) = default;

	ResourceType::Enum getResourceType() const final { return ResourceType::BlendState; }

	virtual bool create(const BlendStateDesc& desc) = 0;
	virtual const BlendStateDesc& getDesc() const = 0;
};

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
struct SGEDevice {
	struct StaticCaps {
		// True if UV coords origin of the currently selected rendering API is at the bottom left (example is OpenGL).
		bool uvBottomLeft;

		// True if the native NDC depth varies form -1 to 1.
		bool depthHomo;
	};

	static SGEDevice* create(const MainFrameTargetDesc& frameTargetDesc);

	// Returns statically determinated capabilites of the currently selected redering API.
	static const StaticCaps& staticCaps();

	// Returns the immediate context attached to this device.
	virtual SGEContext* getContext() = 0;

	template <typename T>
	T* requestResource();

	virtual RAIResource* requestResource(ResourceType::Enum const resourceType) = 0;

	virtual int getStringIndex(const std::string& str) = 0;

	virtual void present() = 0;

	virtual void resizeBackBuffer(int width, int height) = 0;
	virtual void setVsync(const bool enabled) = 0;
	virtual bool getVsync() const = 0;

	virtual FrameTarget* getWindowFrameTarget() = 0;

	virtual void releaseResource(RAIResource* resource) = 0;

	virtual RasterizerState* requestRasterizerState(const RasterDesc& desc) = 0;
	virtual DepthStencilState* requestDepthStencilState(const DepthStencilDesc& desc) = 0;
	virtual BlendState* requestBlendState(const BlendStateDesc& desc) = 0;

	virtual const FrameStatistics& getFrameStatistics() const = 0;

	// Vertex declaration caching used to speed up draw calls processing.
	virtual VertexDeclIndex getVertexDeclIndex(const VertexDecl* const declElems, const int declElemsCount) = 0;
	virtual const std::vector<VertexDecl>& getVertexDeclFromIndex(const VertexDeclIndex index) const = 0;
	virtual const std::map<std::vector<VertexDecl>, VertexDeclIndex>& getVertexDeclMap() const = 0;
};

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
struct SGEContext {
	SGEContext() = default;
	virtual ~SGEContext() = default;

	virtual SGEDevice* getDevice() = 0;

	virtual void executeDrawCall(DrawCall& drawCall,
	                             FrameTarget* frameTarget,
	                             const Rect2s* const pViewport = nullptr,
	                             const Rect2s* const pScissorsRect = nullptr) = 0;

	// Buffers.
	virtual void* map(Buffer* buffer, const Map::Enum map) = 0;
	virtual void unMap(Buffer* buffer) = 0;

	// Textures.
	virtual void updateTextureData(Texture* texture,  const TextureData& td) = 0;

	// Frame targets.
	virtual void clearColor(FrameTarget* target, int index, const float rgba[4]) = 0;
	virtual void clearDepth(FrameTarget* target, float depth) = 0;

	// Queries.
	virtual void beginQuery(Query* const query) = 0;
	virtual void endQuery(Query* const query) = 0;
	virtual bool isQueryReady(Query* const query) = 0;
	virtual bool getQueryData(Query* const query, uint64& queryData) = 0;
};

template <typename T>
struct GpuHandle {
	GpuHandle()
	    : m_pResourceInstance(NULL) {}


	template <typename S>
	GpuHandle(S* pInstance)
	    : m_pResourceInstance(static_cast<T*>(pInstance)) {
		ProxyIncrease();
	}

	~GpuHandle() {
		ProxyDecrease();
		m_pResourceInstance = NULL;
	}

	GpuHandle(const GpuHandle& other) {
		m_pResourceInstance = other.m_pResourceInstance;
		ProxyIncrease();
	}

	GpuHandle& operator=(const GpuHandle& other) {
		if (IsEqual(other) == false) {
			ProxyDecrease();
			m_pResourceInstance = other.m_pResourceInstance;
			ProxyIncrease();
		}

		return *this;
	}

	template <typename S>
	GpuHandle& operator=(S* ptr) {
		if (IsEqual(ptr) == false) {
			ProxyDecrease();
			m_pResourceInstance = static_cast<T*>(ptr);
			ProxyIncrease();
		}

		return *this;
	}

	void Release() { ProxyDecrease(); }
	bool HasResource() const { return m_pResourceInstance != NULL; }
	bool IsResourceValid() const { return HasResource() && m_pResourceInstance->isValid(); }

	bool IsEqual(RAIResource* pOther) const { return pOther == static_cast<RAIResource*>(m_pResourceInstance); }

	template <class S>
	bool IsEqual(const GpuHandle<S>& other) const {
		return static_cast<RAIResource*>(m_pResourceInstance) == static_cast<RAIResource*>(other.m_pResourceInstance);
	}

	T* operator->() { return m_pResourceInstance; }
	const T* operator->() const { return m_pResourceInstance; }
	operator T*() { return m_pResourceInstance; }
	T* GetPtr() const { return m_pResourceInstance; }
	T** PtrPtr() { return &m_pResourceInstance; }

	template <typename S>
	S* as() {
		return m_pResourceInstance ? static_cast<S*>(m_pResourceInstance) : NULL;
	}

	template <typename S>
	const S* as() const {
		return m_pResourceInstance ? static_cast<const S*>(m_pResourceInstance) : NULL;
	}

  private:
	void ProxyIncrease() {
		if (m_pResourceInstance != NULL) {
			m_pResourceInstance->addRef();
		}
	}

	void ProxyDecrease() {
		if (m_pResourceInstance != NULL) {
			m_pResourceInstance->releaseRef();

			if (m_pResourceInstance->getRefCount() == 0) {
				m_pResourceInstance->destroy(); // Manually call the "destructor".
				m_pResourceInstance->getDevice()->releaseResource(m_pResourceInstance);
			}

			m_pResourceInstance = NULL;
		}
	}

	T* m_pResourceInstance;
};

/// @brief RenderDestination is a pack of SGEContext FrameTarget and Viewport
/// Commonly used to specify where we want a particular @DrawCall to be executed.
struct RenderDestination {
	RenderDestination() = default;

	RenderDestination(SGEContext* const sgecon, FrameTarget* const frameTarget) {
		sgeAssert(sgecon && frameTarget);
		this->sgecon = sgecon;
		this->frameTarget = frameTarget;
		this->viewport = frameTarget->getViewport();
	}

	RenderDestination(SGEContext* const sgecon, FrameTarget* const frameTarget, const Rect2s& viewport) {
		sgeAssert(sgecon && frameTarget);
		this->sgecon = sgecon;
		this->frameTarget = frameTarget;
		this->viewport = viewport;
	}

	SGEDevice* getDevice() const {
		sgeAssert(sgecon);
		return sgecon->getDevice();
	}

	void executeDrawCall(DrawCall& dc, const Rect2s* const scissorsRect = nullptr) const {
		sgeAssert(sgecon && frameTarget);
		sgecon->executeDrawCall(dc, frameTarget, &viewport, scissorsRect);
	}

  public:
	SGEContext* sgecon = nullptr;
	FrameTarget* frameTarget = nullptr;
	Rect2s viewport;
};


} // namespace sge

#include "APICommands.h"
