#include <vector>

#include "sge_core/model/CollisionMesh.h"
#include "sge_engine/sge_engine_api.h"
#include "sge_utils/math/Box.h"
#include "sge_utils/math/vec3.h"

namespace sge {

// TODO: This is really similar to the GeomGen class. But i suppect I will need a bit more specifc thing
// so I'll keep it spearate.

enum class SimpleObstacleType : int {
	Stairs,
	Slope,
	SlantedBlock,
};

struct StairsDesc {
	int numStairs = 1; // if the number of stairs is 1 this is just a block.

	// the sizes of the whole stairs block.
	float width = 1;
	float height = 1;
	float depth = 1;

	StairsDesc() = default;

	bool isValid() const { return numStairs > 0 && width > 1e-6f && height > 1e-6f && depth > 1e-6f; }
};

struct SlopeDesc {
	float width = 1;
	float height = 1;
	float depth = 1;

	SlopeDesc() = default;

	bool isValid() const { return width > 1e-6f && height > 1e-6f && depth > 1e-6f; }
};

struct SlantedBlockDesc {
	float width = 1;
	float height = 1;
	float depth = 1;
	float thickness = 0.5f;

	SlantedBlockDesc() = default;

	bool isValid() const { return width > 1e-6f && height > 1e-6f && depth > 1e-6f && thickness > 1e-6f; }
};

struct SimpleObstacleDesc {
	SimpleObstacleType type = SimpleObstacleType::Stairs;
	StairsDesc stairs;
	SlopeDesc slope;
	SlantedBlockDesc slantedBlock;
};

struct SGE_ENGINE_API TerrainGenerator {
	struct Vertex {
		Vertex() = default;
		Vertex(const vec3f& p, const vec3f& n)
		    : p(p)
		    , n(n) {}

		vec3f p = vec3f(0.f);
		vec3f n = vec3f(0.f, 1.f, 0.f);
	};

	// Generates the vertex buffer and the bounding boxes needed to represent this geometry for both rendering and physics.
	static bool generateStairs(std::vector<Vertex>& vertices,
	                           std::vector<int>& indices,
	                           std::vector<AABox3f>& bboxes,
	                           const StairsDesc& desc,
	                           int* pNumVertsAdded = nullptr,
	                           int* pNumIndicesAdded = nullptr,
	                           int* pNumBoxesAdded = nullptr);


	static bool generateSlope(std::vector<Vertex>& vertices,
	                          std::vector<int>& indices,
	                          ModelCollisionMesh& outCollisionMesh,
	                          const SlopeDesc& desc,
	                          int* pNumVertsAdded = nullptr,
	                          int* pNumIndicesAdded = nullptr);

	static bool generateSlantedBlock(std::vector<Vertex>& vertices,
	                                 std::vector<int>& indices,
	                                 ModelCollisionMesh& outCollisionMesh,
	                                 const SlantedBlockDesc& desc,
	                                 int* pNumVertsAdded = nullptr,
	                                 int* pNumIndicesAdded = nullptr);
};


} // namespace sge
