#pragma once

#include <fbxsdk.h>

#include "ModelParseSettings.h"
#include "sge_core/model/Model.h"
#include "sge_utils/math/transform.h"

namespace sge {

struct IAssetRelocationPolicy;

bool InitializeFBXSDK();

//---------------------------------------------------------------
//
//---------------------------------------------------------------
struct FBXSDKParser {
	// Prases the input FBX scene and produces a ready to save SGE model.
	// @param enforcedRootNode is the node to be used as a root node insted of the actual one, if null, the regular root is going to be
	// used.
	bool parse(Model::Model* result,
	           std::vector<std::string>* pReferencedTextures,
	           fbxsdk::FbxScene* scene,
	           FbxNode* enforcedRootNode,
	           const ModelParseSettings& parseSettings);

  private:
	// Step 0 (new)
	int discoverNodesRecursive(fbxsdk::FbxNode* const fbxNode);
	void importMaterials();
	void importMeshes(const bool importSkinningData);
	void importMeshes_singleMesh(FbxMesh* fbxMesh, int importedMeshIndex, const bool importSkinningData);
	void importNodes();
	void importNodes_singleNode(FbxNode* fbxNode, int importNodeIndex);
	void importAnimations();
	void importCollisionGeometry();

  private:
	/// The fbx scene being imported.
	fbxsdk::FbxScene* m_fbxScene = nullptr;
	ModelParseSettings m_parseSettings;
	std::vector<std::string>* m_pReferencedTextures = nullptr;

	std::map<FbxSurfaceMaterial*, int> m_fbxMtl2MtlIndex;
	std::map<FbxMesh*, int> m_fbxMesh2MeshIndex;
	std::map<FbxNode*, int> m_fbxNode2NodeIndex;

	/// Meshes to be used for collision geometry (if any). These do not participate in the m_fbxMesh2MeshIndex
	/// as they aren't going to be used for rendering.
	std::map<FbxMesh*, std::vector<transf3d>> m_collision_ConvexHullMeshes;
	std::map<FbxMesh*, std::vector<transf3d>> m_collision_BvhTriMeshes;
	std::map<FbxMesh*, std::vector<transf3d>> m_collision_BoxMeshes;
	std::map<FbxMesh*, std::vector<transf3d>> m_collision_CaplsuleMeshes;
	std::map<FbxMesh*, std::vector<transf3d>> m_collision_CylinderMeshes;
	std::map<FbxMesh*, std::vector<transf3d>> m_collision_SphereMeshes;

	// When enforcing a root node we need to remove it's transfrom from the collsion objects.
	transf3d m_collision_transfromCorrection = transf3d::getIdentity();

	/// The model that is going to store the imported result.
	Model::Model* m_model = nullptr;
};

} // namespace sge
