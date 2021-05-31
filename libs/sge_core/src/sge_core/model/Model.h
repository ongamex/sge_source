#pragma once

#include <string>

#include "sge_core/sgecore_api.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/math/Box.h"
#include "sge_utils/math/mat4.h"
#include "sge_utils/math/primitives.h"
#include "sge_utils/math/transform.h"
#include "sge_utils/utils/ChunkContainer.h"

#include "CollisionMesh.h"

namespace sge {

struct Model_CollisionShapeBox {
	Model_CollisionShapeBox() = default;
	Model_CollisionShapeBox(std::string name, transf3d transform, vec3f halfDiagonal)
	    : name(std::move(name))
	    , transform(transform)
	    , halfDiagonal(halfDiagonal) {}

	std::string name;
	transf3d transform = transf3d::getIdentity();
	vec3f halfDiagonal = vec3f(0.f);
};

/// Represents a capsule. The capsule is defined by all points that are at distace "radius" from
/// the line defined by ({0.f, -halfHeight, 0.f}, {0.f, halfHeight, 0.f}}
struct Model_CollisionShapeCapsule {
	Model_CollisionShapeCapsule() = default;
	Model_CollisionShapeCapsule(std::string name, transf3d transform, float halfHeight, float radius)
	    : name(std::move(name))
	    , transform(transform)
	    , halfHeight(halfHeight)
	    , radius(radius) {}

	std::string name;
	transf3d transform = transf3d::getIdentity();
	float halfHeight = 0.f;
	float radius = 0.f;
};

struct Model_CollisionShapeCylinder {
	Model_CollisionShapeCylinder() = default;
	Model_CollisionShapeCylinder(std::string name, transf3d transform, vec3f halfDiagonal)
	    : name(std::move(name))
	    , transform(transform)
	    , halfDiagonal(halfDiagonal) {}

	std::string name;
	transf3d transform = transf3d::getIdentity();
	vec3f halfDiagonal = vec3f(0.f);
};

struct Model_CollisionShapeSphere {
	Model_CollisionShapeSphere() = default;
	Model_CollisionShapeSphere(std::string name, transf3d transform, float radius)
	    : name(std::move(name))
	    , transform(transform)
	    , radius(radius) {}

	std::string name;
	transf3d transform = transf3d::getIdentity();
	float radius = 0.f;
};

/// Loading settings describing how the model should be loaded.
struct ModelLoadSettings {
	/// The directory of the file, used for loading asset. Needed when the model references other assets like textures.
	std::string assetDir;
};

struct ModelMaterial {
	std::string name;

	vec4f diffuseColor = vec4f(1.f);
	vec4f emissionColor = vec4f(0.f);
	float metallic = 0.f;
	float roughness = 1.f;

	std::string diffuseTextureName;
	std::string emissionTextureName;
	std::string normalTextureName;
	std::string metallicTextureName;
	std::string roughnessTextureName;
};

// Skinning bone.
struct ModelMeshBone {
	ModelMeshBone() = default;
	ModelMeshBone(const mat4f& offsetMatrix, int nodeIdx)
	    : offsetMatrix(offsetMatrix)
	    , nodeIdx(nodeIdx) {}

	mat4f offsetMatrix = mat4f::getIdentity(); ///< The transformation matrix used for binding a bone to a mesh.
	int nodeIdx = -1;                          ///< The index of the node representing this bone transformation.
};

struct SGE_CORE_API ModelMesh {
	std::string name; ///< The name of the mesh.

	PrimitiveTopology::Enum primitiveTopology = PrimitiveTopology::Unknown;
	int vbByteOffset = 0;                           ///< 1st vertex byte offset into the vertex buffer
	int ibByteOffset = 0;                           ///< 1st index byte offse int the index buffer
	UniformType::Enum ibFmt = UniformType::Unknown; ///< The format the index buffer, if unknown this mesh doesn't use index buffers.
	int numElements = 0;                            ///< The number of vertices/indices used by this mesh.
	int numVertices = 0;                            ///< The number of vertices in the mesh.
	std::vector<VertexDecl> vertexDecl;             ///< The vertex declaration

	int stride = 0;                    ///< The byte size stride of a single vertex.
	int vbVertexColorOffsetBytes = -1; ///, The byte offset of the vertex color.
	int vbPositionOffsetBytes = -1;    ///< The byte offset in the stride of vertex position.
	int vbNormalOffsetBytes = -1;      ///< The byte offset in the stride of the vertex normal.
	int vbTangetOffsetBytes = -1;
	int vbBinormalOffsetBytes = -1;
	int vbUVOffsetBytes = -1; ///< The byte offset in the stride of the 1st vertex UV channel.
	int vbBonesIdsBytesOffset = -1;
	int vbBonesWeightsByteOffset = -1;

	std::vector<char> vertexBufferRaw; ///< The raw data containing all vertices in the vertex buffer,
	std::vector<char> indexBufferRaw;  ///< The raw data containing all indices in the vertex buffer,

	AABox3f aabox; ///< The bounding box around the vertices of the mesh, without any deformation by skinning or anything else.

	std::vector<ModelMeshBone> bones; ///< A list of bones affecting the mesh.

	// The member below are available only if @Model::createRenderingResources gets called:

	GpuHandle<Buffer> vertexBuffer;    ///< The vertex buffer to be used for rendering of that mesh.
	GpuHandle<Buffer> indexBuffer;     ///< The index buffer to be used for rendering of that mesh.
	bool hasUsableTangetSpace = false; ///< True if uv and all 3 tangent space vectors are available in the vertex declaration.
	VertexDeclIndex vertexDeclIndex = VertexDeclIndex_Null;
};

struct MeshAttachment {
	MeshAttachment() = default;

