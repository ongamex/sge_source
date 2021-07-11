#pragma once

#include "sge_core/Animator.h"
#include "sge_core/shaders/modeldraw.h"
#include "sge_engine/Actor.h"
#include "sge_engine/AssetProperty.h"
#include "sge_engine/GameDrawer/IRenderItem.h"


namespace sge {

struct ICamera;
struct TraitModel;

struct TraitModelRenderItem : public IRenderItem {
	TraitModel* traitModel = nullptr;
	int iModel = -1;
	const EvaluatedModel* evalModel = nullptr;

	int iEvalNode = -1; // The mesh to be rendered from the model.
	int iEvalNodeMechAttachmentIndex = -1;
};

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
///    In that way you have a generic actor type that could be configured to your desiers.
///    In order for the game object to take into account the change you need in your Actor::postUpdate to
///    to update the trait, see if the model has been changed and maybe update the rigid body for that actor.
DefineTypeIdExists(TraitModel);
struct SGE_ENGINE_API TraitModel : public Trait {
	SGE_TraitDecl_Full(TraitModel);

	TraitModel() = default;

	void setModel(const char* assetPath, bool updateNow);
	void setModel(std::shared_ptr<Asset>& asset, bool updateNow);

	void addModel(const char* assetPath, bool updateNow);
	void addModel(std::shared_ptr<Asset>& asset, bool updateNow);

	/// Not called automatically see the class comment above.
	/// Updates the working models.
	/// Returns true if a model has been changed (no matter if it is valid or not).
	bool postUpdate() { return updateAssetProperties(); }

	AABox3f getBBoxOS() const;

	void getRenderItems(std::vector<IRenderItem*>& renderItems);

	void invalidateCachedAssets() {
		for (PerModelSettings& modelSets : m_models) {
			modelSets.invalidateCachedAssets();
		}
	}


  private:
	bool updateAssetProperties();

  public:
	struct MaterialOverride {
		std::string materialName;
		ObjectId materialObjId;
	};

	struct PerModelSettings {
		PerModelSettings()
		    : m_assetProperty(AssetType::Model) {}

		/// Invalidates the asset property focing an update.
		void invalidateCachedAssets() { m_assetProperty.clear(); }

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

		void setNoLighting(bool v) { instanceDrawMods.forceNoLighting = v; }
		bool getNoLighting() const { return instanceDrawMods.forceNoLighting; }

		void setModel(const char* assetPath, bool updateNow);
		void setModel(std::shared_ptr<Asset>& asset, bool updateNow);

		AABox3f getBBoxOS() const;

		void computeNodeToBoneIds(TraitModel& ownerTraitModel);
		void computeSkeleton(TraitModel& ownerTraitModel, std::vector<mat4f>& boneOverrides);

		bool isRenderable = true;
		AssetProperty m_assetProperty;
		mat4f m_additionalTransform = mat4f::getIdentity();

		// Used when the trait is going to render an animated model.
		// This holds the evaluated 3D model to be rendered.
		// If null the static EvaluatedModel of the asset is going to get rendered.
		Optional<EvaluatedModel> m_evalModel;
		InstanceDrawMods instanceDrawMods;

		std::vector<MaterialOverride> m_materialOverrides;

		// External skeleton, useful for IK. Not sure for regular skinned meshes.
		bool useSkeleton = false;
		ObjectId rootSkeletonId;
		std::unordered_map<int, ObjectId> nodeToBoneId;
	};

  public:
	bool isRenderable = true;               ///< True if the whole trait is renderable.
	bool isFixedModelsSize = true;          ///< if true the interface will not offer adding/removing more models to the trait.
	std::vector<PerModelSettings> m_models; ///< A list of all models in their settings to rendered by the trait.

	std::vector<TraitModelRenderItem> m_tempRenderItems;
};

} // namespace sge
