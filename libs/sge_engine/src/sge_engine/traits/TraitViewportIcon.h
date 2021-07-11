#pragma once

#include "sge_engine/Actor.h"
#include "sge_engine/AssetProperty.h"
#include "sge_engine/enums2d.h"

namespace sge {

struct ICamera;
struct TraitViewportIconRenderItem;

///-----------------------------------------------------------------------
/// A viewport icon, always facing the camera represnting an object.
/// These icons are only visible in edit mode.
DefineTypeIdExists(TraitViewportIcon);
struct SGE_ENGINE_API TraitViewportIcon : public Trait {
	SGE_TraitDecl_Full(TraitViewportIcon);

	TraitViewportIcon();

	void setTexture(const char* assetPath, bool updateNow) {
		m_assetProperty.setTargetAsset(assetPath);
		if (updateNow) {
			postUpdate();
		}
	}

	void setTexture(PAsset asset, bool updateNow) {
		m_assetProperty.setAsset(asset);
		if (updateNow) {
			postUpdate();
		}
	}

	void setObjectSpaceOffset(const vec3f& offset) { m_objectSpaceIconOffset = offset; }

	// Updates the texture model.
	// Returns true if the model has been changed (no matter if it is valid or not).
	bool postUpdate() { return m_assetProperty.update(); }

	/// Computes the node-to-world transform of the icon, taking into account the billboarding.
	/// @param [in] camera to be used for computing the billboarding orientation.
	mat4f computeNodeToWorldMtx(const ICamera& camera) const;

	/// Returns the loaded icon texture, nullptr is returned if none is loaded.
	Texture* getIconTexture() const;

	void getRenderItems(std::vector<TraitViewportIconRenderItem>& renderItems);

  private:
	float m_pixelSizeUnitsScreenSpace = 0.001f;
	vec3f m_objectSpaceIconOffset = vec3f(0.f);
	AssetProperty m_assetProperty;
};
} // namespace sge
