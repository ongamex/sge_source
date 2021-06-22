// Assumes "renderer.h" included.

#pragma once

#include <string.h>
#include <string>

#include "sge_renderer/renderer/GraphicsCommon.h"
#include "sge_utils/utils/Pair.h"

namespace sge {

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
struct CBufferRefl;
struct CBufferFiller {
	struct ValueDesc {
		unsigned nameStrIdx = 0;
		UniformType::Enum type = UniformType::Unknown;
		size_t offsetBytes = 0;
	};

	void setFloat4(const unsigned nameStrIdx, const float v[4]) { SetData(nameStrIdx, UniformType::Float4, v, sizeof(float) * 4); }

	void setFloat4x4(const unsigned nameStrIdx, const float v[16]) { SetData(nameStrIdx, UniformType::Float4x4, v, sizeof(float) * 16); }

	ValueDesc& getValueDesc(const unsigned nameStrIdx, const UniformType::Enum type);
	void DeleteValue(const int idx);

	void updateCBuffer(SGEContext* context, Buffer* cbuffer, const CBufferFiller& cbRefl);

	void clear() {
		values.clear();
		data.clear();
	}

	void SetData(const unsigned nameStrIdx, const UniformType::Enum type, const void* data, const int inDataSizeBytes);

	std::vector<ValueDesc> values;
	std::vector<char> data;
};

//----------------------------------------------------------------------------
// DrawExecDesc
//----------------------------------------------------------------------------
struct DrawExecDesc {
	DrawExecDesc()
	    : m_type(Type_Invalid) {}

	enum Type : char {
		Type_Invalid,
		Type_Linear,
		Type_Indexed,
	};

	// https://msdn.microsoft.com/en-us/library/windows/desktop/ff476898(v=vs.85).aspx
	struct Linear {
		uint32 numVerts;
		uint32 startVert;
		uint32 numInstances;
	};

	struct Indexed {
		uint32 numIndices;
		uint32 startIndex;
		uint32 startVertex; // A value added to each index before reading a vertex from the vertex buffer.
		uint32 numInstances;
	};

	void Draw(const uint32 numVerts, const uint32 startVert, const uint32 instanceCount) {
		m_type = Type_Linear;

		m_linear.numVerts = numVerts;
		m_linear.startVert = startVert;
		m_linear.numInstances = instanceCount;
	}

	void DrawIndexed(const uint32 numIndices, const uint32 startIndex, const uint32 startVertex, const uint32 instanceCount) {
		m_type = Type_Indexed;

		m_indexed.numIndices = numIndices;
		m_indexed.startIndex = startIndex;
		m_indexed.startVertex = startVertex;
		m_indexed.numInstances = instanceCount;
	}

	bool IsValid() const { return m_type != Type_Invalid; }
	Type GetType() const { return m_type; }

	const Linear& LinearCall() const { return m_linear; }
	const Indexed& IndexedCall() const { return m_indexed; }

	union {
		Linear m_linear;
		Indexed m_indexed;
	};

	Type m_type;
};

struct StateGroup {
	StateGroup() = default;

	void setProgram(ShadingProgram* pShadingProgram);
	void setVB(const int slot, Buffer* pBuffer, const uint32 byteOffset, const uint32 stride);
	void setVBDeclIndex(const VertexDeclIndex idx);
	void setPrimitiveTopology(const PrimitiveTopology::Enum pt);
	void setIB(Buffer* pBuffer, const UniformType::Enum format, const uint32 byteOffset);

	void setRasterizerState(RasterizerState* state) { m_rasterState = state; }
	void setDepthStencilState(DepthStencilState* state) { m_depthStencilState = state; }

	void setRenderState(RasterizerState* rasterState, DepthStencilState* depthStencilState, BlendState* blendState = nullptr);

	// Bound geometry.
	Buffer* m_vertexBuffers[GraphicsCaps::kVertexBufferSlotsCount] = {};
	uint32 m_vbOffsets[GraphicsCaps::kVertexBufferSlotsCount] = {};
	uint32 m_vbStrides[GraphicsCaps::kVertexBufferSlotsCount] = {};
	VertexDeclIndex m_vertDeclIndex = VertexDeclIndex_Null;
	PrimitiveTopology::Enum m_primTopology = PrimitiveTopology::Unknown;
	Buffer* m_indexBuffer = nullptr;
	UniformType::Enum m_indexBufferFormat = UniformType::Unknown;
	uint32 m_indexBufferByteOffset;

	// Pipeline state.
	ShadingProgram* m_shadingProg = nullptr;
	RasterizerState* m_rasterState = nullptr;
	DepthStencilState* m_depthStencilState = nullptr;
	BlendState* m_blendState = nullptr;
};

struct BoundUniform {
	BoundUniform()
	    : data(nullptr) {}

	BoundUniform(BindLocation bindLocation, void* data)
	    : bindLocation(bindLocation)
	    , data(data) {}

	BoundUniform(BindLocation bindLocation, Texture** textures)
	    : bindLocation(bindLocation)
	    , textures(textures) {}

	BoundUniform(BindLocation bindLocation, Buffer* buffer)
	    : bindLocation(bindLocation)
	    , buffer(buffer) {}

	BindLocation bindLocation;
	union {
		void* data;
		Texture* texture;
		Texture** textures;
		Buffer* buffer;
		SamplerState* sampler;
		SamplerState** samplers;
	};
};


//----------------------------------------------------------------------------
// DrawCall
//----------------------------------------------------------------------------
struct DrawCall {
	DrawExecDesc m_drawExec;
	StateGroup* m_pStateGroup = nullptr;
	BoundUniform* uniforms = nullptr;
	int numUniforms = 0;

	void setUniforms(BoundUniform* const uniformsNew, int const numUniformsNew) {
		this->uniforms = uniformsNew;
		this->numUniforms = numUniformsNew;
	}

	void setStateGroup(StateGroup* const stateGroup) { m_pStateGroup = stateGroup; }

	void draw(const uint32 numVerts, const uint32 startVert, const uint32 numInstances = 1);
	void drawIndexed(const uint32 numIndices, const uint32 startIndex, const uint32 startVert, const uint32 numInstances = 1);

#if 0
	// This functions checks the validity of the bound resources
	// and returns true if the draw call is valid-ish...
	// [CAUTION] this function is NOT complete.

	bool ValidateDrawCall() const;
#endif
};

//----------------------------------------------------------------------------
// There are currently used only for SGERecordingContext.
// [TODO] Consider making them the offical way of doing anything?
//----------------------------------------------------------------------------
struct BufferMapCmd {
	Buffer* buffer = nullptr;
	char* data = nullptr; // There is no guarantee who owns the data.
};

struct ClearColorCmd {
	ClearColorCmd() = default;
	ClearColorCmd(FrameTarget* frameTarget, int index, const float rgba[4]);

	FrameTarget* m_frameTarget = nullptr;
	int m_index = -1;
	float m_rgba[4];
};

struct ClearDepthStencilCmd {
	ClearDepthStencilCmd() = default;
	ClearDepthStencilCmd(FrameTarget* frameTarget, float depth);

	FrameTarget* m_frameTarget = nullptr;
	float m_depth = 1.f;
};

} // namespace sge
