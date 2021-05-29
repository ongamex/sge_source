
#include <vector>
#include <string>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "sge/renderer/GraphicsCommon.h"
#include "sge/utils/json.h"
#include "sge/math/mat4.h"
#include "sge/math/Box.h"

#include "sge/model/Model.h"

#include "Common.h"

namespace sge {

//---------------------------------------------------------------------
// AssimpSceneParser
//---------------------------------------------------------------------
struct AssimpSceneParser
{
	void parse(
		Model* result,
		const aiScene* scene, 
		const ModelParseSettings& settings,
		std::vector<std::string>* pReferencedTextures);

private : 

	// The ASSIMP scene parsing has been broken down in few ORDERED steps:

	// Step 1: Loades all materials represented by ASSIMP scene.
	void ParseMaterials(std::vector<std::string>* pReferencedTextures);

	// Step 2: Loads a mesh from ASSIMP. Take a note that this step is done before Node loading,
	// becase of that the bones node pointers(Bones are basically Nodes...) aren't resolved. 
	// This is done after all the nodes are loaded using the "bonesToResolve" container.
	void ParseMesh(aiMesh* impMesh);
	
	// Step 3: Loades all the child nodes of the input node.
	Model::Node* ParseNodesRecursive(const aiNode* const impNode); 

	// Step 4: Resolve the Bone noodes pointers form step 2.
	void ResolveBonesNodePointer();

	// Step 5: Load all available animations for every node(aka Parameter Curves).
	void ParseAnimations();
	
	// Helper functions :

	// Searches for existing suitable mesh data that will be used for storing a newly loaded mesh.
	// If not new MeshData is created.
	Model::MeshData* GetBestSuitableMeshData(aiMesh* impMesh);

	// Searches for a direct mesh representation from ASSIMP in all loaded meshes.
	Model::Mesh* FindMesh(aiMesh* impMesh);

	// Searches for a scene node by name.
	Model::Node* FindNodeByName(const char const* nodeName);

	int getNextId() { return m_nextFreeId++; }

private :

	// Import settings.
	ModelParseSettings settings;
	const aiScene* impScene = NULL; 
	Model* model = NULL;

	int m_nextFreeId = 0;

	// Import state vairables.
	
	// Pointer from m_meshesData, just to simplify GetBestSuitableMeshData.
	Model::MeshData* cachedWholeSceneMeshData = nullptr; 

	// An assocication container form ASSIMP scene componets to 'ours'.
	std::map<aiMesh*, Model::Mesh*> iaMeshToMesh;
	std::map<aiMaterial*, Model::Material*> iaMaterialToMaterial;

	// Mesh naming in assimp is optional, additionally, one name may be used acoss multiple nodes.
	std::map<std::string, int> meshNamesInAssimpScene;

	// The container used in ResolveBonesNodePointer to resolve bone nodes pointers.
	// [The pointer to the target bone; the name of the node]
	// CAUTION:: BTW there cuold be multiple nodes with the same name in assimp, 
	// but this is how they describe bones.
	// I guess they make sure that bones have unique names?
	std::map<Model::Bone*, std::string> bonesToResolve;
};

}
