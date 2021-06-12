#include "sge_engine/DefaultGameDrawer.h"
#include "sge_core/DebugDraw.h"
#include "sge_core/ICore.h"
#include "sge_core/QuickDraw.h"
#include "sge_engine/GameInspector.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/IWorldScript.h"
#include "sge_engine/Physics.h"
#include "sge_engine/actors/ABlockingObstacle.h"
#include "sge_engine/actors/ACRSpline.h"
#include "sge_engine/actors/ACamera.h"
#include "sge_engine/actors/AInfinitePlaneObstacle.h"
#include "sge_engine/actors/AInvisibleRigidObstacle.h"
#include "sge_engine/actors/ALight.h"
#include "sge_engine/actors/ALine.h"
#include "sge_engine/actors/ALocator.h"
#include "sge_engine/actors/ANavMesh.h"
#include "sge_engine/materials/Material.h"
#include "sge_engine/traits/TraitModel.h"
#include "sge_engine/traits/TraitMultiModel.h"
#include "sge_engine/traits/TraitParticles.h"
#include "sge_engine/traits/TraitRenderableGeom.h"
#include "sge_engine/traits/TraitTexturedPlane.h"
#include "sge_engine/traits/TraitViewportIcon.h"
#include "sge_utils/math/Frustum.h"
#include "sge_utils/math/color.h"
#include "sge_utils/utils/FileStream.h"

// Caution:
// this include is an exception do not include anything else like it.
#include "../core_shaders/ShadeCommon.h"
//#include "../shaders/FWDDefault_buildShadowMaps.h"

