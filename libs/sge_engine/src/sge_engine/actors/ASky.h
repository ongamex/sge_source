#pragma once

#include "sge_core/shaders/SkyShader.h"
#include "sge_engine/Actor.h"
#include "sge_engine/traits/TraitCustomAE.h"
#include "sge_engine/traits/TraitViewportIcon.h"

namespace sge {

struct ASky : public Actor, public IActorCustomAttributeEditorTrait {
	ASky()
	    : m_textureAssetProp(AssetType::Texture2D) {}

	AABox3f getBBoxOS() const override { return AABox3f(); }

	void create() override;
	void postUpdate(const GameUpdateSets& updateSets) override;

	// IActorCustomAttributeEditorTrait
	void doAttributeEditor(GameInspector* inspector) override;

	const SkyShaderSettings& getSkyShaderSettings();

	SkyShaderSettings getSkyShaderSetting() const;

  public:
	TraitViewportIcon ttViewportIcon;

	SkyShaderSettings::Mode m_mode = SkyShaderSettings::mode_colorGradinet;

	vec3f m_topColor = vec3f(0.259f, 0.814f, 1.f);
	vec3f m_bottomColor = vec3f(1.000f, 0.739f, 0.409f);

	AssetProperty m_textureAssetProp;
};

} // namespace sge
