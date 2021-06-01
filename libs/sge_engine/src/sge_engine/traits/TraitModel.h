#pragma once

#include "sge_core/Animator.h"
#include "sge_core/shaders/modeldraw.h"
#include "sge_engine/Actor.h"
#include "sge_engine/AssetProperty.h"
#include "sge_engine/enums2d.h"


namespace sge {

struct ICamera;

/// @brief TraitModel is a trait designed to be attached in an Actor.
/// It provides a simple way to assign a renderable 3D Model to the game object (both animated and static).
/// The trait is not automatically updateable, the user needs to manually call @postUpdate() method in their objects.
/// This is because not all game object need this complication of auto updating.
///
/// For Example:
///
///    Lets say that you have a actor that is some collectable, a coin from Super Mario.
///    That coin 3D model is never going to change, you know that the game object is only going to use
///    that one specfic 3D model that you can set in @Actor::create method with @TraitModel::setModel() and forget about it.
///    You do not need any upadates on it, nor you want to be able to change the 3D model from the Property Editor Window.
///    In this situation you just add the trait, set the model and you are done.
///
///    The situation where the 3D model might change is for example with some Decore.
///    Lets say that your 3D artist has prepared a grass and bush models that you want to scatter around the level.
///    It would be thedious to have a separate game object for each 3D model.
///    Instead you might do what the @AStaticObstacle does, have the model be changed via Propery Editor Window.
///    In that way you have I generic actor type that could be configured to your desiers.
///    In order for the game object to take into account the change you need in your Actor::postUpdate to
///    to update the trait, see if the model has been changed and maybe update the rigid body for that actor.
DefineTypeIdExists(TraitModel);
struct SGE_ENGINE_API TraitModel : public Trait {
	SGE_TraitDecl_Full(TraitModel);

	TraitModel()
	    : m_assetProperty(AssetType::Model, AssetType::Texture2D, AssetType::Sprite) {}

	void setModel(const char* assetPath, bool updateNow) {
		m_assetProperty.setTargetAsset(assetPath);
		if (updateNow) {
			postUpdate();
		}
	}

	void setModel(std::shared_ptr<Asset>& asset, bool updateNow) {
		m_assetProperty.setAsset(asset);
		m_evalModel = NullOptional();
		if (updateNow) {
			updateAssetProperty();
		}
	}

	/// Not called automatically see the class comment above.
	/// Updates the working model.
	/// Returns true if the model has been changed (no matter if it is valid or not).
	bool postUpdate() { return updateAssetProperty(); }

	/// Invalidates the asset property focing an update.
	void clear() { m_assetProperty.clear(); }

	AssetProperty& getAssetProperty() { return m_assetProperty; }
	const AssetProperty& getAssetProperty() const { return m_assetProperty; }

	mat4f getAdditionalTransform() const { return m_additionalTransform; }
	void setAdditionalTransform(const mat4f& tr) { m_additionalTransform = tr; }

	AABox3f getBBoxOS() const;

	void setRenderable(bool v) { isRenderable = v; }
	bool getRenderable() const { return isRenderable; }
	void setNoLighting(bool v) { instanceDrawMods.forceNoLighting = v; }
	bool getNoLighting() const { return instanceDrawMods.forceNoLighting; }

	void computeNodeToBoneIds();
	void computeSkeleton(std::vector<mat4f>& boneOverrides);

  private:
	bool updateAssetProperty() {
		if (m_assetProperty.update()) {
			m_evalModel = NullOptional();
			return true;
		}
		return false;
	}

	void onModelChanged() {
		useSkeleton = false;
		rootSkeletonId = ObjectId();
		nodeToBoneId.clear();
	}

  public:
	struct MaterialOverride {
		std::string materialName;
		ObjectId materialObjId;
	};

	mat4f m_additionalTransform = mat4f::getIdentity();
	AssetProperty m_assetProperty;

	Optional<EvaluatedModel> m_evalModel;

	// 3D model specific properties.
	// TODO: move them in a strcuture.
	InstanceDrawMods instanceDrawMods;
	bool isRenderable = true;
	std::vector<MaterialOverride> m_materialOverrides;

	// External skeleton, useful for IK. Not sure for regular skinned meshes.
	bool useSkeleton = false;
	ObjectId rootSkeletonId;
	std::unordered_map<int, ObjectId> nodeToBoneId;

	/// @brief A struct holding the rendering options of a sprite or a texture in 3D.
	struct ImageSettings {
		/// @brief Computes the object-to-node transformation of the sprite so i has its origin in the deisiered location.
		mat4f getAnchorAlignMtxOS(float imageWidth, float imageHeight) const {
			const float sz = imageWidth / m_pixelsPerUnit;
			const float sy = imageHeight / m_pixelsPerUnit;

			const mat4f anchorAlignMtx = anchor_getPlaneAlignMatrix(m_anchor, vec2f(sz, sy));
			return anchorAlignMtx;
		}

		/// @brief Computes the matrix that needs to be applied to a quad
		/// that faces +X and has corners (0,0,0) and (0,1,1), So it appears as described by this structure:
		/// its size, orientation (billboarding), aligment and so on.
		/// @param [in] asset is the asset that is going to be attached to the plane. it needs to be a texture or a sprite.
		/// @param [in] drawCamera the camera that is going to be used for rendering. If null the billboarding effect will be missing.
		/// @param [in] nodeToWorldTransform the transform that indicates the location of the object(for example, could be the transform
		/// of an actor).
		/// @param [in] additionaTransform an additional transform to be applied before before all other transforms.
		/// @return the matrix to be used as object-to-world transform form the quad described above.
		mat4f computeObjectToWorldTransform(const Asset& asset,
		                                    const ICamera* const drawCamera,
		                                    const transf3d& nodeToWorldTransform,
		                                    const mat4f& additionaTransform) const;

		AABox3f computeBBoxOS(const Asset& asset, const mat4f& additionaTransform) const;

		/// Adds a color tint to the final color and the alpha of the object.
		vec4f colorTint = vec4f(1.f);

		// Sprite and texture drawing.
		/// @brief Describes where the (0,0,0) point of the plane should be relative to the object.
		/// TODO: replace this with UV style coordinates.
		Anchor m_anchor = anchor_bottomMid;

		/// @brief Describes how much along the plane normal (which is X) should the plane be moved.
		/// This is useful when we want to place recoration on top of walls or floor objects.
		float m_localXOffset = 0.01f;

		/// @brief Describes how big is one pixel in world space.
		float m_pixelsPerUnit = 64.f;

		/// @brief Describes if any billboarding should happen for the plane.
		Billboarding m_billboarding = billboarding_none;

		/// @brief if true the image will get rendered with no lighting applied,
		/// as if we just rendered the color (with gamma correction and post processing).
		bool forceNoLighting = true;

		/// @brief if true the plane will not get any culling applied. Useful if we want the
		/// texture to be visible from both sides.
		bool forceNoCulling = true;

		/// @brief If true, the sprite plane will, by default look towards +Z, otherwise it will be facing +X.
		bool defaultFacingAxisZ = true;

		/// Flips the sprite horizontally while maintaining its origin at the same place.
		/// Useful if we are building a 2D game where the character is a sprite that can walk left and right.
		bool flipHorizontally = false;

		/// Sprite (if any) evaluation time in seconds.
		float spriteFrameTime = 0.f;
	};

	ImageSettings imageSettings;
};

} // namespace sge