	MeshAttachment(int attachedMeshIndex, int attachedMaterialIndex)
	    : attachedMeshIndex(attachedMeshIndex)
	    , attachedMaterialIndex(attachedMaterialIndex) {}

	int attachedMeshIndex = -1;
	int attachedMaterialIndex = -1;
};

struct KeyFrames {
	std::map<float, vec3f> positionKeyFrames;
	std::map<float, quatf> rotationKeyFrames;
	std::map<float, vec3f> scalingKeyFrames;

	void evaluate(transf3d& result, const float t) const;
};

struct ModelAnimation {
	ModelAnimation() = default;

	ModelAnimation(std::string animationName, float durationSec, std::map<int, KeyFrames> perNodeKeyFrames)
	    : animationName(std::move(animationName))
	    , durationSec(durationSec)
	    , perNodeKeyFrames(std::move(perNodeKeyFrames)) {}

	bool evaluateForNode(transf3d& outTransform, const int nodeIndex, const float time) const {
		auto itr = perNodeKeyFrames.find(nodeIndex);
		if (itr != perNodeKeyFrames.end()) {
			itr->second.evaluate(outTransform, time);
			return true;
		}
		return false;
	}

  public:
	/// The name of the animation.
	std::string animationName;

	/// The lenght of the animation in seconds.. Keep in mind that there might be key frames beyond that value,
	/// The artist might need them for interpolation purposes.
	float durationSec = 0;

	/// The keyframes of all affected nodes in their local space (relative to their parents).
	std::map<int, KeyFrames> perNodeKeyFrames;
};

struct ModelNode {
	/// int nodeIdx = -1; // The "id" used to identify this piece of data in the file.
	transf3d staticLocalTransform; ///< The local transformation of the node when not animated.

	std::vector<MeshAttachment> meshAttachments;
	std::vector<int> childNodes; ///< The indices of all child nodes.
	float limbLength = 0.f; ///< If the node seemed to be used for skinning, this is the length of the bone in the interface. 0 otherwise.
	std::string name; ///< The name of the node.
};

/// @brief Model hold a serialized/deserialized state of our internal 3D model file format,
/// usually imported from obj/fbx/others.
/// In addition as some of the data may not change during runtime the structure hold that immutable data
/// like the GPUHandles to the vertex buffers needed.
/// To render and/or play animations on that model use @EvaluatedModel.
struct SGE_CORE_API Model {
	/// @brief If the model is going to be used for rendering, you need to call this function
	/// after the model is fully loaded. It will initialize vertex/index buffer or other needed data.
	void createRenderingResources(SGEDevice& sgedev);

	int makeNewNode();
	int makeNewMaterial();
	int makeNewMesh();
	int makeNewAnim();
	void setRootNodeIndex(const int newRootNodeIndex);

	int getRootNodeIndex() const { return m_rootNodeIndex; }
	ModelNode* getRootNode() { return nodeAt(getRootNodeIndex()); }
	const ModelNode* getRootNode() const { return nodeAt(getRootNodeIndex()); }

	int numNodes() const { return int(m_nodes.size()); }
	ModelNode* nodeAt(int nodeIndex);
	const ModelNode* nodeAt(int nodeIndex) const;
	int findFistNodeIndexWithName(const std::string& name) const;

	int numMaterials() const { return int(m_materials.size()); }
	ModelMaterial* materialAt(int materialIndex);
	const ModelMaterial* materialAt(int materialIndex) const;

	int numMeshes() const { return int(m_meshes.size()); }
	ModelMesh* meshAt(int meshIndex);
	const ModelMesh* meshAt(int meshIndex) const;

	int numAnimations() const { return int(m_animations.size()); }
	const ModelAnimation* animationAt(int iAnim) const;
	ModelAnimation* animationAt(int iAnim);
	const ModelAnimation* getAnimationByName(const std::string& name) const;
	int getAnimationIndexByName(const std::string& name) const;

	const std::vector<ModelNode*>& getNodes() { return m_nodes; }
	const std::vector<ModelMesh*>& getMeshes() { return m_meshes; }
	const std::vector<ModelMaterial*>& getMatrials() { return m_materials; }

	void setModelLoadSettings(const ModelLoadSettings& loadSets) { m_loadSets = loadSets; }
	const ModelLoadSettings& getModelLoadSetting() const { return m_loadSets; }

  private:
	int m_rootNodeIndex = -1;
	std::vector<ModelAnimation> m_animations;
	std::vector<ModelNode*> m_nodes;
	std::vector<ModelMesh*> m_meshes;
	std::vector<ModelMaterial*> m_materials;

	/// The actual storage for the model data.
	ChunkContainer<ModelMesh> m_containerMesh;
	ChunkContainer<ModelMaterial> m_containerMaterial;
	ChunkContainer<ModelNode> m_containerNode;

	/// Cached loading settings.
	ModelLoadSettings m_loadSets;

  public: // TODO: make this private.
	/// Stores the various collision shapes defineded in the model.
	/// They are useful for having one place for the rendering geometry and the
	/// geometry that is going to be used for game physics.
	std::vector<ModelCollisionMesh> m_convexHulls;
	std::vector<ModelCollisionMesh> m_concaveHulls;
	std::vector<Model_CollisionShapeBox> m_collisionBoxes;
	std::vector<Model_CollisionShapeCapsule> m_collisionCapsules;
	std::vector<Model_CollisionShapeCylinder> m_collisionCylinders;
	std::vector<Model_CollisionShapeSphere> m_collisionSpheres;
};


} // namespace sge
