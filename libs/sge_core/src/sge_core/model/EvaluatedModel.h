#pragma once

#include <memory>

#include "sge_core/sgecore_api.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/math/Box.h"
#include "sge_utils/math/mat4.h"
#include "sge_utils/math/primitives.h"
#include "sge_utils/utils/vector_map.h"

#include "Model.h"
#include "sge_core/Geometry.h"

namespace sge {

struct AssetLibrary;
struct Asset;

struct EvaluatedMesh;

struct EvaluatedMaterial {
	std::string name;
	std::shared_ptr<Asset> diffuseTexture;
	std::shared_ptr<Asset> texNormalMap;
	std::shared_ptr<Asset> texMetallic;
	std::shared_ptr<Asset> texRoughness;

	vec4f diffuseColor = vec4f(1.f, 0.f, 1.f, 1.f);
	float metallic = 1.f;
	float roughness = 1.f;
};

struct EvaluatedMeshAttachment {
	EvaluatedMesh* pMesh = nullptr;
	EvaluatedMaterial* pMaterial = nullptr;
};

struct EvaluatedNode {
	mat4f evalLocalTransform = mat4f::getZero();
	mat4f evalGlobalTransform = mat4f::getIdentity();
	AABox3f aabb; // untransformed contents bounding box.
	std::vector<EvaluatedMeshAttachment> attachedMeshes;
	const char* name = nullptr;
};

struct EvaluatedMesh {
	GpuHandle<Buffer> vertexBuffer;
	GpuHandle<Buffer> indexBuffer;
	GpuHandle<Texture> skinningBoneTransfsTex;
	VertexDeclIndex vertexDeclIndex = VertexDeclIndex_Null;
	Model::Mesh* pReferenceMesh = nullptr;
	Geometry geom;
};

struct EvalMomentSets {
	EvalMomentSets() = default;
	EvalMomentSets(Model::Model* model, std::string animationName, float time = 0.f, float weight = 1.f)
	    : model(model)
	    , animationName(std::move(animationName))
	    , time(time)
	    , weight(weight) {}

	bool operator==(const EvalMomentSets& ref) const {
		return model == ref.model && animationName == ref.animationName && time == ref.time && weight == ref.weight;
	}

	Model::Model* model = nullptr; // The model that proiveds the animation.
	std::string animationName;     // The animation name for the evaluation. If none the static moment is used.
	float time = 0.f;              // The time in the animation.
	float weight = 1.f; // The weight of this "moment", if multiple moments are used their sum of weight component should be equal to 1.
};

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
struct SGE_CORE_API EvaluatedModel {
	EvaluatedModel()
	    : m_assetLibrary(NULL)
	    , m_model(NULL) {}

	void initialize(AssetLibrary* const assetLibrary, Model::Model* model);
	bool isInitialized() const { return m_model && m_assetLibrary; }

	bool evaluate(const char* const curveName, float const time);

	// Evaluates the model at the specified momemnt.
	bool evaluate(const std::vector<EvalMomentSets>& evalSets);
	bool evaluate(vector_map<const Model::Node*, mat4f>& boneGlobalTrasnformOverrides);

	// Returns true if an evaluation was performed.
	float Raycast(const Ray& ray, Model::Node** ppNode = NULL, const char* const positionSemantic = "a_position") const;

	const EvaluatedNode* findNode(const char* name) const {
		for (auto pair : m_nodes) {
			if (pair.value().name == name) {
				return &pair.value();
			}
		}

		return nullptr;
	}

  private:
	bool evaluateNodes_common();
	bool evaluateNodesFromMoments(const std::vector<EvalMomentSets>& evalSets);
	bool evaluateNodesFromExternalBones(vector_map<const Model::Node*, mat4f>& boneGlobalTrasnformOverrides);
	void buildNodeRemappingLUT(const Model::Model* otherModel);
	bool evaluateMaterials();
	bool evaluateSkinning();


  public:
	// Evaluation settings used for the current state.

	Model::Model* m_model = nullptr;
	AssetLibrary* m_assetLibrary = nullptr;

	// The evaluated state.
	vector_map<const Model::Node*, EvaluatedNode> m_nodes;
	vector_map<const Model::Mesh*, EvaluatedMesh> meshes;
	vector_map<const Model::Material*, EvaluatedMaterial> m_materials;

	vector_map<const Model::Model*, vector_map<const Model::Node*, const Model::Node*>> m_nodeRemapping;

	AABox3f aabox;
};

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void Model_FindReferencedResources(std::vector<std::string>& referencedTextures, const Model::Model& model);

} // namespace sge
