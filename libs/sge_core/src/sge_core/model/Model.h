#pragma once

#include "sge_core/model/Parameter.h"
#include "sge_core/sgecore_api.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/math/Box.h"
#include "sge_utils/math/mat4.h"
#include "sge_utils/math/primitives.h"
#include "sge_utils/math/transform.h"
#include "sge_utils/utils/ChunkContainer.h"
#include "sge_utils/utils/IStream.h"
#include <string>

#include "CollisionMesh.h"

namespace sge {

namespace Model {

	struct CollisionShapeBox {
		CollisionShapeBox() = default;
		CollisionShapeBox(std::string name, transf3d transform, vec3f halfDiagonal)
		    : name(std::move(name))
		    , transform(transform)
		    , halfDiagonal(halfDiagonal) {}

		std::string name;
		transf3d transform = transf3d::getIdentity();
		vec3f halfDiagonal = vec3f(0.f);
	};

	// Represents a capsule. The capsule is defined by all points that are at distace "radius" from
	// the line defined by ({0.f, -halfHeight, 0.f}, {0.f, halfHeight, 0.f}}
	struct CollisionShapeCapsule {
		CollisionShapeCapsule() = default;
		CollisionShapeCapsule(std::string name, transf3d transform, float halfHeight, float radius)
		    : name(std::move(name))
		    , transform(transform)
		    , halfHeight(halfHeight)
		    , radius(radius) {}

		std::string name;
		transf3d transform = transf3d::getIdentity();
		float halfHeight = 0.f;
		float radius = 0.f;
	};

	struct CollisionShapeCylinder {
		CollisionShapeCylinder() = default;
		CollisionShapeCylinder(std::string name, transf3d transform, vec3f halfDiagonal)
		    : name(std::move(name))
		    , transform(transform)
		    , halfDiagonal(halfDiagonal) {}

		std::string name;
		transf3d transform = transf3d::getIdentity();
		vec3f halfDiagonal = vec3f(0.f);
	};

	struct CollisionShapeSphere {
		CollisionShapeSphere() = default;
		CollisionShapeSphere(std::string name, transf3d transform, float radius)
		    : name(std::move(name))
		    , transform(transform)
		    , radius(radius) {}

		std::string name;
		transf3d transform = transf3d::getIdentity();
		float radius = 0.f;
	};

	// Loading settings describing how the model should be loaded.
	struct LoadSettings {
		enum KeepMeshData {
			KeepMeshData_None, // Do not keep any vertex/index buffer data for the CPU.
			KeepMeshData_Skin, // Keep only vetex/index buffer data for meshes with skinning for the CPU.
			KeepMeshData_All,  // Keep all vertex/index buffer data for the CPU.
		};

		KeepMeshData cpuMeshData = KeepMeshData_All;

		// The directory of the file, used for loading asset.
		std::string assetDir;
	};

	// Mesh material.
	struct Material {
		int id = -1; // The "id" used to identify this piece of data in the file.
		std::string name;
		ParameterBlock paramBlock;
	};

	// Skinning bone.
	struct Bone {
		mat4f offsetMatrix = mat4f::getIdentity();
		struct Node* node = nullptr;
	};

	struct SGE_CORE_API Mesh {
		int id = -1; // The "id" used to identify this piece of data in the file.
		std::string name;

		PrimitiveTopology::Enum primTopo = PrimitiveTopology::Unknown; // TODO: Rename that variable.
		int vbByteOffset = 0;                                          // 1st vertex byte offset into the vertex buffer
		int ibByteOffset = 0;                                          // 1st index byte offse int the index buffer
		UniformType::Enum ibFmt = UniformType::Unknown; // The format the index buffer, if unknown this mesh doesn't use index buffers.
		int numElements = 0;                            // The number of vertices/indices used by this mesh.
		int numVertices = 0;                            // The number of vertices in the mesh.
		std::vector<VertexDecl> vertexDecl;             // The vertex declaration

