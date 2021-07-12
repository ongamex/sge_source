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

#include "sge_engine/traits/TraitModelRenderItem.h"
#include "sge_engine/traits/TraitSpriteRrenderItem.h"
#include "sge_engine/traits/TraitViewportIconRenderItem.h"

namespace sge {

struct TraitRenderableGeom;
struct TraitParticles;
struct TraitParticles2;
struct ANavMesh;

struct LightShadowInfo {
	ShadowMapBuildInfo buildInfo;
	GpuHandle<Texture> pointLightDepthTexture; ///< This could be a single 2D or a Cube texture depending on the light source.
	GpuHandle<FrameTarget> pointLightFrameTargets[signedAxis_numElements]; ///< FrameTarget used specifically for rendering point light shadow maps.
	GpuHandle<FrameTarget> frameTarget; ///< Regular frame target for spot and directional lights.
	bool isCorrectlyUpdated = false;
};

struct SGE_ENGINE_API DefaultGameDrawer : public IGameDrawer {
	void prepareForNewFrame() final;
	void updateShadowMaps(const GameDrawSets& drawSets) final;

	void getRenderItemsForActor(const GameDrawSets& drawSets, const SelectedItemDirect& item, DrawReason drawReason);
	void drawItem(const GameDrawSets& drawSets, const SelectedItemDirect& item, DrawReason drawReason) override;
	void drawWorld(const GameDrawSets& drawSets, const DrawReason drawReason) override;
	void drawSky(const GameDrawSets& drawSets, const DrawReason drawReason);
	void drawCurrentRenderItems(const GameDrawSets& drawSets, DrawReason drawReason, bool shouldDrawSky);

	void drawRenderItem_TraitModel(TraitModelRenderItem& ri,
	                               const GameDrawSets& drawSets,
	                               const DrawReasonInfo& generalMods,
	                               DrawReason const drawReason);

	void drawRenderItem_TraitSprite(const TraitSpriteRenderItem& ri,
	                                const GameDrawSets& drawSets,
	                                const DrawReasonInfo& generalMods,
	                                DrawReason const drawReason);

	void drawRenderItem_TraitViewportIcon(TraitViewportIconRenderItem& viewportIcon,
	                                      const GameDrawSets& drawSets,
	                                      DrawReason const drawReason);


	// Legacy Functions to be removed:

	void drawActor(const GameDrawSets& drawSets, EditMode const editMode, Actor* actor, int const itemIndex, DrawReason const drawReason);

	// A Legacy function that should end up not being used.
	void drawActorLegacy(Actor* actor,
	                     const GameDrawSets& drawSets,
	                     EditMode const editMode,
	                     int const itemIndex,
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
	/// @brief Returns true if the bounding box of the actor (Actor::getBBoxOs) is in the specified
	/// draw camera frustum.
	bool isInFrustum(const GameDrawSets& drawSets, Actor* actor) const;
	void fillGeneralModsWithLights(Actor* actor, DrawReasonInfo& generalMods);

	void clearRenderItems() {
		m_RIs_opaque.clear();
		m_RIs_alphaSorted.clear();

		m_RIs_traitModel.clear();
		m_RIs_traitSprite.clear();
		m_RIs_traitViewportIcon.clear();
	}

  public:
	BasicModelDraw m_modeldraw;
	ConstantColorWireShader m_constantColorShader;
	SkyShader m_skyShader;
	TexturedPlaneDraw m_texturedPlaneDraw;
	ParticleRenderDataGen m_partRendDataGen;

	std::vector<ShadingLightData> m_shadingLights;
	std::vector<const ShadingLightData*> m_shadingLightPerObject;

	// TODO: find a proper place for this
	std::map<ObjectId, LightShadowInfo> m_perLightShadowFrameTarget;

	/// Render items cache variables.
	/// These could safely be a member variable in @drawWorld or @drawItem
	/// However in order to save up from alocating them again and again for each frame
	/// We store the containers here.
	
	std::vector<IRenderItem*> m_RIs_opaque; ///< Pointers to all opaque render items. Pointers are not owned.
	std::vector<IRenderItem*> m_RIs_alphaSorted; ///< Pointers to all semi-transparent render items. Pointers are not owned.

	// Render items cache to save up on memory.
	std::vector<TraitModelRenderItem> m_RIs_traitModel;
	std::vector<TraitSpriteRenderItem> m_RIs_traitSprite;
	std::vector<TraitViewportIconRenderItem> m_RIs_traitViewportIcon;
};

} // namespace sge
