#include "DebugDraw.h"

namespace sge {

const char EFFECT_3D_VERTEX_COLOR[] = R"(
//----------------------------------------
//
//----------------------------------------
uniform float4x4 projViewWorld;

struct VERTEX_IN {
	float3 a_position : a_position;
	float4 a_color : a_color;
};


struct VERTEX_OUT {
	float4 SV_Position : SV_Position;
	float4 v_color : v_color;
};

VERTEX_OUT vsMain(VERTEX_IN IN)
{
	VERTEX_OUT OUT;

	const float4 worldPos = mul(projViewWorld, float4(IN.a_position, 1.0));
	OUT.v_color = IN.a_color;
	OUT.SV_Position = worldPos;

	return OUT;
}

//----------------------------------------
//
//----------------------------------------
float4 psMain(VERTEX_OUT IN) : COLOR {
	return IN.v_color;
}
)";

//------------------------------------------------------------------------------------------
// WiredCommandData
//------------------------------------------------------------------------------------------
void DebugDraw::WiredCommandData::line(const vec3f& a, const vec3f& b, const int rgba) {
	m_verts.push_back(GeomGen::PosColorVert(a, rgba));
	m_verts.push_back(GeomGen::PosColorVert(b, rgba));
}

void DebugDraw::WiredCommandData::box(const mat4f& world, const int rgba) {
	GeomGen::wiredBox(m_verts, world, rgba);
}

void DebugDraw::WiredCommandData::box(const AABox3f& aabb, const int rgba) {
	GeomGen::wiredBox(m_verts, aabb, rgba);
}

void DebugDraw::WiredCommandData::box(const mat4f& world, const AABox3f& aabb, const int rgba) {
	const size_t newBoxStart = m_verts.size();

	GeomGen::wiredBox(m_verts, aabb, rgba);
	for (size_t t = newBoxStart; t < m_verts.size(); ++t) {
		// m_verts[t].pt = (world * vec4f(m_verts[t].pt, 1.f)).xyz();
		m_verts[t].pt = mat_mul_pos(world, m_verts[t].pt);
	}
}

void DebugDraw::WiredCommandData::capsule(const mat4f& world, const int rgba, float height, float radius, int numSides) {
	GeomGen::wiredCapsule(m_verts, world, rgba, height, radius, numSides, GeomGen::center);
}

void DebugDraw::WiredCommandData::sphere(const mat4f& world, const int rgba, float radius, int numSides) {
	GeomGen::wiredSphere(m_verts, world, rgba, radius, numSides);
}

void DebugDraw::WiredCommandData::cylinder(const mat4f& world, const int rgba, float height, float radius, int numSides) {
	GeomGen::wiredCylinder(m_verts, world, rgba, height, radius, numSides, GeomGen::center);
}

void DebugDraw::WiredCommandData::basis(const mat4f& world) {
	GeomGen::wiredBasis(m_verts, world);
}

void DebugDraw::WiredCommandData::grid(
    const vec3f& origin, const vec3f& xAxis, const vec3f& zAxis, const int xLines, const int yLines, const int color) {
	GeomGen::wiredGrid(m_verts, origin, xAxis, zAxis, xLines, yLines, color);
}

//------------------------------------------------------------------------------------------
// WiredCommandData
//------------------------------------------------------------------------------------------
void DebugDraw::initialze(SGEDevice* sgedev) {
	if (isInitialized)
		return;
	isInitialized = true;

	// Initialize the uniform string indices.
	m_projViewWorld_strIdx = sgedev->getStringIndex("projViewWorld");

	//
	const VertexDecl vtxDecl_pos3d_rgba_int[] = {
	    {0, "a_position", UniformType::Float3, 0},
	    {0, "a_color", UniformType::Int_RGBA_Unorm_IA, 12},
	};

	m_vertexDeclIndex_pos3d_rgba_int = sgedev->getVertexDeclIndex(vtxDecl_pos3d_rgba_int, SGE_ARRSZ(vtxDecl_pos3d_rgba_int));
	;

	//
	m_shaderSolidVertexColor = sgedev->requestResource<ShadingProgram>();
	m_shaderSolidVertexColor->createFromCustomHLSL(EFFECT_3D_VERTEX_COLOR, EFFECT_3D_VERTEX_COLOR);

	//
	m_vertexBuffer = sgedev->requestResource<Buffer>();
	BufferDesc bd = BufferDesc::GetDefaultVertexBuffer(sizeof(GeomGen::PosColorVert) * 10000, ResourceUsage::Dynamic);
	m_vertexBuffer->create(bd, nullptr);
}

void DebugDraw::draw(const RenderDestination& rdest, const mat4f& projView) {
	for (auto& grpItr : m_groups) {
		drawWieredCommand(rdest, projView, grpItr.second.getWiered());
	}
}

void DebugDraw::drawWieredCommand(const RenderDestination& rdest, const mat4f& projView, const WiredCommandData& cmd) {
	const std::vector<GeomGen::PosColorVert> verts = cmd.getVerts();

	if (verts.size() == 0)
		return;

	sgeAssert(verts.size() % 2 == 0);
	if (m_vertexBuffer.IsResourceValid() == false) {
		sgeAssert(false);
		return;
	}

	// Set-up the draw call
	BoundUniform uniforms[] = {
	    BoundUniform(m_shaderSolidVertexColor->getReflection().numericUnforms.findUniform("projViewWorld", ShaderType::VertexShader), (void*)&projView),
	};

	m_stateGroup.setProgram(m_shaderSolidVertexColor);
	m_stateGroup.setVB(0, m_vertexBuffer, 0, sizeof(GeomGen::PosColorVert));
	m_stateGroup.setVBDeclIndex(m_vertexDeclIndex_pos3d_rgba_int);
	m_stateGroup.setPrimitiveTopology(PrimitiveTopology::LineList);

	//
	int maxVertsCntInCall = int(m_vertexBuffer->getDesc().sizeBytes / sizeof(GeomGen::PosColorVert));
	if (maxVertsCntInCall % 2) {
		maxVertsCntInCall--;
	}

	int idxUnprocessed = 0;
	while (idxUnprocessed < verts.size()) {
		int const totalUnprocessedVertices = int(verts.size()) - idxUnprocessed;
		int const numVertsToProcess = std::min(maxVertsCntInCall, totalUnprocessedVertices);

		GeomGen::PosColorVert* const vbdata = (GeomGen::PosColorVert*)rdest.sgecon->map(m_vertexBuffer, Map::WriteDiscard);
		std::copy(verts.begin() + idxUnprocessed, verts.begin() + idxUnprocessed + numVertsToProcess, vbdata);
		rdest.sgecon->unMap(m_vertexBuffer);

		DrawCall dc;

		dc.setUniforms(uniforms, SGE_ARRSZ(uniforms));
		dc.setStateGroup(&m_stateGroup);
		dc.draw(numVertsToProcess, 0);

		rdest.sgecon->executeDrawCall(dc, rdest.frameTarget, &rdest.viewport);

		idxUnprocessed += numVertsToProcess;
	}
}

} // namespace sge
