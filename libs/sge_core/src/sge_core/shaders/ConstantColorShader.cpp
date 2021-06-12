#include "ConstantColorShader.h"
#include "sge_core/AssetLibrary.h"
#include "sge_core/ICore.h"
#include "sge_core/model/EvaluatedModel.h"
#include "sge_core/model/Model.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/utils/FileStream.h"
#include <sge_utils/math/mat4.h>

// Caution:
// this include is an exception do not include anything else like it.
#include "../core_shaders/FWDDefault_buildShadowMaps.h"
#include "../core_shaders/ShadeCommon.h"

using namespace sge;

//-----------------------------------------------------------------------------
// BasicModelDraw
//-----------------------------------------------------------------------------
void ConstantColorShader::drawGeometry(
    const RenderDestination& rdest, const mat4f& projView, const mat4f& world, const Geometry& geometry, const vec4f& shadingColor) {
	enum : int { OPT_HasVertexSkinning, kNumOptions };

	enum : int {
		uColor,
		uWorld,
		uProjView,
		uSkinningBones,
		uSkinningFirstBoneOffsetInTex,
	};

	if (shadingPermut.isValid() == false) {
		shadingPermut = ShadingProgramPermuator();

		const std::vector<OptionPermuataor::OptionDesc> compileTimeOptions = {
		    {OPT_HasVertexSkinning, "OPT_HasVertexSkinning", {SGE_MACRO_STR(kHasVertexSkinning_No), SGE_MACRO_STR(kHasVertexSkinning_Yes)}},
		};

		// clang-format off
		// Caution: It is important that the order of the elements here MATCHES the order in the enum above.
		const std::vector<ShadingProgramPermuator::Unform> uniformsToCache = {
		    {uColor, "uColor", ShaderType::PixelShader},
		    {uWorld, "uWorld", ShaderType::VertexShader},
		    {uProjView, "uProjView", ShaderType::VertexShader},
			{uSkinningBones, "uSkinningBones", ShaderType::VertexShader},
			{uSkinningFirstBoneOffsetInTex, "uSkinningFirstBoneOffsetInTex", ShaderType::VertexShader},
		};
		// clang-format on

		SGEDevice* const sgedev = rdest.getDevice();
		shadingPermut->createFromFile(sgedev, "core_shaders/ConstantColor.shader", compileTimeOptions, uniformsToCache);
	}

	const int optHasVertexSkinning = (geometry.hasVertexSkinning()) ? kHasVertexSkinning_Yes : kHasVertexSkinning_No;

	const OptionPermuataor::OptionChoice optionChoice[kNumOptions] = {
	    {OPT_HasVertexSkinning, optHasVertexSkinning},
	};

	const int iShaderPerm = shadingPermut->getCompileTimeOptionsPerm().computePermutationIndex(optionChoice, SGE_ARRSZ(optionChoice));
	const ShadingProgramPermuator::Permutation& shaderPerm = shadingPermut->getShadersPerPerm()[iShaderPerm];

	DrawCall dc;

	stateGroup.setProgram(shaderPerm.shadingProgram.GetPtr());
	stateGroup.setVBDeclIndex(geometry.vertexDeclIndex);
	stateGroup.setVB(0, geometry.vertexBuffer, uint32(geometry.vbByteOffset), geometry.stride);
	stateGroup.setPrimitiveTopology(geometry.topology);
	if (geometry.ibFmt != UniformType::Unknown) {
		stateGroup.setIB(geometry.indexBuffer, geometry.ibFmt, geometry.ibByteOffset);
	} else {
		stateGroup.setIB(nullptr, UniformType::Unknown, 0);
	}

	const bool flipCulling = determinant(world) < 0.f;

	RasterizerState* const rasterState =
	    flipCulling ? getCore()->getGraphicsResources().RS_wireframeBackfaceCCW : getCore()->getGraphicsResources().RS_defaultWireframe;
	stateGroup.setRenderState(rasterState, getCore()->getGraphicsResources().DSS_default_lessEqual);

	StaticArray<BoundUniform, 24> uniforms;
	shaderPerm.bind<24>(uniforms, uWorld, (void*)&world);
	shaderPerm.bind<24>(uniforms, uProjView, (void*)&projView);
	shaderPerm.bind<24>(uniforms, uColor, (void*)&shadingColor);

	if (optHasVertexSkinning == kHasVertexSkinning_Yes) {
		uniforms.push_back(BoundUniform(shaderPerm.uniformLUT[uSkinningBones], (geometry.skinningBoneTransforms)));
		sgeAssert(uniforms.back().bindLocation.isNull() == false && uniforms.back().bindLocation.uniformType != 0);
		uniforms.push_back(BoundUniform(shaderPerm.uniformLUT[uSkinningFirstBoneOffsetInTex], (void*)&geometry.firstBoneOffset));
		sgeAssert(uniforms.back().bindLocation.isNull() == false && uniforms.back().bindLocation.uniformType != 0);
	}

	// Lights and draw call.
	dc.setUniforms(uniforms.data(), uniforms.size());
	dc.setStateGroup(&stateGroup);

	if (geometry.ibFmt != UniformType::Unknown) {
		dc.drawIndexed(geometry.numElements, 0, 0);
	} else {
		dc.draw(geometry.numElements, 0);
	}

	rdest.sgecon->executeDrawCall(dc, rdest.frameTarget, &rdest.viewport);
}

void ConstantColorShader::draw(const RenderDestination& rdest,
                               const mat4f& projView,
                               const mat4f& preRoot,
                               const EvaluatedModel& evalModel,
                               const vec4f& shadingColor) {
	for (int iNode = 0; iNode < evalModel.getNumEvalNodes(); ++iNode) {
		const EvaluatedNode& evalNode = evalModel.getEvalNode(iNode);
		const ModelNode* rawNode = evalModel.m_model->nodeAt(iNode);

		for (int iMesh = 0; iMesh < rawNode->meshAttachments.size(); ++iMesh) {
			const MeshAttachment& meshAttachment = rawNode->meshAttachments[iMesh];
			const EvaluatedMesh& evalMesh = evalModel.getEvalMesh(meshAttachment.attachedMeshIndex);
			mat4f const finalTrasform = (evalMesh.geometry.hasVertexSkinning()) ? preRoot : preRoot * evalNode.evalGlobalTransform;

			drawGeometry(rdest, projView, finalTrasform, evalMesh.geometry, shadingColor);
		}
	}
}
