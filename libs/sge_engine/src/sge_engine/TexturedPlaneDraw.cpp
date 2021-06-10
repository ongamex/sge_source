#include "sge_core/ICore.h"

#include "sge_engine/TexturedPlaneDraw.h"

const char* const EFFECT_TEXTURED_PLANE = R"(
uniform float4x4 projViewWorld;
uniform float4 tint;
uniform float4 uvRegion;
uniform sampler2D texDiffuse;

struct VS_INPUT {
	float3 a_position : a_position;
	float2 a_uv : a_uv;
};

struct VS_OUTPUT { 
    float2 v_uv : v_uv;
    float4 SV_Position : SV_Position;
};

//----------------------------------------
// Vertex Shader
//----------------------------------------
VS_OUTPUT vsMain(VS_INPUT IN) {
	VS_OUTPUT OUT;

	const float4 worldPos = mul(projViewWorld, float4(IN.a_position, 1.0));

	OUT.SV_Position = worldPos;
	OUT.v_uv = IN.a_uv;

	return OUT;
}

//----------------------------------------
// Pixel Shader
//----------------------------------------
float4 psMain(VS_OUTPUT IN) : COLOR {
	const float2 uv = lerp(uvRegion.xy, uvRegion.zw, IN.v_uv);


	const float4 c0 = tint * tex2D(texDiffuse, uv);
	if (c0.w==0) {
		discard;
	}

	return c0;
}

)";

namespace sge {

void TexturedPlaneDraw::draw(
    const RenderDestination& rdest, const mat4f& projViewWorld, Texture* texture, const vec4f& tint, const vec4f uvRegion) {
	initialize(rdest.getDevice());

	m_stateGroup.setProgram(m_shadingProgram);
	m_stateGroup.setPrimitiveTopology(PrimitiveTopology::TriangleList);
	m_stateGroup.setVBDeclIndex(m_vertexDecl);
	m_stateGroup.setVB(0, m_vertexBuffer, 0, sizeof(Vertex));
	m_stateGroup.setRenderState(getCore()->getGraphicsResources().RS_noCulling, getCore()->getGraphicsResources().DSS_default_lessEqual,
	                            getCore()->getGraphicsResources().BS_backToFrontAlpha);

	BoundUniform uniforms[] = {
	    BoundUniform(m_texDiffuse_bindLoc, texture),
#ifdef SGE_RENDERER_D3D11
	    BoundUniform(m_texDiffuseSampler_bindLoc, texture->getSamplerState()),
#endif
	    BoundUniform(m_projViewWorld_bindLoc, (void*)&projViewWorld),
	    BoundUniform(m_tint_bindLoc, (void*)&tint),
	    BoundUniform(m_uvRegion_bindLoc, (void*)&uvRegion),
	};

	DrawCall dc;

	dc.setUniforms(uniforms, SGE_ARRSZ(uniforms));
	dc.setStateGroup(&m_stateGroup);
	dc.draw(6, 0);

	rdest.sgecon->executeDrawCall(dc, rdest.frameTarget, &rdest.viewport);
}

void TexturedPlaneDraw::initialize(SGEDevice* sgedev) {
	if (m_isInitialized == false) {
		m_isInitialized = true;

		const VertexDecl vertexDeclDesc[] = {
		    {0, "a_position", UniformType::Float3, 0},
		    {0, "a_normal", UniformType::Float3, 12},
		    {0, "a_uv", UniformType::Float2, 24},
		};

		m_vertexDecl = sgedev->getVertexDeclIndex(vertexDeclDesc, SGE_ARRSZ(vertexDeclDesc));

		Vertex vertices[6] = {
		    // Bottom-right triangle.
		    {vec3f(0.f, 0.f, 1.f), vec3f(1.f, 0.f, 0.f), vec2f(0.f, 1.f)},
		    {vec3f(0.f, 0.f, 0.f), vec3f(1.f, 0.f, 0.f), vec2f(1.f, 1.f)},
		    {vec3f(0.f, 1.f, 0.f), vec3f(1.f, 0.f, 0.f), vec2f(1.f, 0.f)},
		    // Top-left triangle.
		    {vec3f(0.f, 0.f, 1.f), vec3f(1.f, 0.f, 0.f), vec2f(0.f, 1.f)},
		    {vec3f(0.f, 1.f, 0.f), vec3f(1.f, 0.f, 0.f), vec2f(1.f, 0.f)},
		    {vec3f(0.f, 1.f, 1.f), vec3f(1.f, 0.f, 0.f), vec2f(0.f, 0.f)},
		};

		m_vertexBuffer = sgedev->requestResource<Buffer>();
		m_vertexBuffer->create(BufferDesc::GetDefaultVertexBuffer(sizeof(vertices)), vertices);

		m_shadingProgram = sgedev->requestResource<ShadingProgram>();
		m_shadingProgram->create(EFFECT_TEXTURED_PLANE, EFFECT_TEXTURED_PLANE);

		const ShadingProgramRefl& refl = m_shadingProgram->getReflection();

		m_texDiffuse_bindLoc = refl.textures.findUniform("texDiffuse", ShaderType::PixelShader);
#ifdef SGE_RENDERER_D3D11
		m_texDiffuseSampler_bindLoc = refl.samplers.findUniform("texDiffuse_sampler", ShaderType::PixelShader);
#endif
		m_projViewWorld_bindLoc = refl.numericUnforms.findUniform("projViewWorld", ShaderType::VertexShader);
		m_tint_bindLoc = refl.numericUnforms.findUniform("tint", ShaderType::PixelShader);
		m_uvRegion_bindLoc = refl.numericUnforms.findUniform("uvRegion", ShaderType::PixelShader);
	}
}

Geometry TexturedPlaneDraw::getGeometry(SGEDevice* sgedev) {
	initialize(sgedev);

	Geometry geom(m_vertexBuffer.GetPtr(), nullptr, nullptr, -1, m_vertexDecl, false, true, true, false, PrimitiveTopology::TriangleList, 0, 0,
	              sizeof(Vertex), UniformType::Unknown, 6);

	return geom;
}

Material TexturedPlaneDraw::getMaterial(Texture* texture) const {
	Material mtl;
	mtl.diffuseTexture = texture;
	return mtl;
}

} // namespace sge
