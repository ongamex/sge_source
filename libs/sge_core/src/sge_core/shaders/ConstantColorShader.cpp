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
	enum : int {
		uColor,
		uWorld,
		uProjView,
	};

	if (shadingPermut.isValid() == false) {
		shadingPermut = ShadingProgramPermuator();

		static const std::vector<OptionPermuataor::OptionDesc> compileTimeOptions = {};

		// clang-format off
		// Caution: It is important that the order of the elements here MATCHES the order in the enum above.
		static const std::vector<ShadingProgramPermuator::Unform> uniformsToCache = {
		    {uColor, "uColor"},
		    {uWorld, "uWorld"},
		    {uProjView, "uProjView"},
		};
		// clang-format on

		SGEDevice* const sgedev = rdest.getDevice();
		shadingPermut->createFromFile(sgedev, "core_shaders/ConstantColor.shader", compileTimeOptions, uniformsToCache);
	}

	const int iShaderPerm = shadingPermut->getCompileTimeOptionsPerm().computePermutationIndex(nullptr, 0);
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
	stateGroup.setRenderState(rasterState, getCore()->getGraphicsResources().DSS_default_lessEqual_noWrite);

	StaticArray<BoundUniform, 24> uniforms;
	shaderPerm.bind<24>(uniforms, uWorld, (void*)&world);
	shaderPerm.bind<24>(uniforms, uProjView, (void*)&projView);
	shaderPerm.bind<24>(uniforms, uColor, (void*)&shadingColor);

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

void ConstantColorShader::draw(
    const RenderDestination& rdest, const mat4f& projView, const mat4f& preRoot, const EvaluatedModel& evalModel, const vec4f& shadingColor) {
	for (int iNode = 0; iNode < evalModel.getNumEvalNodes(); ++iNode) {
		const EvaluatedNode& evalNode = evalModel.getEvalNode(iNode);
		const Model::Node* rawNode = evalModel.m_model->nodeAt(iNode);

		for (int iMesh = 0; iMesh < rawNode->meshAttachments.size(); ++iMesh) {
			const Model::MeshAttachment& meshAttachment = rawNode->meshAttachments[iMesh];
			const EvaluatedMesh& mesh = evalModel.getEvalMesh(meshAttachment.attachedMeshIndex);
			mat4f const finalTrasform = (mesh.boneTransformMatrices.size() == 0) ? preRoot * evalNode.evalGlobalTransform : preRoot;

			drawGeometry(rdest, projView, finalTrasform, mesh.geom, shadingColor);
		}
	}
}
