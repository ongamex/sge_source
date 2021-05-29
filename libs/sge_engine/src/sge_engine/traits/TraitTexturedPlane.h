#pragma once

#include "sge_engine/Actor.h"
#include "sge_engine/AssetProperty.h"
#include "sge_engine/enums2d.h"

namespace sge {

//-------------------------------------------------------------------
// TraitTexturedPlane
//-------------------------------------------------------------------
DefineTypeIdExists(TraitTexturedPlane);
struct SGE_ENGINE_API TraitTexturedPlane : public Trait {
	SGE_TraitDecl_Full(TraitTexturedPlane);

	TraitTexturedPlane()
	    : m_assetProperty(AssetType::Texture2D) {}

	void setTexture(const char* assetPath, bool updateNow) {
		m_assetProperty.setTargetAsset(assetPath);
		if (updateNow) {
			postUpdate();
		}
	}

	// Updates the working model.
	// Returns true if the model has been changed (no matter if it is valid or not).
	bool postUpdate() { return m_assetProperty.update(); }

	void clear() { m_assetProperty.clear(); }

	AssetProperty& getAssetProperty() { return m_assetProperty; }
	const AssetProperty& getAssetProperty() const { return m_assetProperty; }

	// mat4f getAdditionalTransform() const { return m_additionalTransform; }
	// void setAdditionalTransform(const mat4f& tr) {
	//	m_additionalTransform = tr;
	//}

	mat4f getFullWorldMtx() const {
		return getActor()->getTransformMtx(); // * m_additionalTransform;
	}

	mat4f getAnchorAlignMtxOS() const;

	// This function is designed to transform a rectangle with coordinates:
	// min (0, 0, 0) and max (0, 1, 1)
	AABox3f getBBoxOS() const;

  public:
	/// @brief Describes where the (0,0,0) point of the plane should be relative to the object.
	/// TODO: replace this with UV style coordinates.
	Anchor m_anchor = anchor_bottomMid;

	/// @brief Describes how much along the plane normal (which is X) should the plane be moved.
	/// This is useful when we want to place recoration on top of walls or floor objects.
	float m_localXOffset = 0.01f;

	/// @brief Describes how big is one pixel in world space.
	float m_pixelSizeInWorld = 0.1f;

	/// @brief Describes if any billboarding should happen for the plane.
	Billboarding m_billboarding = billboarding_none;

	/// @brief Describes the current texture to be used.
	/// TODO: add support for sprites.
	AssetProperty m_assetProperty;
};

} // namespace sge
