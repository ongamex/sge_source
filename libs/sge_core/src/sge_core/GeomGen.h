#pragma once

#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/math/Box.h"
#include "sge_utils/math/vec3.h"
#include "sgecore_api.h"

namespace sge {

struct SGE_CORE_API GeomGen {
  public:
	enum Origin {
		center,
		Bottom,
	};

	/// Generates a full screen quad in NDC with UV.
	/// Vertices: Triangle List of [ float3 Position, float2 UV ], total 6 vertices.
	static int ndcQuad3DUV(Buffer* const resultVertBuffer);

	// Generates a sphere in RH Y-up.
	// Returns the number of vertices
	// Vertices: Triangle List of [ float3 Position ]
	static int sphere(Buffer* const resultVertBuffer, int numSlices, int numSectors);

	// Generates a plane using up and right vector.
	// Returns the number of vertices
	// Vertices: Triangle List of [ float3 Position, float3 Normal ]
	static int plane(Buffer* resultVertBuffer, const vec3f& up, const vec3f& right, const bool addNormals = false);

	// Generates a cylinder
	// Returns the number of vertices
	// Vertices: Triangle Strip of [ float3 Position ]
	// [NOTE] Some additional triangles will be generated inside of the cylinder, becase of the
	// Triangle Strip primitive topology.
	static int cylinder(Buffer* resultVertBuffer, const vec3f position, float height, float radius, int numSlices);

	struct PosColorVert {
		PosColorVert() = default;

		PosColorVert(vec3f pt, uint32 rgba)
		    : pt(pt)
		    , rgba(rgba) {}

		vec3f pt;
		uint32 rgba;
	};

	static void wiredLine(std::vector<PosColorVert>& verts, const vec3f& a, const vec3f b, const uint32 rgba);

	// Generates a 3D {2,2,2}sized writed(edges only) box in RH Y-up. Origin is the center of the box.
	// Adds the vertices to the global array for wire geometry - verts.
	// Vertices: Line List of [ float3 Position ]
	static void wiredBox(std::vector<PosColorVert>& verts, const mat4f& transform, const uint32 rgba);
	static void wiredBox(std::vector<PosColorVert>& verts, const AABox3f& aabb, const uint32 rgba);

	// Generates a 3D wired capsule in RH Y-up.
	// Adds the vertices to the global array for wire geometry - verts.
	// Vertices: Line List of [ PosColorVert ]
	static void wiredCapsule(std::vector<PosColorVert>& verts,
	                         const mat4f& transform,
	                         const uint32 rgba,
	                         float height,
	                         float radius,
	                         int numSideDivs,
	                         const Origin origin);

	// Generates a 3D wired sphere in RH Y-up.
	// Adds the vertices to the global array for wire geometry - verts.
	// Vertices: Line List of [ PosColorVert ]
	static void wiredSphere(std::vector<PosColorVert>& verts, const mat4f& transform, const uint32 rgba, float radius, int numSides);

	// Generates a 3D wired cylinder in RH Y-up.
	// Adds the vertices to the global array for wire geometry - verts.
	// Vertices: Line List of [ PosColorVert ]
	static void wiredCylinder(std::vector<PosColorVert>& verts,
	                          const mat4f& transform,
	                          const uint32 rgba,
	                          float height,
	                          float radius,
	                          int numSides,
	                          const Origin origin);

	// Generates a 3D wired cone in RH Y-up.
	// Adds the vertices to the global array for wire geometry - verts.
	// Vertices: Line List of [ PosColorVert ]
	static void wiredCone(std::vector<PosColorVert>& verts,
	                      const mat4f& transform,
	                      const uint32 rgba,
	                      float height,
	                      float radius,
	                      int numSides,
	                      const Origin origin);

	// Generates a 3D wired(edges only) basis.
	// Vertices Line List of [ PosColorVert ]
	static int wiredBasis(std::vector<PosColorVert>& verts, const mat4f& transform);

	// Generates
	//
	static void wiredGrid(std::vector<PosColorVert>& verts,
	                      const vec3f& origin,
	                      const vec3f& xAxis,
	                      const vec3f& zAxis,
	                      const int xLinesCnt,
	                      const int zLinesCnt,
	                      int color = 0x000000);

	// Generates an Immutable R8G8B8A8_UNORM checker texture texture, starting with black.
	static void createChecker(Texture* texture, const int size, const int checkSize, const SamplerDesc& prefferedSampler);
};

} // namespace sge
