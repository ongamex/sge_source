#include "SkyShader.h"
#include "sge_core/GeomGen.h"
#include "sge_core/ICore.h"
#include "sge_utils/math/Frustum.h"

namespace sge {

struct SkyShaderCBufferParams {
	mat4f uView;
	mat4f uProj;
	mat4f uWorld;
	vec3f uColorBottom;
	float uColorBottom_padding;
	vec3f uColorTop;
	float uColorTop_padding;
};

void SkyShader::draw(const RenderDestination& rdest, const vec3f&, const mat4f view, const mat4f proj, Texture* skyTexture) {
	SGEDevice* const sgedev = rdest.getDevice();

	enum : int {
		uParamsCb_vertex,
		uParamsCb_pixel,
		uSkyTexture,

	};

	if (shadingPermut.isValid() == false) {
		shadingPermut = ShadingProgramPermuator();

		const std::vector<OptionPermuataor::OptionDesc> compileTimeOptions = {};

		const std::vector<ShadingProgramPermuator::Unform> uniformsToCache = {
		    {uParamsCb_vertex, "SkyShaderCBufferParams", ShaderType::VertexShader},
		    {uParamsCb_pixel, "SkyShaderCBufferParams", ShaderType::PixelShader},
		    {uSkyTexture, "uSkyTexture", ShaderType::PixelShader},
		};

		shadingPermut->createFromFile(sgedev, "core_shaders/SkyGradient.shader", compileTimeOptions, uniformsToCache);
	}

	if (!cbParms.IsResourceValid()) {
		BufferDesc bd = BufferDesc::GetDefaultConstantBuffer(256, ResourceUsage::Dynamic);
		cbParms = sgedev->requestResource<Buffer>();
		cbParms->create(bd, nullptr);
	}

	if (m_skySphereVB.IsResourceValid() == false) {
		m_skySphereVB = getCore()->getDevice()->requestResource<Buffer>();
		m_skySphereNumVerts = GeomGen::sphere(m_skySphereVB.GetPtr(), 16, 32);
		VertexDecl vdecl[] = {VertexDecl(0, "a_position", UniformType::Float3, 0)};
		m_skySphereVBVertexDeclIdx = getCore()->getDevice()->getVertexDeclIndex(vdecl, SGE_ARRSZ(vdecl));
	}

	const int iShaderPerm = shadingPermut->getCompileTimeOptionsPerm().computePermutationIndex(nullptr, 0);
	const ShadingProgramPermuator::Permutation& shaderPerm = shadingPermut->getShadersPerPerm()[iShaderPerm];


	mat4f prjInv = proj.inverse();
	vec4f left = mat_mul_vec(prjInv, vec4f(-1.f, 0.f, 0.f, 1.f));
	vec4f right = mat_mul_vec(prjInv, vec4f(1.f, 0.f, 0.f, 1.f));

	left = left / left.w;
	right = right / right.w;

	float s = fabsf(right.x - left.x);

	SkyShaderCBufferParams cbParamsData;
	cbParamsData.uView = view;
	cbParamsData.uProj = proj;
	cbParamsData.uWorld = mat4f::getScaling(s);
	cbParamsData.uColorTop = vec3f(0.75f);
	cbParamsData.uColorBottom = vec3f(0.1f);

	if (void* const cbParamsMapped = rdest.sgecon->map(cbParms, Map::WriteDiscard)) {
		memcpy(cbParamsMapped, &cbParamsData, sizeof(cbParamsData));
		rdest.sgecon->unMap(cbParms);
	}

	StaticArray<BoundUniform, 12> uniforms;

	shaderPerm.bind(uniforms, uParamsCb_vertex, cbParms.GetPtr());
	shaderPerm.bind(uniforms, uParamsCb_pixel, cbParms.GetPtr());
	shaderPerm.bind(uniforms, uSkyTexture, skyTexture);

	stateGroup.setProgram(shaderPerm.shadingProgram.GetPtr());
	stateGroup.setPrimitiveTopology(PrimitiveTopology::TriangleList);
	stateGroup.setVBDeclIndex(m_skySphereVBVertexDeclIdx);
	stateGroup.setVB(0, m_skySphereVB, 0, sizeof(vec3f));

	RasterizerState* const rasterState = getCore()->getGraphicsResources().RS_noCulling;
	stateGroup.setRenderState(rasterState, getCore()->getGraphicsResources().DSS_default_lessEqual);

	DrawCall dc;
	dc.setStateGroup(&stateGroup);
	dc.setUniforms(uniforms.data(), uniforms.size());
	dc.draw(m_skySphereNumVerts, 0);

	rdest.sgecon->executeDrawCall(dc, rdest.frameTarget, &rdest.viewport);
}

}; // namespace sge