namespace sge {

static inline const vec4f kPrimarySelectionColor = vec4f(0.25f, 1.f, 0.63f, 1.f);
static inline const vec4f kSecondarySelectionColor = colorFromIntRgba(245, 158, 66, 255);

// Actors forward declaration
struct AInvisibleRigidObstacle;

void DefaultGameDrawer::prepareForNewFrame() {
	m_shadingLights.clear();

	if (m_skySphereVB.IsResourceValid() == false) {
		m_skySphereVB = getCore()->getDevice()->requestResource<Buffer>();
		m_skySphereNumVerts = GeomGen::sphere(m_skySphereVB.GetPtr(), 8, 8);

		VertexDecl vdecl[] = {VertexDecl(0, "a_position", UniformType::Float3, 0)};

		m_skySphereVBVertexDeclIdx = getCore()->getDevice()->getVertexDeclIndex(vdecl, SGE_ARRSZ(vdecl));
	}

	if (m_skyGradientShader.IsResourceValid() == false) {
		m_skyGradientShader = getCore()->getDevice()->requestResource<ShadingProgram>();
		std::string code;
		if (FileReadStream::readTextFile("core_shaders/SkyGradient.shader", code)) {
			m_skyGradientShader->create(code.c_str(), code.c_str());
		}
	}
}

void DefaultGameDrawer::updateShadowMaps(const GameDrawSets& drawSets) {
	const std::vector<GameObject*>* const allLights = getWorld()->getObjects(sgeTypeId(ALight));
	if (allLights == nullptr) {
		return;
	}

	ICamera* const gameCamera = drawSets.gameCamera;
	const Frustum* const gameCameraFrustumWs = drawSets.gameCamera->getFrustumWS();

	// Compute All shadow maps.
	for (const GameObject* const actorLight : *allLights) {
		const ALight* const light = static_cast<const ALight*>(actorLight);
		const LightDesc& lightDesc = light->getLightDesc();

		const bool isPointLight = lightDesc.type == light_point;

		LightShadowInfo& lsi = this->m_perLightShadowFrameTarget[light->getId()];
		lsi.isCorrectlyUpdated = false;

		if (lightDesc.isOn == false) {
			continue;
		}

		// if the light is not inside the view frsutum do no update its shadow map.
		// If the camera frustum is present, try to clip the object.
		if (gameCameraFrustumWs != nullptr) {
			const AABox3f bboxOS = light->getBBoxOS();
			if (!bboxOS.IsEmpty()) {
				if (gameCameraFrustumWs->isObjectOrientedBoxOutside(bboxOS, light->getTransform().toMatrix())) {
					continue;
				}
			}
		}

		// Retrieve the ShadowMapBuildInfo which tells us the cameras to be used for rendering the shadow map.
		Optional<ShadowMapBuildInfo> shadowMapBuildInfoOpt = lightDesc.buildShadowMapInfo(light->getTransform(), *gameCameraFrustumWs);
		if (shadowMapBuildInfoOpt.isValid() == false) {
			continue;
		}

		// Sanity check.
		if (shadowMapBuildInfoOpt->isPointLight != isPointLight) {
			sgeAssert(false);
			continue;
		}

		lsi.buildInfo = shadowMapBuildInfoOpt.get();

		// Create the appropriatley sized frame target for the shadow map.
		if (isPointLight) {
			// No regular shadow maps are needed so relese them.
			lsi.frameTarget.Release();

			const int shadowMapWidth = lightDesc.shadowMapRes;
			const int shadowMapHegiht = lightDesc.shadowMapRes;


			const bool shouldCreateNewShadowMapTexture = lsi.pointLightDepthTexture.IsResourceValid() == false ||
			                                             lsi.pointLightDepthTexture->getDesc().textureType != UniformType::TextureCube ||
			                                             lsi.pointLightDepthTexture->getDesc().textureCube.width != shadowMapWidth ||
			                                             lsi.pointLightDepthTexture->getDesc().textureCube.height != shadowMapHegiht;

			if (shouldCreateNewShadowMapTexture) {
				TextureDesc texDesc;
				texDesc.textureType = UniformType::TextureCube;
				texDesc.format = TextureFormat::D24_UNORM_S8_UINT;
				texDesc.usage = TextureUsage::DepthStencilResource;
				texDesc.textureCube.width = shadowMapWidth;
				texDesc.textureCube.height = shadowMapHegiht;
				texDesc.textureCube.arraySize = 1;
				texDesc.textureCube.numMips = 1;
				texDesc.textureCube.sampleQuality = 0;
				texDesc.textureCube.numSamples = 1;

				lsi.pointLightDepthTexture = getCore()->getDevice()->requestResource<Texture>();
				[[maybe_unused]] const bool succeeded = lsi.pointLightDepthTexture->create(texDesc, nullptr);
				sgeAssert(succeeded);


				for (int iSignedAxis = 0; iSignedAxis < SGE_ARRSZ(lsi.pointLightFrameTargets); ++iSignedAxis) {
					// Destroy the prevously existing frame target for the face.
					lsi.pointLightFrameTargets[iSignedAxis].Release();

					// Create the new one.
					GpuHandle<FrameTarget>& faceFrameTarget = lsi.pointLightFrameTargets[iSignedAxis];
					faceFrameTarget = getCore()->getDevice()->requestResource<FrameTarget>();

					TargetDesc faceTargetDesc;
					faceTargetDesc.baseTextureType = UniformType::TextureCube;
					faceTargetDesc.textureCube.face = SignedAxis(iSignedAxis);
					faceTargetDesc.textureCube.mipLevel = 0;

					[[maybe_unused]] const bool targetSucceeded =
					    faceFrameTarget->create(0, nullptr, nullptr, lsi.pointLightDepthTexture, faceTargetDesc);

					sgeAssert(targetSucceeded);
				}
			}

		} else {
			// No point light frame targets are going to be needed so release them.
			for (int t = 0; t < SGE_ARRSZ(lsi.pointLightFrameTargets); ++t) {
				lsi.pointLightFrameTargets[t].Release();
			}

			lsi.pointLightDepthTexture.Release();

			// Create the shadow map frame target (with texture) for the shadow map.
			const int shadowMapWidth = lightDesc.shadowMapRes;
			const int shadowMapHegiht = lightDesc.shadowMapRes;

			GpuHandle<FrameTarget>& shadowFrameTarget = lsi.frameTarget;

			const bool shouldCreateNewShadowMapTexture = shadowFrameTarget.IsResourceValid() == false ||
			                                             shadowFrameTarget->getWidth() != shadowMapWidth ||
			                                             shadowFrameTarget->getHeight() != shadowMapHegiht;

			if (shouldCreateNewShadowMapTexture) {
				// Caution, TODO: On Safari (the web browser) I've read that it needs a color render target.
				// Keep that in mind when testing and developing.
				shadowFrameTarget = getCore()->getDevice()->requestResource<FrameTarget>();
				shadowFrameTarget->create2D(shadowMapWidth, shadowMapHegiht, TextureFormat::Unknown, TextureFormat::D24_UNORM_S8_UINT);
			}
		}


		// Draw the shadow map to the created frame target.
		const auto drawShadowMapFromCamera = [this, &lsi](const RenderDestination& rendDest, ICamera* gameCamera,
		                                                  ICamera* drawCamera) -> void {
			GameDrawSets drawShadowSets;

			drawShadowSets.gameCamera = gameCamera;
			drawShadowSets.drawCamera = drawCamera; //&shadowMapBuildInfoOpt->shadowMapCamera;
			drawShadowSets.rdest = rendDest;        // RenderDestination(getCore()->getDevice()->getContext(), resultFrameTarget);
			drawShadowSets.quickDraw = &getCore()->getQuickDraw();
			drawShadowSets.shadowMapBuildInfo = &lsi.buildInfo;

			drawWorld(drawShadowSets, drawReason_gameplayShadow);
		};

		if (shadowMapBuildInfoOpt->isPointLight) {
			for (int iSignedAxis = 0; iSignedAxis < signedAxis_numElements; ++iSignedAxis) {
				// Clear the pre-existing state.
				GpuHandle<FrameTarget>& faceFrameTarget = lsi.pointLightFrameTargets[iSignedAxis];
				getCore()->getDevice()->getContext()->clearColor(faceFrameTarget, 0, vec4f(0.f).data);
				getCore()->getDevice()->getContext()->clearDepth(faceFrameTarget, 1.f);

				// Render the scene for the current face of the cube map.
				RenderDestination rdest(getCore()->getDevice()->getContext(), faceFrameTarget);
				drawShadowMapFromCamera(rdest, gameCamera, &shadowMapBuildInfoOpt->pointLightShadowMapCameras[iSignedAxis]);
			}
		} else {
			// Clear the pre-existing state.
			getCore()->getDevice()->getContext()->clearColor(lsi.frameTarget, 0, vec4f(0.f).data);
			getCore()->getDevice()->getContext()->clearDepth(lsi.frameTarget, 1.f);

			// Non-point lights have only one camera that uses the whole texture for storing the shadow map.
			RenderDestination rdest(getCore()->getDevice()->getContext(), lsi.frameTarget);
			drawShadowMapFromCamera(rdest, gameCamera, &shadowMapBuildInfoOpt->shadowMapCamera);
		}

		lsi.isCorrectlyUpdated = true;
	}

	for (const GameObject* actorLight : *allLights) {
		const ALight* const light = static_cast<const ALight*>(actorLight);
		const LightDesc& lightDesc = light->getLightDesc();

		ShadingLightData shadingLight;

		if (lightDesc.isOn == false) {
			continue;
		}

		vec3f color = lightDesc.color * lightDesc.intensity;
		vec4f position(0.f);
		float spotLightCosAngle = 1.f; // Defaults to cos(0)

		if (lightDesc.type == light_point) {
			position = vec4f(light->getTransform().p, float(light_point));
		} else if (lightDesc.type == light_directional) {
			position = vec4f(-light->getTransformMtx().c0.xyz().normalized0(), float(light_directional));
		} else if (lightDesc.type == light_spot) {
			position = vec4f(light->getTransform().p, float(light_spot));
			spotLightCosAngle = cosf(lightDesc.spotLightAngle);
		} else {
			sgeAssert(false);
		}

		int flags = 0;
		LightShadowInfo& lsi = m_perLightShadowFrameTarget[light->getId()];
		if (lightDesc.hasShadows && lsi.isCorrectlyUpdated) {
			if (lsi.buildInfo.isPointLight) {
				if (lsi.pointLightDepthTexture.IsResourceValid()) {
					shadingLight.shadowMap = lsi.pointLightDepthTexture;
					shadingLight.shadowMapProjView = mat4f::getIdentity(); // Not used.
					flags |= kLightFlg_HasShadowMap;
				}
			} else {
				if (lsi.frameTarget.IsResourceValid()) {
					shadingLight.shadowMap = lsi.frameTarget->getDepthStencil();
					shadingLight.shadowMapProjView = lsi.buildInfo.shadowMapCamera.getProjView();
					flags |= kLightFlg_HasShadowMap;
				}
			}
		}

		shadingLight.lightPositionAndType = position;
		shadingLight.lightColorWFlags = vec4f(color, float(flags));
		shadingLight.lightSpotDirAndCosAngle = vec4f(light->getTransformMtx().c0.xyz().normalized0(), spotLightCosAngle);
		shadingLight.lightXShadowRange = vec4f(lightDesc.range, 0.f, 0.f, 0.f);

		shadingLight.lightBoxWs = light->getBBoxOS().getTransformed(light->getTransformMtx());

		m_shadingLights.push_back(shadingLight);
	}

	m_shadingLightPerObject.reserve(m_shadingLights.size());
}

bool DefaultGameDrawer::isInFrustum(const GameDrawSets& drawSets, Actor* actor) const {
	// If the camera frustum is present, try to clip the object.
	const Frustum* const pFrustum = drawSets.drawCamera->getFrustumWS();
	const AABox3f bboxOS = actor->getBBoxOS();
	if (pFrustum != nullptr) {
		if (!bboxOS.IsEmpty()) {
			const transf3d& tr = actor->getTransform();

			// We can technically transform the box in world space and then take the bounding sphere, however
			// transforming 8 verts is a perrty costly operation (and we do not need all the data).
			// So instead of that we "manually compute the sphere here.
			// Note: is that really faster?!
			vec3f const bboxCenterOS = bboxOS.center();
			quatf const bbSpherePosQ = tr.r * quatf(bboxCenterOS * tr.s, 0.f) * conjugate(tr.r);
			vec3f const bbSpherePos = bbSpherePosQ.xyz() + tr.p;
			float const bbSphereRadius = bboxOS.halfDiagonal().length() * tr.s.componentMaxAbs();

			if (pFrustum->isSphereOutside(bbSpherePos, bbSphereRadius)) {
				return false;
			}
		}
	}

	return true;
}

void DefaultGameDrawer::fillGeneralModsWithLights(Actor* actor, GeneralDrawMod& generalMods) {
	// Find all the lights that can affect this object.
	m_shadingLightPerObject.clear();
	AABox3f bboxOS = actor->getBBoxOS();
	AABox3f actorBBoxWs = bboxOS.getTransformed(actor->getTransformMtx());
	for (const ShadingLightData& shadingLight : m_shadingLights) {
		if (shadingLight.lightBoxWs.IsEmpty() || shadingLight.lightBoxWs.overlaps(actorBBoxWs)) {
			m_shadingLightPerObject.emplace_back(&shadingLight);
		}
	}

	generalMods.ppLightData = m_shadingLightPerObject.data();
	generalMods.lightsCount = int(m_shadingLightPerObject.size());
}


void DefaultGameDrawer::drawWorld(const GameDrawSets& drawSets, const DrawReason drawReason) {
	IGameDrawer::drawWorld(drawSets, drawReason);

// Draw the sky.
#if 0
	if (drawReason_IsGameOrEditNoShadowPass(drawReason)) {
		StateGroup sg;
		sg.setProgram(m_skyGradientShader);
		sg.setPrimitiveTopology(PrimitiveTopology::TriangleList);
		sg.setVBDeclIndex(m_skySphereVBVertexDeclIdx);
		sg.setVB(0, m_skySphereVB, 0, sizeof(vec3f));

		RasterizerState* const rasterState = getCore()->getGraphicsResources().RS_noCulling;

		sg.setRenderState(rasterState, getCore()->getGraphicsResources().DSS_default_lessEqual);

		BindLocation uViewLoc = m_skyGradientShader->getReflection().findUniform("uView");
		BindLocation uProjLoc = m_skyGradientShader->getReflection().findUniform("uProj");
		BindLocation uColorBottomLoc = m_skyGradientShader->getReflection().findUniform("uColorBottom");
		BindLocation uColorTomLoc = m_skyGradientShader->getReflection().findUniform("uColorTop");
		BindLocation uProjViewInverseLoc = m_skyGradientShader->getReflection().findUniform("uProjViewInverse");
		BindLocation uSkyTextureLoc = m_skyGradientShader->getReflection().findUniform("uSkyTexture");
		BindLocation uCamPosWSLoc = m_skyGradientShader->getReflection().findUniform("uCamPosWS");

		mat4f view = drawSets.drawCamera->getView();
		mat4f proj = drawSets.drawCamera->getProj();

		// TODO: more optimal sky shading please.uProjViewInverse
		[[maybe_unused]] Frustum f = Frustum::extractClippingPlanes(proj, kIsTexcoordStyleD3D);
		[[maybe_unused]] float far = f.f.v4.w;
		// HACK: use a separate variable.
		view = drawSets.drawCamera->getView() * mat4f::getScaling(far - 1e-3f);

		vec3f colorBottom = getWorld()->m_skyColorBottom;
		vec3f colorTop = getWorld()->m_skyColorTop;

		PAsset skyTex = getWorld()->skyTexAsset;
		if (isAssetLoaded(skyTex)) {
			mat4f projViewInv = drawSets.drawCamera->getProjView().inverse();
			vec3f camPos = drawSets.drawCamera->getCameraPosition();

			StaticArray<BoundUniform, 12> uniforms;
			uniforms.push_back(BoundUniform(uViewLoc, &view));
			uniforms.push_back(BoundUniform(uProjLoc, &proj));
			uniforms.push_back(BoundUniform(uProjViewInverseLoc, &projViewInv));
			uniforms.push_back(BoundUniform(uCamPosWSLoc, camPos.data));
			uniforms.push_back(BoundUniform(uColorBottomLoc, &colorBottom));
			uniforms.push_back(BoundUniform(uColorTomLoc, &colorTop));
			uniforms.push_back(BoundUniform(uSkyTextureLoc, skyTex->asTextureView()->tex.GetPtr()));

			DrawCall dc;
			dc.setStateGroup(&sg);
			dc.setUniforms(uniforms.data(), uniforms.size());
			dc.draw(m_skySphereNumVerts, 0);

			drawSets.rdest.sgecon->executeDrawCall(dc, drawSets.rdest.frameTarget, &drawSets.rdest.viewport);
		}
	}
#endif
}

void DefaultGameDrawer::drawActor(
    const GameDrawSets& drawSets, EditMode const editMode, Actor* actor, int const itemIndex, DrawReason const drawReason) {
	const bool useWireframe = drawReason_IsVisualizeSelection(drawReason);

	const vec4f wireframeColor = (drawReason == drawReason_visualizeSelectionPrimary) ? kPrimarySelectionColor : kSecondarySelectionColor;
	const int wireframeColorInt = colorToIntRgba(wireframeColor);

	vec4f selectionTint = useWireframe ? wireframeColor : vec4f(0.f);
	selectionTint.w = useWireframe ? 1.f : 0.f;

	// Build the general modifications for the actor drawing.
	GeneralDrawMod generalMods;

	generalMods.selectionTint = selectionTint;
	generalMods.isRenderingShadowMap = (drawReason == drawReason_gameplayShadow);
	generalMods.ambientLightColor = getWorld()->m_ambientLight;
	generalMods.uRimLightColorWWidth = vec4f(getWorld()->m_rimLight, getWorld()->m_rimCosineWidth);

	// If the camera frustum is present, try to clip the object.
	if (isInFrustum(drawSets, actor) == false) {
		return;
	}

	// Find all the lights that can affect this object.
	fillGeneralModsWithLights(actor, generalMods);

	if (TraitParticles* const particlesTrait = getTrait<TraitParticles>(actor); editMode == editMode_actors && particlesTrait) {
		drawTraitParticles(particlesTrait, drawSets, generalMods);
	}

	if (TraitParticles2* const particles2Trait = getTrait<TraitParticles2>(actor); editMode == editMode_actors && particles2Trait) {
		drawTraitParticles2(particles2Trait, drawSets, generalMods);
	}

	if (TraitModel* const modelTrait = getTrait<TraitModel>(actor); editMode == editMode_actors && modelTrait) {
		drawTraitStaticModel(modelTrait, drawSets, generalMods, drawReason);
	}

	if (TraitMultiModel* const multiModelTrait = getTrait<TraitMultiModel>(actor); editMode == editMode_actors && multiModelTrait) {
		drawTraitMultiModel(multiModelTrait, drawSets, generalMods, drawReason);
	}

	if (TraitRenderableGeom* const ttRendGeom = getTrait<TraitRenderableGeom>(actor); editMode == editMode_actors && ttRendGeom) {
		drawTraitRenderableGeom(ttRendGeom, drawSets, generalMods);
	}

	if (TraitTexturedPlane* const traitTexPlane = getTrait<TraitTexturedPlane>(actor); traitTexPlane) {
		drawTraitTexturedPlane(traitTexPlane, drawSets, generalMods, drawReason);
	}

	if (drawReason_IsEditOrSelectionTool(drawReason)) {
		if (TraitViewportIcon* const traitViewportIcon = getTrait<TraitViewportIcon>(actor); traitViewportIcon) {
			drawTraitViewportIcon(traitViewportIcon, drawSets, drawReason);
		}
	}

	const TypeId actorType = actor->getType();
	if (actorType == sgeTypeId(ANavMesh) && drawReason_IsEditOrSelectionTool(drawReason)) {
		drawANavMesh(static_cast<ANavMesh*>(actor), drawSets, generalMods, drawReason, wireframeColorInt);
	}

	drawActorLegacy(actor, drawSets, editMode, itemIndex, generalMods, drawReason);
}

void DefaultGameDrawer::drawTraitTexturedPlane(TraitTexturedPlane* traitTexPlane,
                                               const GameDrawSets& drawSets,
                                               const GeneralDrawMod& generalMods,
                                               DrawReason const UNUSED(drawReason)) {
	const vec3f camPos = drawSets.drawCamera->getCameraPosition();
	const vec3f camLookDir = drawSets.drawCamera->getCameraLookDir();
	Actor* actor = traitTexPlane->getActor();

	AssetTexture* assetTexture = traitTexPlane->getAssetProperty().getAssetTexture();
	if (assetTexture && assetTexture->tex.IsResourceValid()) {
		Texture* const texture = assetTexture->tex.GetPtr();
		if (texture) {
			const mat4f localOffsetmtx = mat4f::getTranslation(traitTexPlane->m_localXOffset, 0.f, 0.f);
			const mat4f anchorAlignMtx = traitTexPlane->getAnchorAlignMtxOS();
			const mat4f billboardFacingMtx =
			    billboarding_getOrentationMtx(traitTexPlane->m_billboarding, actor->getTransform(),
			                                  drawSets.drawCamera->getCameraPosition(), drawSets.drawCamera->getView(), false);
			const mat4f objToWorld = billboardFacingMtx * anchorAlignMtx * localOffsetmtx;

			Geometry texPlaneGeom = m_texturedPlaneDraw.getGeometry(drawSets.rdest.getDevice());
			Material texPlaneMtl = m_texturedPlaneDraw.getMaterial(texture);

			InstanceDrawMods mods;
			mods.gameTime = getWorld()->timeSpendPlaying;

			m_modeldraw.drawGeometry(drawSets.rdest, camPos, camLookDir, drawSets.drawCamera->getProjView(), objToWorld, generalMods,
			                         &texPlaneGeom, texPlaneMtl, mods);
		}
	}
};

void DefaultGameDrawer::drawTraitViewportIcon(TraitViewportIcon* viewportIcon, const GameDrawSets& drawSets, DrawReason const drawReason) {
	vec4f wireframeColor = vec4f(1.f);

	if (drawReason == drawReason_visualizeSelectionPrimary)
		wireframeColor = kPrimarySelectionColor;
	else if (drawReason == drawReason_visualizeSelection)
		wireframeColor = kSecondarySelectionColor;

	Texture* const iconTexture = viewportIcon->getIconTexture();

	if (iconTexture != nullptr) {
		const mat4f node2world = viewportIcon->computeNodeToWorldMtx(*drawSets.drawCamera);
		m_texturedPlaneDraw.draw(drawSets.rdest, drawSets.drawCamera->getProjView() * node2world, iconTexture, wireframeColor);
	}
};


void DefaultGameDrawer::drawTraitStaticModel(TraitModel* modelTrait,
                                             const GameDrawSets& drawSets,
                                             const GeneralDrawMod& generalMods,
                                             DrawReason const drawReason) {
	if (modelTrait->getRenderable() == false) {
		return;
	}

	const vec3f camPos = drawSets.drawCamera->getCameraPosition();
	const vec3f camLookDir = drawSets.drawCamera->getCameraLookDir();
	Actor* const actor = modelTrait->getActor();

	PAsset const asset = modelTrait->getAssetProperty().getAsset();

	if (isAssetLoaded(asset) && asset->getType() == AssetType::Model) {
		AssetModel* const model = modelTrait->getAssetProperty().getAssetModel();

		std::vector<MaterialOverride> mtlOverrides;
		for (auto& mtlOverride : modelTrait->m_materialOverrides) {
			GameObject* const mtlProvider = getWorld()->getObjectById(mtlOverride.materialObjId);
			OMaterial* mtl = dynamic_cast<OMaterial*>(mtlProvider);
			if (mtl) {
				MaterialOverride ovr;
				ovr.name = mtlOverride.materialName;
				ovr.mtl = mtl->getMaterial();

				mtlOverrides.push_back(ovr);
			}
		}

		if (!drawReason_IsVisualizeSelection(drawReason)) {
			if (modelTrait->useSkeleton) {
				if (model && model->sharedEval.isInitialized()) {
					// Compute the overrdies.
					std::vector<mat4f> boneOverrides;
					modelTrait->computeSkeleton(boneOverrides);
					// Draw
					const mat4f n2w = actor->getTransformMtx() * modelTrait->m_additionalTransform;
					model->sharedEval.evaluateFromNodesGlobalTransform(boneOverrides);
					m_modeldraw.draw(drawSets.rdest, camPos, camLookDir, drawSets.drawCamera->getProjView(), n2w, generalMods,
					                 model->sharedEval, modelTrait->instanceDrawMods, &mtlOverrides);
				}
			} else {
				if (modelTrait->m_evalModel) {
					const mat4f n2w = actor->getTransformMtx() * modelTrait->m_additionalTransform;
					m_modeldraw.draw(drawSets.rdest, camPos, camLookDir, drawSets.drawCamera->getProjView(), n2w, generalMods,
					                 *modelTrait->m_evalModel, modelTrait->instanceDrawMods, &mtlOverrides);
				} else if (model && model->staticEval.isInitialized()) {
					const mat4f n2w = actor->getTransformMtx() * modelTrait->m_additionalTransform;
					m_modeldraw.draw(drawSets.rdest, camPos, camLookDir, drawSets.drawCamera->getProjView(), n2w, generalMods,
					                 model->staticEval, modelTrait->instanceDrawMods, &mtlOverrides);
				}
			}
		} else {
			if (modelTrait->useSkeleton) {
				if (model && model->sharedEval.isInitialized()) {
					// Compute the overrdies.
					std::vector<mat4f> boneOverrides;
					modelTrait->computeSkeleton(boneOverrides);

					// Draw
					const mat4f n2w = actor->getTransformMtx() * modelTrait->m_additionalTransform;
					model->sharedEval.evaluateFromNodesGlobalTransform(boneOverrides);
					m_constantColorShader.draw(drawSets.rdest, drawSets.drawCamera->getProjView(), n2w, model->sharedEval,
					                           generalMods.selectionTint);
				}
			} else {
				if (modelTrait->m_evalModel) {
					const mat4f n2w = actor->getTransformMtx() * modelTrait->m_additionalTransform;
					m_constantColorShader.draw(drawSets.rdest, drawSets.drawCamera->getProjView(), n2w, *modelTrait->m_evalModel,
					                           generalMods.selectionTint);
				} else if (model && model->staticEval.isInitialized()) {
					const mat4f n2w = actor->getTransformMtx() * modelTrait->m_additionalTransform;
					m_constantColorShader.draw(drawSets.rdest, drawSets.drawCamera->getProjView(), n2w, model->staticEval,
					                           generalMods.selectionTint);
				}
			}
		}
	} else if (isAssetLoaded(asset) && asset->getType() == AssetType::Texture2D) {
		AssetTexture* ppTexture = asset->asTextureView();
		if (ppTexture && ppTexture->tex.IsResourceValid()) {
			Texture* const texture = ppTexture->tex.GetPtr();
			if (texture) {
				mat4f obj2world = modelTrait->imageSettings.computeObjectToWorldTransform(
				    *asset.get(), drawSets.drawCamera, actor->getTransform(), modelTrait->m_additionalTransform);

				Geometry texPlaneGeom = m_texturedPlaneDraw.getGeometry(drawSets.rdest.getDevice());
				Material texPlaneMtl = m_texturedPlaneDraw.getMaterial(texture);
				texPlaneMtl.diffuseColor = vec4f(modelTrait->imageSettings.colorTint);

				if (drawReason_IsVisualizeSelection(drawReason)) {
					texPlaneMtl.diffuseColor *= generalMods.selectionTint;
				}

				InstanceDrawMods mods;
				mods.gameTime = getWorld()->timeSpendPlaying;
				mods.forceNoLighting = modelTrait->imageSettings.forceNoLighting;
				mods.forceNoCulling = modelTrait->imageSettings.forceNoCulling;

				m_modeldraw.drawGeometry(drawSets.rdest, camPos, camLookDir, drawSets.drawCamera->getProjView(), obj2world, generalMods,
				                         &texPlaneGeom, texPlaneMtl, mods);
			}
		}
	} else if (isAssetLoaded(asset) && asset->getType() == AssetType::Sprite) {
		SpriteAnimationAsset* const pSprite = asset->asSprite();
		if (pSprite && isAssetLoaded(pSprite->textureAsset) && pSprite->textureAsset->asTextureView() != nullptr &&
		    pSprite->textureAsset->asTextureView()->tex.GetPtr() != nullptr) {
			// Get the frame of the sprite to be rendered.
			const SpriteAnimation::Frame* const frame = pSprite->spriteAnimation.getFrameForTime(modelTrait->imageSettings.spriteFrameTime);
			if (frame) {
				mat4f obj2world = modelTrait->imageSettings.computeObjectToWorldTransform(
				    *asset.get(), drawSets.drawCamera, actor->getTransform(), modelTrait->m_additionalTransform);

				Geometry texPlaneGeom = m_texturedPlaneDraw.getGeometry(drawSets.rdest.getDevice());
				Material texPlaneMtl = m_texturedPlaneDraw.getMaterial(pSprite->textureAsset->asTextureView()->tex.GetPtr());
				texPlaneMtl.diffuseColor = vec4f(modelTrait->imageSettings.colorTint);

				if (drawReason_IsVisualizeSelection(drawReason)) {
					texPlaneMtl.diffuseColor *= generalMods.selectionTint;
				}

				InstanceDrawMods mods;
				mods.gameTime = getWorld()->timeSpendPlaying;
				mods.forceNoLighting = modelTrait->imageSettings.forceNoLighting;
				mods.forceNoCulling = true; // modelTrait->imageSettings.forceNoCulling;

				// Compute the UVW transform so we get only this frame portion of the texture to be displayed.
				texPlaneMtl.uvwTransform =
				    mat4f::getTranslation(frame->uvRegion.x, frame->uvRegion.y, 0.f) *
				    mat4f::getScaling(frame->uvRegion.z - frame->uvRegion.x, frame->uvRegion.w - frame->uvRegion.y, 0.f);

				m_modeldraw.drawGeometry(drawSets.rdest, camPos, camLookDir, drawSets.drawCamera->getProjView(), obj2world, generalMods,
				                         &texPlaneGeom, texPlaneMtl, mods);
			}
		}
	}
}

void DefaultGameDrawer::drawTraitParticles(TraitParticles* particlesTrait, const GameDrawSets& drawSets, GeneralDrawMod generalMods) {
	const vec3f camPos = drawSets.drawCamera->getCameraPosition();
	const vec3f camLookDir = drawSets.drawCamera->getCameraLookDir();

	generalMods.selectionTint = vec4f(0.f);

	for (ParticleGroupDesc& pdesc : particlesTrait->m_pgroups) {
		if (pdesc.m_visMethod == ParticleGroupDesc::vis_model3D) {
			AssetModel* model = pdesc.m_particleModel.getAssetModel();
			if (model != nullptr) {
				const auto& itr = particlesTrait->m_pgroupState.find(pdesc.m_name);
				const mat4f n2w = itr->second.getParticlesToWorldMtx();
				if (itr != particlesTrait->m_pgroupState.end()) {
					const ParticleGroupState& pstate = itr->second;

					for (const ParticleGroupState::ParticleState& particle : pstate.getParticles()) {
						mat4f particleTForm = n2w * mat4f::getTranslation(particle.pos) * mat4f::getScaling(particle.scale);

						m_modeldraw.draw(drawSets.rdest, camPos, camLookDir, drawSets.drawCamera->getProjView(), particleTForm, generalMods,
						                 model->staticEval, InstanceDrawMods());
					}
				}
			}
		} else if (pdesc.m_visMethod == ParticleGroupDesc::vis_sprite) {
			const auto& itr = particlesTrait->m_pgroupState.find(pdesc.m_name);
			if (itr != particlesTrait->m_pgroupState.end()) {
				const mat4f n2w = mat4f::getIdentity();

				ParticleGroupState& pstate = itr->second;
				ParticleGroupState::SpriteRendData* srd =
				    pstate.computeSpriteRenderData(*drawSets.rdest.sgecon, pdesc, *drawSets.drawCamera);
				if (srd != nullptr) {
					InstanceDrawMods mods;
					mods.forceNoLighting = true;
					mods.forceAdditiveBlending = true;

					m_modeldraw.drawGeometry(drawSets.rdest, camPos, camLookDir, drawSets.drawCamera->getProjView(), n2w, generalMods,
					                         &srd->geometry, srd->material, mods);
				}
			}
		}
	}
}

void DefaultGameDrawer::drawTraitParticles2(TraitParticles2* particlesTrait, const GameDrawSets& drawSets, GeneralDrawMod generalMods) {
	const mat4f n2w = particlesTrait->getActor()->getTransformMtx();

	const vec3f camPos = drawSets.drawCamera->getCameraPosition();
	const vec3f camLookDir = drawSets.drawCamera->getCameraLookDir();

	generalMods.selectionTint = vec4f(0.f);

	const int numPGroups = particlesTrait->getNumPGroups();
	for (int iGroup = 0; iGroup < numPGroups; ++iGroup) {
		TraitParticles2::ParticleGroup* pgrp = particlesTrait->getPGroup(iGroup);

		if (pgrp->spriteTexture->getType() == AssetType::Model) {
			for (const TraitParticles2::ParticleGroup::ParticleData& particle : pgrp->allParticles) {
				mat4f particleTForm =
				    mat4f::getTranslation(particle.position) * mat4f::getRotationQuat(particle.spin) * mat4f::getScaling(particle.scale);

				m_modeldraw.draw(drawSets.rdest, camPos, camLookDir, drawSets.drawCamera->getProjView(), particleTForm, generalMods,
				                 pgrp->spriteTexture->asModel()->staticEval, InstanceDrawMods());
			}
		} else {
			if (m_partRendDataGen.generate(*pgrp, *drawSets.rdest.sgecon, *drawSets.drawCamera, n2w)) {
				InstanceDrawMods mods;
				mods.forceNoLighting = true;
				mods.forceAdditiveBlending = true;

				const mat4f identity = mat4f::getIdentity();
				m_modeldraw.drawGeometry(drawSets.rdest, camPos, camLookDir, drawSets.drawCamera->getProjView(), identity, generalMods,
				                         &m_partRendDataGen.geometry, m_partRendDataGen.material, mods);
			}
		}
	}
}

void DefaultGameDrawer::drawANavMesh(ANavMesh* navMesh,
                                     const GameDrawSets& drawSets,
                                     const GeneralDrawMod& UNUSED(generalMods),
                                     DrawReason const drawReason,
                                     const uint32 wireframeColor) {
	vec4f wireframeColorAlphaFloat = colorFromIntRgba(wireframeColor);
	wireframeColorAlphaFloat.w = 0.5f;
	uint32 wireframeColorAlpha = colorToIntRgba(wireframeColorAlphaFloat);

	if (drawReason_IsEditOrSelectionTool(drawReason) && !drawReason_IsSelection(drawReason)) {
		drawSets.quickDraw->drawWired_Clear();
		drawSets.quickDraw->drawWiredAdd_Box(navMesh->getTransformMtx(), wireframeColor);

		for (size_t iTri = 0; iTri < navMesh->m_debugDrawNavMeshTriListWs.size() / 3; ++iTri) {
			vec3f a = navMesh->m_debugDrawNavMeshTriListWs[iTri * 3 + 0];
			vec3f b = navMesh->m_debugDrawNavMeshTriListWs[iTri * 3 + 1];
			vec3f c = navMesh->m_debugDrawNavMeshTriListWs[iTri * 3 + 2];

			drawSets.quickDraw->drawSolidAdd_Triangle(a, b, c, wireframeColorAlpha);
			drawSets.quickDraw->drawWiredAdd_triangle(a, b, c, wireframeColor);
		}

		uint32 buildMeshColorInt = colorIntFromRGBA255(246, 189, 85);
		uint32 buildMeshColorAlphaInt = colorIntFromRGBA255(246, 189, 85, 127);
		for (size_t iTri = 0; iTri < navMesh->m_debugDrawNavMeshBuildTriListWs.size() / 3; ++iTri) {
			vec3f a = navMesh->m_debugDrawNavMeshBuildTriListWs[iTri * 3 + 0];
			vec3f b = navMesh->m_debugDrawNavMeshBuildTriListWs[iTri * 3 + 1];
			vec3f c = navMesh->m_debugDrawNavMeshBuildTriListWs[iTri * 3 + 2];

			drawSets.quickDraw->drawSolidAdd_Triangle(a, b, c, buildMeshColorAlphaInt);
			drawSets.quickDraw->drawWiredAdd_triangle(a, b, c, buildMeshColorInt);
		}

		drawSets.quickDraw->drawSolid_Execute(drawSets.rdest, drawSets.drawCamera->getProjView(), false,
		                                      getCore()->getGraphicsResources().BS_backToFrontAlpha);
		drawSets.quickDraw->drawWired_Execute(drawSets.rdest, drawSets.drawCamera->getProjView());
	}
}

void DefaultGameDrawer::drawTraitMultiModel(TraitMultiModel* multiModelTrait,
                                            const GameDrawSets& drawSets,
                                            const GeneralDrawMod& generalMods,
                                            DrawReason const UNUSED(drawReason)) {
	const vec3f camPos = drawSets.drawCamera->getCameraPosition();
	const vec3f camLookDir = drawSets.drawCamera->getCameraLookDir();
	Actor* actor = multiModelTrait->getActor();

	for (TraitMultiModel::Element& elem : multiModelTrait->models) {
		if (elem.isRenderable) {
			AssetModel* const model = elem.assetProperty.getAssetModel();
			if (model && model->staticEval.isInitialized()) {
				mat4f n2w;
				if (elem.isAdditionalTransformInWorldSpace)
					n2w = elem.additionalTransform;
				else
					n2w = actor->getTransformMtx() * elem.additionalTransform;

				m_modeldraw.draw(drawSets.rdest, camPos, camLookDir, drawSets.drawCamera->getProjView(), n2w, generalMods,
				                 model->staticEval, InstanceDrawMods()); // TODO MODS
			}
		}
	}
}

void DefaultGameDrawer::drawTraitRenderableGeom(TraitRenderableGeom* ttRendGeom,
                                                const GameDrawSets& drawSets,
                                                const GeneralDrawMod& generalMods) {
	const mat4f actorToWorld = ttRendGeom->getActor()->getTransformMtx();
	const vec3f camPos = drawSets.drawCamera->getCameraPosition();
	const vec3f camLookDir = drawSets.drawCamera->getCameraLookDir();

	for (const TraitRenderableGeom::Element& elem : ttRendGeom->geoms) {
		if (elem.isRenderable && elem.pGeom && elem.pMtl) {
			mat4f n2w = elem.isTformInWorldSpace ? elem.tform : actorToWorld * elem.tform;
			m_modeldraw.drawGeometry(drawSets.rdest, camPos, camLookDir, drawSets.drawCamera->getProjView(), n2w, generalMods, elem.pGeom,
			                         *elem.pMtl, InstanceDrawMods());
		}
	}
}

// A Legacy function that should end up not being used.
void DefaultGameDrawer::drawActorLegacy(Actor* actor,
                                        const GameDrawSets& drawSets,
                                        EditMode const editMode,
                                        int const itemIndex,
                                        const GeneralDrawMod& generalMods,
                                        DrawReason const drawReason) {
	TypeId actorType = actor->getType();

	const vec3f camPos = drawSets.drawCamera->getCameraPosition();
	const vec3f camLookDir = drawSets.drawCamera->getCameraLookDir();

	const bool useWireframe = drawReason_IsVisualizeSelection(drawReason);
	vec4f wireframeColor = (drawReason == drawReason_visualizeSelectionPrimary) ? kPrimarySelectionColor : kSecondarySelectionColor;
	if (!useWireframe && drawReason == drawReason_editing) {
		wireframeColor = vec4f(1.f);
	}
	const uint32 wireframeColorInt = colorToIntRgba(wireframeColor);

	vec4f selectionTint = useWireframe ? wireframeColor : vec4f(0.f);
	selectionTint.w = useWireframe ? 1.f : 0.f;

	// If we are rendering the actor to visualize a selection, show a wireframe representation specifying how the light is oriented, or its
	// area of effect.
	if (actorType == sgeTypeId(ALight) && drawReason_IsVisualizeSelection(drawReason)) {
		if (editMode == editMode_actors) {
			drawSets.quickDraw->drawWired_Clear();

			const ALight* const light = static_cast<const ALight*>(actor);
			const vec3f color = light->getLightDesc().color;
			const uint32 colorInt = useWireframe ? wireframeColorInt : colorToIntRgba(color);
			const LightDesc lightDesc = light->getLightDesc();
			if (lightDesc.type == light_point) {
				const float sphereRadius = maxOf(lightDesc.range, 0.1f);
				drawSets.quickDraw->drawWiredAdd_Sphere(actor->getTransformMtx(), colorInt, sphereRadius, 6);
			} else if (lightDesc.type == light_directional) {
				const float arrowLength = maxOf(lightDesc.intensity * 10.f, 1.f);
				drawSets.quickDraw->drawWiredAdd_Arrow(
				    actor->getTransform().p, actor->getTransform().p + actor->getTransformMtx().c0.xyz().normalized() * arrowLength,
				    colorInt);
			} else if (lightDesc.type == light_spot) {
				const float coneHeight = maxOf(lightDesc.range, 2.f);
				const float coneRadius = tanf(lightDesc.spotLightAngle) * coneHeight;
				drawSets.quickDraw->drawWiredAdd_ConeBottomAligned(actor->getTransformMtx() * mat4f::getRotationZ(deg2rad(-90.f)), colorInt,
				                                                   coneHeight, coneRadius, 6);
			}

			drawSets.quickDraw->drawWired_Execute(drawSets.rdest, drawSets.drawCamera->getProjView());
		}
	}
	if (actorType == sgeTypeId(ABlockingObstacle) && editMode == editMode_actors) {
		ABlockingObstacle* const simpleObstacle = static_cast<ABlockingObstacle*>(const_cast<Actor*>(actor));

		if (simpleObstacle->geometry.hasData()) {
			if (drawReason_IsVisualizeSelection(drawReason)) {
				m_constantColorShader.drawGeometry(drawSets.rdest, drawSets.drawCamera->getProjView(), simpleObstacle->getTransformMtx(),
				                                   simpleObstacle->geometry, generalMods.selectionTint);
			} else {
				m_modeldraw.drawGeometry(drawSets.rdest, camPos, camLookDir, drawSets.drawCamera->getProjView(),
				                         simpleObstacle->getTransformMtx(), generalMods, &simpleObstacle->geometry,
				                         simpleObstacle->material, InstanceDrawMods());
			}
		}
	}
	if (actorType == sgeTypeId(ACamera) && drawReason_IsVisualizeSelection(drawReason)) {
		if (editMode == editMode_actors) {
			float const bodyEnd = -0.3f;
			float const sizeZ = 0.35f;
			float height = 0.35f;
			float eyeScaleStart = 0.5f;

			vec3f vertices[] = {
			    // Body
			    vec3f(bodyEnd, -height, -sizeZ),
			    vec3f(bodyEnd, -height, sizeZ),
			    vec3f(bodyEnd, height, -sizeZ),
			    vec3f(bodyEnd, height, sizeZ),

			    vec3f(-1.f, -height, -sizeZ),
			    vec3f(-1.f, -height, sizeZ),
			    vec3f(-1.f, height, -sizeZ),
			    vec3f(-1.f, height, sizeZ),

			    vec3f(bodyEnd, -height, -sizeZ),
			    vec3f(bodyEnd, height, -sizeZ),
			    vec3f(bodyEnd, -height, sizeZ),
			    vec3f(bodyEnd, height, sizeZ),

			    vec3f(-1.f, -height, -sizeZ),
			    vec3f(-1.f, height, -sizeZ),
			    vec3f(-1.f, -height, sizeZ),
			    vec3f(-1.f, height, sizeZ),

			    vec3f(bodyEnd, height, sizeZ),
			    vec3f(-1.f, height, sizeZ),
			    vec3f(bodyEnd, -height, sizeZ),
			    vec3f(-1.f, -height, sizeZ),

			    vec3f(bodyEnd, height, -sizeZ),
			    vec3f(-1.f, height, -sizeZ),
			    vec3f(bodyEnd, -height, -sizeZ),
			    vec3f(-1.f, -height, -sizeZ),

			    // Eye
			    vec3f(0.f, -height, -sizeZ),
			    vec3f(0.f, -height, sizeZ),
			    vec3f(0.f, height, -sizeZ),
			    vec3f(0.f, height, sizeZ),

			    vec3f(bodyEnd, eyeScaleStart * -height, eyeScaleStart * -sizeZ),
			    vec3f(bodyEnd, eyeScaleStart * -height, eyeScaleStart * sizeZ),
			    vec3f(bodyEnd, eyeScaleStart * height, eyeScaleStart * -sizeZ),
			    vec3f(bodyEnd, eyeScaleStart * height, eyeScaleStart * sizeZ),

			    vec3f(0.f, -height, -sizeZ),
			    vec3f(0.f, height, -sizeZ),
			    vec3f(0.f, -height, sizeZ),
			    vec3f(0.f, height, sizeZ),

			    vec3f(bodyEnd, eyeScaleStart * -height, eyeScaleStart * -sizeZ),
			    vec3f(bodyEnd, eyeScaleStart * height, eyeScaleStart * -sizeZ),
			    vec3f(bodyEnd, eyeScaleStart * -height, eyeScaleStart * sizeZ),
			    vec3f(bodyEnd, eyeScaleStart * height, eyeScaleStart * sizeZ),

			    vec3f(0.f, height, sizeZ),
			    vec3f(bodyEnd, eyeScaleStart * height, eyeScaleStart * sizeZ),
			    vec3f(0.f, -height, sizeZ),
			    vec3f(bodyEnd, eyeScaleStart * -height, eyeScaleStart * sizeZ),

			    vec3f(0.f, height, -sizeZ),
			    vec3f(bodyEnd, eyeScaleStart * height, eyeScaleStart * -sizeZ),
			    vec3f(0.f, -height, -sizeZ),
			    vec3f(bodyEnd, eyeScaleStart * -height, eyeScaleStart * -sizeZ),
			};

			const TraitCamera* const trait = getTrait<TraitCamera>(actor);
			if (trait) {
				const auto intersectPlanes = [](const Plane& p0, const Plane& p1, const Plane& p2) -> vec3f {
					// http://www.ambrsoft.com/TrigoCalc/Plan3D/3PlanesIntersection_.htm
					float const det = -triple(p0.norm(), p1.norm(), p2.norm()); // Caution: I'm not sure about that minus...

					float const x = triple(p0.v4.wyz(), p1.v4.wyz(), p2.v4.wyz()) / det;
					float const y = triple(p0.v4.xwz(), p1.v4.xwz(), p2.v4.xwz()) / det;
					float const z = triple(p0.v4.xyw(), p1.v4.xyw(), p2.v4.xyw()) / det;

					return vec3f(x, y, z);
				};


				const ICamera* const icam = trait->getCamera();
				const Frustum* const f = icam->getFrustumWS();
				if (f) {
					const vec3f frustumVerts[8] = {
					    intersectPlanes(f->t, f->r, f->n), intersectPlanes(f->t, f->l, f->n),
					    intersectPlanes(f->b, f->l, f->n), intersectPlanes(f->b, f->r, f->n),

					    intersectPlanes(f->t, f->r, f->f), intersectPlanes(f->t, f->l, f->f),
					    intersectPlanes(f->b, f->l, f->f), intersectPlanes(f->b, f->r, f->f),
					};

					const vec3f lines[] = {
					    // Near plane.
					    frustumVerts[0],
					    frustumVerts[1],
					    frustumVerts[1],
					    frustumVerts[2],
					    frustumVerts[2],
					    frustumVerts[3],
					    frustumVerts[3],
					    frustumVerts[0],

					    // Lines that connect near and far.
					    frustumVerts[4 + 0],
					    frustumVerts[4 + 1],
					    frustumVerts[4 + 1],
					    frustumVerts[4 + 2],
					    frustumVerts[4 + 2],
					    frustumVerts[4 + 3],
					    frustumVerts[4 + 3],
					    frustumVerts[4 + 0],

					    // Far plane.
					    frustumVerts[0],
					    frustumVerts[4],
					    frustumVerts[1],
					    frustumVerts[5],
					    frustumVerts[2],
					    frustumVerts[6],
					    frustumVerts[3],
					    frustumVerts[7],
					};

					drawSets.quickDraw->drawWired_Clear();
					for (int t = 0; t < SGE_ARRSZ(lines); t += 2) {
						drawSets.quickDraw->drawWiredAdd_Line(lines[t], lines[t + 1], wireframeColorInt);
					}
					drawSets.quickDraw->drawWired_Execute(drawSets.rdest, drawSets.drawCamera->getProjView());
				}
			}
		}
	}
	if (actorType == sgeTypeId(ALine) && drawReason_IsEditOrSelectionTool(drawReason)) {
		const ALine* const spline = static_cast<const ALine*>(actor);

		if (editMode == editMode_actors) {
			mat4f const obj2world = spline->getTransformMtx();

			const int color = useWireframe ? 0xFF0055FF : 0xFFFFFFFF;

			const int numSegments = spline->getNumSegments();
			for (int iSegment = 0; iSegment < numSegments; ++iSegment) {
				int i0, i1;
				if (spline->getSegmentVerts(iSegment, i0, i1) == false) {
					sgeAssert(false);
					break;
				}

				// TODO: cache one of the matrix multiplications.
				vec3f p0 = mat_mul_pos(obj2world, spline->points[i0]);
				vec3f p1 = mat_mul_pos(obj2world, spline->points[i1]);

				getCore()->getQuickDraw().drawWiredAdd_Line(p0, p1, color);
				getCore()->getQuickDraw().drawWiredAdd_Sphere(mat4f::getTranslation(p0), color, 0.2f, 3);

				if (iSegment == numSegments - 1) {
					getCore()->getQuickDraw().drawWiredAdd_Sphere(mat4f::getTranslation(p1), color, 0.2f, 3);
				}
			}

			getCore()->getQuickDraw().drawWired_Execute(drawSets.rdest, drawSets.drawCamera->getProjView(), nullptr);
		} else if (editMode == editMode_points) {
			mat4f const tr = spline->getTransformMtx();

			getCore()->getQuickDraw().drawWired_Clear();
			getCore()->getQuickDraw().drawWiredAdd_Sphere(tr * mat4f::getTranslation(spline->points[itemIndex]),
			                                              useWireframe ? 0x550055FF : 0xFFFFFFFF, 0.2f, 3);
			getCore()->getQuickDraw().drawWired_Execute(drawSets.rdest, drawSets.drawCamera->getProjView(), nullptr);
		}
	}
	if (actorType == sgeTypeId(ACRSpline) && drawReason_IsEditOrSelectionTool(drawReason)) {
		ACRSpline* const spline = static_cast<ACRSpline*>(actor);

		mat4f const tr = spline->getTransformMtx();
		const float pointScale = spline->getBBoxOS().getTransformed(tr).size().length() * 0.01f;

		if (editMode == editMode_actors) {
			int color = useWireframe ? 0x550055FF : 0xFFFFFFFF;

			const float lenPerLine = 1.f;

			getCore()->getQuickDraw().drawWired_Clear();

			vec3f p0;
			spline->evaluateAtDistance(&p0, nullptr, 0.f);
			p0 = mat_mul_pos(tr, p0);
			for (float t = lenPerLine; t <= spline->totalLength; t += lenPerLine) {
				vec3f p1;
				spline->evaluateAtDistance(&p1, nullptr, t);
				p1 = mat_mul_pos(tr, p1);
				getCore()->getQuickDraw().drawWiredAdd_Line(p0, p1, color);
				p0 = p1;
			}

			for (int t = 0; t < spline->getNumPoints(); t++) {
				vec3f worldPos = mat_mul_pos(tr, spline->points[t]);
				if (t == 0)
					getCore()->getQuickDraw().drawWiredAdd_Box(mat4f::getTRS(worldPos, quatf::getIdentity(), vec3f(pointScale)), color);
				else
					getCore()->getQuickDraw().drawWiredAdd_Sphere(mat4f::getTranslation(worldPos), color, pointScale, 3);
			}

			getCore()->getQuickDraw().drawWired_Execute(drawSets.rdest, drawSets.drawCamera->getProjView(), nullptr);
		} else if (editMode == editMode_points) {
			getCore()->getQuickDraw().drawWired_Clear();
			getCore()->getQuickDraw().drawWiredAdd_Sphere(tr * mat4f::getTranslation(spline->points[itemIndex]),
			                                              useWireframe ? 0x550055FF : 0xFFFFFFFF, pointScale, 3);
			getCore()->getQuickDraw().drawWired_Execute(drawSets.rdest, drawSets.drawCamera->getProjView(), nullptr);
		}
	}

	if (actorType == sgeTypeId(ALocator) && drawReason_IsEditOrSelectionTool(drawReason)) {
		if (editMode == editMode_actors) {
			drawSets.quickDraw->drawWired_Clear();
			drawSets.quickDraw->drawWiredAdd_Basis(actor->getTransformMtx());
			drawSets.quickDraw->drawWiredAdd_Box(actor->getTransformMtx(), wireframeColorInt);
			drawSets.quickDraw->drawWired_Execute(drawSets.rdest, drawSets.drawCamera->getProjView());
		}
	}

	if (actorType == sgeTypeId(ABone) && drawReason_IsEditOrSelectionTool(drawReason)) {
		if (editMode == editMode_actors) {
			drawSets.quickDraw->drawWired_Clear();

			ABone* const bone = static_cast<ABone*>(actor);
			GameWorld* world = bone->getWorld();
			float boneLength = 1.f;
			const vector_set<ObjectId>* pChildren = world->getChildensOf(bone->getId());
			if (pChildren != nullptr) {
				vec3f boneFromWs = bone->getPosition();

				const vector_set<ObjectId>& children = *pChildren;
				if (children.size() == 1) {
					Actor* child = world->getActorById(children.getNth(0));
					if (child != nullptr) {
						vec3f boneToWs = child->getPosition();
						vec3f boneDirVectorWs = (boneToWs - boneFromWs);

						if (boneDirVectorWs.lengthSqr() > 1e-6f) {
							boneLength = boneDirVectorWs.length();
							quatf rotationWs =
							    quatf::fromNormalizedVectors(vec3f(1.f, 0.f, 0.f), boneDirVectorWs.normalized0(), vec3f(0.f, 0.f, 1.f));
							vec3f translWs = boneFromWs;
							mat4f visualBoneTransformWs = mat4f::getTRS(translWs, rotationWs, vec3f(1.f));
							drawSets.quickDraw->drawWiredAdd_Bone(visualBoneTransformWs, boneDirVectorWs.length(), bone->boneLength,
							                                      wireframeColor);
						}
					}
				} else {
					for (ObjectId childId : children) {
						Actor* child = world->getActorById(childId);
						if (child != nullptr) {
							vec3f boneToWs = child->getPosition();
							drawSets.quickDraw->drawWiredAdd_Line(boneFromWs, boneToWs, wireframeColorInt);
						}
					}
				}
			}

			// Dreaw the location of the bone.
			drawSets.quickDraw->drawWiredAdd_Sphere(bone->getTransformMtx(), wireframeColorInt, boneLength / 12.f, 6);
			drawSets.quickDraw->drawWired_Execute(drawSets.rdest, drawSets.drawCamera->getProjView(), nullptr,
			                                      getCore()->getGraphicsResources().DSS_always_noTest);
		}
	}

	if ((actorType == sgeTypeId(AInvisibleRigidObstacle)) && drawReason_IsEditOrSelectionTool(drawReason)) {
		if (editMode == editMode_actors) {
			drawSets.quickDraw->drawWired_Clear();
			drawSets.quickDraw->drawWiredAdd_Box(actor->getTransformMtx(), actor->getBBoxOS(), wireframeColorInt);
			drawSets.quickDraw->drawWired_Execute(drawSets.rdest, drawSets.drawCamera->getProjView());
		}
	}

	if (actorType == sgeTypeId(AInfinitePlaneObstacle) && drawReason_IsEditOrSelectionTool(drawReason)) {
		if (editMode == editMode_actors) {
			AInfinitePlaneObstacle* plane = static_cast<AInfinitePlaneObstacle*>(actor);
			const float scale = plane->displayScale * plane->getTransform().s.componentMaxAbs();

			drawSets.quickDraw->drawWired_Clear();
			drawSets.quickDraw->drawWiredAdd_Arrow(actor->getPosition(), actor->getPosition() + actor->getDirY() * scale,
			                                       wireframeColorInt);
			drawSets.quickDraw->drawWiredAdd_Grid(actor->getPosition(), actor->getDirX() * scale, actor->getDirZ() * scale, 1, 1,
			                                      wireframeColorInt);
			drawSets.quickDraw->drawWired_Execute(drawSets.rdest, drawSets.drawCamera->getProjView());
		}
	}
}

} // namespace sge