		struct MeshData* pMeshData = nullptr; // Vertex/index buffer ect. are stored here.
		Material* pMaterial = nullptr;        // A pointer to the material.

		std::vector<Bone> bones;

		AABox3f aabox;

		// Few precached values for every mesh.
		int stride = 0;
		int vbPositionOffsetBytes = -1;
		int vbNormalOffsetBytes = -1;
		int vbUVOffsetBytes = -1;
		float Raycast(const Ray& ray, const char* positionSemantic = "a_position") const;

	  private:
		// The function expects validated data.
		// T  could be ushort or uint32
		// pAllIntersections - an optional array of interpolation coefficients where an intersection had happened (not sorted).
		// Returns the interpolation coeffieceint FLT_MAX on no intersection.
		template <typename T>
		float RaycastIndexBuffer(const Ray& ray, const int posByteOffset, std::vector<float>* pAllIntersections = NULL) const;
		float RaycastVertexBufferOnly(const Ray& ray, const int posByteOffset, std::vector<float>* pAllIntersections = NULL) const;
	};

	struct MeshData {
		GpuHandle<Buffer> vertexBuffer;
		GpuHandle<Buffer> indexBuffer;

		std::vector<char> vertexBufferRaw; // CPU cached vertex buffer data. May be empty depending on import settings
		std::vector<char> indexBufferRaw;  // CPU cached index buffer data. May be empty depending on import settings

		std::vector<Mesh*> meshes;
	};

	struct AnimationInfo {
		AnimationInfo() = default;
		AnimationInfo(const char* curveName, float startTime, float duration)
		    : curveName(curveName)
		    , startTime(startTime)
		    , duration(duration) {}

		std::string curveName;
		float startTime = 0;
		float duration = 0;
	};

	struct MeshAttachment {
		Mesh* mesh = nullptr;
		Material* material = nullptr;
	};

	struct Node {
		ParameterBlock paramBlock;

		std::vector<MeshAttachment> meshAttachments;
		std::vector<Node*> childNodes; // A pointers to the child nodes.

		int id = -1; // The "id" used to identify this piece of data in the file.
		std::string name;
	};

	//--------------------------------------------------------------
	// Model
	//
	// The member convension here:
	// I - means this member is used when importing models (aka. in game).
	// O - means this member MUST be VALID when exporting the model(in the ModelWriter).
	// IO - means that the member is used by both.
	//--------------------------------------------------------------
	struct SGE_CORE_API Model {
		// Searches for an object. If the object is missing these functions will return nullptr.
		Material* FindMaterial(const int id);
		Node* FindNode(const int id);
		const Node* FindNode(const int id) const;
		const Node* FindFirstNodeByName(const std::string& name) const;
		Mesh* FindMesh(const int id);
		const AnimationInfo* findAnimation(const std::string& name) const;

	  public:
		// The actual storage for most of the models data.
		ChunkContainer<Mesh> m_containerMesh;
		ChunkContainer<MeshData> m_containerMeshData;
		ChunkContainer<Material> m_containerMaterial;
		ChunkContainer<Node> m_containerNode;

		// The model working data. Those pointer must allways be valid.
		Node* m_rootNode = nullptr;              // IO
		std::vector<AnimationInfo> m_animations; // IO
		std::vector<Node*> m_nodes;              // IO
		std::vector<MeshData*> m_meshesData;     // IO
		std::vector<Material*> m_materials;      // IO

		// A set of convex hulls (if any) evaluated in the static moment using the meshes.
		std::vector<CollisionMesh> m_convexHulls;                 // IO
		std::vector<CollisionMesh> m_concaveHulls;                // IO
		std::vector<CollisionShapeBox> m_collisionBoxes;          // IO
		std::vector<CollisionShapeCapsule> m_collisionCapsules;   // IO
		std::vector<CollisionShapeCylinder> m_collisionCylinders; // IO
		std::vector<CollisionShapeSphere> m_collisionSpheres;     // IO

		// Cached loading settings.
		LoadSettings m_loadSets; // I
	};
} // namespace Model

} // namespace sge
