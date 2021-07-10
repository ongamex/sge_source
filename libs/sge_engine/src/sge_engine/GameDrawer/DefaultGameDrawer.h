#pragma once

#include "sge_core/shaders/ConstantColorShader.h"
#include "sge_core/shaders/SkyShader.h"
#include "sge_core/shaders/modeldraw.h"
#include "sge_engine/GameDrawer/GameDrawer.h"
#include "sge_engine/GameObject.h"
#include "sge_engine/TexturedPlaneDraw.h"
#include "sge_engine/actors/ALight.h"
#include "sge_engine/traits/TraitParticles.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/math/mat4.h"

namespace sge {

struct TraitTexturedPlane;
struct TraitModel;
struct TraitSprite;
struct TraitMultiModel;
struct TraitViewportIcon;
struct TraitPath3DFoQuickPlatform;
struct TraitRenderableGeom;
struct TraitParticles;
struct TraitParticles2;
struct ANavMesh;

struct LightShadowInfo {
	ShadowMapBuildInfo buildInfo;
	GpuHandle<Texture> pointLightDepthTexture; // This could be a single 2D or a Cube texture depending on the light source.
	GpuHandle<FrameTarget> pointLightFrameTargets[signedAxis_numElements];
	GpuHandle<FrameTarget> frameTarget; // Regular frame target for spot and directional lights.
	bool isCorrectlyUpdated = false;
};

struct SGE_ENGINE_API DefaultGameDrawer : public IGameDrawer {
	void prepareForNewFrame() final;
	void updateShadowMaps(const GameDrawSets& drawSets) final;

	void drawActor(
	    const GameDrawSets& drawSets, EditMode const editMode, Actor* actor, int const itemIndex, DrawReason const drawReason) override;

	virtual void drawWorld(const GameDrawSets& drawSets, const DrawReason drawReason) override;

	// A Legacy function that should end up not being used.
	void drawActorLegacy(Actor* actor,
	                     const GameDrawSets& drawSets,
	                     EditMode const editMode,
	                     int const itemIndex,
	                     const DrawReasonInfo& generalMods,
	                     DrawReason const drawReason);

	void drawTraitTexturedPlane(TraitTexturedPlane* traitTexPlane,
	                            const GameDrawSets& drawSets,
	                            const DrawReasonInfo& generalMods,
	                            DrawReason const drawReason);

	void drawTraitViewportIcon(TraitViewportIcon* viewportIcon, const GameDrawSets& drawSets, DrawReason const drawReason);

	void drawTraitModel(TraitModel* modelTrait,
	                    const GameDrawSets& drawSets,
	                    const DrawReasonInfo& generalMods,
	                    DrawReason const drawReason);

	void drawTraitSprite(TraitSprite* modelTrait,
	                     const GameDrawSets& drawSets,
	                     const DrawReasonInfo& generalMods,
	                     DrawReason const drawReason);

	void drawTraitMultiModel(TraitMultiModel* multiModelTrait,
	                         const GameDrawSets& drawSets,
	                         const DrawReasonInfo& generalMods,
	                         DrawReason const drawReason);

	void drawTraitRenderableGeom(TraitRenderableGeom* ttRendGeom, const GameDrawSets& drawSets, const DrawReasonInfo& generalMods);

	void drawTraitParticles(TraitParticles* particlesTrait, const GameDrawSets& drawSets, DrawReasonInfo generalMods);

	void drawTraitParticles2(TraitParticles2* particlesTrait, const GameDrawSets& drawSets, DrawReasonInfo generalMods);

	void drawANavMesh(ANavMesh* navMesh,
	                  const GameDrawSets& drawSets,
	                  const DrawReasonInfo& generalMods,
	                  DrawReason const drawReason,
	                  const uint32 wireframeColor);

  private:
	bool isInFrustum(const GameDrawSets& drawSets, Actor* actor) const;
	void fillGeneralModsWithLights(Actor* actor, DrawReasonInfo& generalMods);

  public:
	BasicModelDraw m_modeldraw;
	ConstantColorWireShader m_constantColorShader;
	TexturedPlaneDraw m_texturedPlaneDraw;
	ParticleRenderDataGen m_partRendDataGen;

	std::vector<ShadingLightData> m_shadingLights;
	std::vector<const ShadingLightData*> m_shadingLightPerObject;

	// TODO: find a proper place for this
	std::map<ObjectId, LightShadowInfo> m_perLightShadowFrameTarget;

	SkyShader m_skyShader;
};

} // namespace sge
