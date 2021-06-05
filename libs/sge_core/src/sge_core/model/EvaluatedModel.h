#pragma once

#include <memory>
#include <vector>

#include "sge_core/Geometry.h"
#include "sge_core/model/Model.h"
#include "sge_core/sgecore_api.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/math/Box.h"
#include "sge_utils/math/mat4.h"
#include "sge_utils/math/primitives.h"
#include "sge_utils/utils/vector_map.h"

namespace sge {

struct AssetLibrary;
struct Asset;

struct EvaluatedMaterial {
	std::shared_ptr<Asset> diffuseTexture;
	std::shared_ptr<Asset> texNormalMap;
	std::shared_ptr<Asset> texMetallic;
	std::shared_ptr<Asset> texRoughness;

	vec4f diffuseColor = vec4f(1.f, 0.f, 1.f, 1.f);
	float metallic = 1.f;
	float roughness = 1.f;
};

struct EvaluatedNode {
	mat4f evalLocalTransform = mat4f::getZero();
	mat4f evalGlobalTransform = mat4f::getIdentity();
	AABox3f aabbGlobalSpace; // untransformed contents bounding box.
};

struct EvaluatedMesh {
	//std::vector<mat4f> boneTransformMatrices;
	Geometry geometry;
};

struct EvalMomentSets {
	EvalMomentSets(int donorIndex = -1, int animationIndex = -1, float time = 0.f, float weight = 1.f)
	    : donorIndex(donorIndex)
	    , animationIndex(animationIndex)
	    , time(time)
	    , weight(weight) {}

	/// The donor index specified which animation donor should be used,
	/// if -1 than then no model should be used and @animationIndex point to an animation in
	/// @EvaluatedModel::m_model
	int donorIndex = -1;

	/// The animation index to be played. The animation is taken from the donor model in @donorIndex.
	int animationIndex = -1;

	/// The evaluation time of the animation
	float time = 0.f;

	/// The weight to be used of this evaluation moment. Useful when we want to blend multiple animations
	/// during transitions. For example if our character was idle and now it has just started running,
	/// we want to smoothly blend for a few miliseconds between the idle animation and the run animation.
	/// If we do not then the animation transition will not be smooth.
	/// Usually, we want the sum of all weights for all EvalMomentSets passed to EvaluatedModel::evaluateFromMoments
	/// to be 1, otherwise the whole animation will not look right.
	float weight = 1.f;
};

struct SGE_CORE_API EvaluatedModel {
	EvaluatedModel()
	    : m_assetLibrary(NULL)
	    , m_model(NULL) {}

	void initialize(AssetLibrary* const assetLibrary, Model* model);
	bool isInitialized() const { return m_model && m_assetLibrary; }

	/// Adds an animation that can be specified to the evaluate function.
	/// Returns the index of the donor.
	/// The asset will be taken form the assetLibrary specified to @initialize.
	int addAnimationDonor(const std::shared_ptr<Asset>& donorAsset);

	/// Evaluates the model at the specified momemnt.
	bool evaluateStatic() { return evaluateFromMoments(nullptr, 0); }

	/// @brief Evaluates the model with the specified animations at the specified time.
	/// If you want to evaluate the model with no animation, just pass @evalMoments nullptr and numMoments 0.
	bool evaluateFromMoments(const EvalMomentSets evalMoments[], int numMoments);

	/// @brief Evaluates the models with the specified transforms for each node (in model global space, not local).
	/// Useful for ragdolls or inverse kinematics.
	/// @param boneGlobalTrasnformOverrides an array for each node matched by the index in the array specifying the global transformation to
	/// be used.
	bool evaluateFromNodesGlobalTransform(const std::vector<mat4f>& boneGlobalTrasnformOverrides);

	int getNumEvalMeshes() const { return int(m_evaluatedMeshes.size()); }
	const EvaluatedMesh& getEvalMesh(const int iMesh) const { return m_evaluatedMeshes[iMesh]; }

	int getNumEvalMaterial() const { return int(m_evaluatedMaterials.size()); }
	const EvaluatedMaterial& getEvalMaterial(const int iMesh) const { return m_evaluatedMaterials[iMesh]; }

	int getNumEvalNodes() const { return int(m_evaluatedNodes.size()); }
	const EvaluatedNode& getEvalNode(const int iNode) const { return m_evaluatedNodes[iNode]; }

	const ModelAnimation* findAnimation(const int idxDonor, const int animIndex) const;

	Texture* getSkinningBonesTexture() const { return m_skinningBoneTransfsTex.GetPtr(); }

	int getMeshBonesOffsetInSkinningBonesTexture(const int iMesh) const {
		if (iMesh >= 0 && iMesh < m_perMeshSkinningBonesTransformOFfsetInTex.size()) {
			return m_perMeshSkinningBonesTransformOFfsetInTex[iMesh];
		}

		sgeAssert(false && "it is expected that this array was allocated");
		return -1;
	}

  private:

	bool evaluateNodes_common();
	bool evaluateFromMomentsInternal(const EvalMomentSets evalMoments[], int numMoments);
	bool evaluateNodesFromExternalBones(const std::vector<mat4f>& boneGlobalTrasnformOverrides);
	bool evaluateMaterials();
	bool evaluateSkinning();

  private:
	struct AnimationDonor {
		/// TODO: handle asset changeing. In that case the asset will still be valid,
		/// but the data in it bill be different and @originalNodeId_to_donorNodeId will be wrong and we will crash.
		std::shared_ptr<Asset> donorModel;
		std::vector<int> originalNodeId_to_donorNodeId;
	};

  public:
	Model* m_model = nullptr;
	AssetLibrary* m_assetLibrary = nullptr;

	/// Stores the state of each node, the index in this array corresponts to the index of the node in the @Model::m_nodes array.
	std::vector<EvaluatedNode> m_evaluatedNodes;

	/// @brief Since meshes can be animated only by bone transforms, which only modify their bones.
	/// We can patically evalute them only once.
	std::vector<EvaluatedMesh> m_evaluatedMeshes;

	/// @brief Since materials cannot be animated in any way,
	/// and we cannot "accept" them from a donor they can be evaluated only once.
	bool areMaterialsAlreadyEvaluated = false;
	std::vector<EvaluatedMaterial> m_evaluatedMaterials;

	std::vector<AnimationDonor> m_donors; // Caution: ckind of assumes that the AnimationDonor is moveable.

	/// A texture holding the bones for every model.
	/// The texture should be of size 4x<number-of-bones-in-all-meshes> and it must be
	/// of type RGBA float32 with point sampler.
	GpuHandle<Texture> m_skinningBoneTransfsTex;

	/// @m_skinningBoneTransfsTex stores the bones transforms for all meshes.
	/// Each mesh esh bones are placed sequentially in that matrix.
	/// This vector hold the row index of the 1st bone for the specified mesh.
	/// -1 if there are no bones for that mesh.
	std::vector<int> m_perMeshSkinningBonesTransformOFfsetInTex;

	AABox3f aabox;


	// Temporaries used to avoid allocating memory again and again for each evaluation.
	std::vector<mat4f> bonesTransformTexDataForAllMeshes;
};

} // namespace sge
