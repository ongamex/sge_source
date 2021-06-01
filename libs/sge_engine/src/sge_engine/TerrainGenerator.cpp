#include "TerrainGenerator.h"
#include "sge_core/model/CollisionMesh.h"
#include "sge_engine/typelibHelper.h"

namespace sge {
// clang-format off
DefineTypeId(StairsDesc, 20'03'02'0009);
ReflBlock() {
	ReflAddType(StairsDesc)
		ReflMember(StairsDesc, numStairs)
		ReflMember(StairsDesc, width)
		ReflMember(StairsDesc, height)
		ReflMember(StairsDesc, depth)
	;
}

DefineTypeId(SlopeDesc, 20'03'02'0010);
ReflBlock() {
	ReflAddType(SlopeDesc)
		ReflMember(SlopeDesc, width)
		ReflMember(SlopeDesc, height)
		ReflMember(SlopeDesc, depth)
	;
}

DefineTypeId(SlantedBlockDesc, 20'10'27'0002);
ReflBlock() {
	ReflAddType(SlantedBlockDesc)
		ReflMember(SlantedBlockDesc, width)
		ReflMember(SlantedBlockDesc, height)
		ReflMember(SlantedBlockDesc, depth)
		ReflMember(SlantedBlockDesc, thickness)
	;
}

DefineTypeId(SimpleObstacleType, 20'03'02'0011);
ReflBlock() {
	ReflAddType(SimpleObstacleType)
		ReflEnumVal((int)SimpleObstacleType::Stairs, "Stairs")
		ReflEnumVal((int)SimpleObstacleType::Slope, "Slope")
		ReflEnumVal((int)SimpleObstacleType::SlantedBlock, "SlantedBlock")
	;

}

DefineTypeId(SimpleObstacleDesc, 20'03'02'0012);
ReflBlock() {
	ReflAddType(SimpleObstacleDesc)
		ReflMember(SimpleObstacleDesc, type)
		ReflMember(SimpleObstacleDesc, stairs)
		ReflMember(SimpleObstacleDesc, slope)
		ReflMember(SimpleObstacleDesc, slantedBlock)
	;
}
// clang-format on

// Generates the vertex buffer and the bounding boxes needed to represent this geometry for both rendering and physics.
bool TerrainGenerator::generateStairs(std::vector<Vertex>& vertices,
                                      std::vector<int>& indices,
                                      std::vector<AABox3f>& bboxes,
                                      const StairsDesc& desc,
                                      int* pNumVertsAdded,
                                      int* pNumIndicesAdded,
                                      int* pNumBoxesAdded) {
	if (!desc.isValid()) {
		return false;
	}

	const int initalVertexCount = int(vertices.size());
	const int initalIndexCount = int(indices.size());
	const int initalBoxesCount = int(bboxes.size());

	float const singleStairWidth = desc.width / (float)desc.numStairs;
	float const singleStairHeight = desc.height / (float)desc.numStairs;

	// Generate the vertices for each box.
	for (int t = 0; t < desc.numStairs; ++t) {
		// the front control points for the stair (the look direction is assumed to be -z).

		float const stairStartX = (t + 0) * singleStairWidth;
		float const stairEndX = (t + 1) * singleStairWidth;
		float const stairHeight = (t + 1) * singleStairHeight;

		vec3f const v0 = vec3f(stairStartX, 0, 0);
		vec3f const v1 = vec3f(stairEndX, 0, 0);
		vec3f const v2 = vec3f(stairEndX, stairHeight, 0);
		vec3f const v3 = vec3f(stairStartX, stairHeight, 0);

		vec3f const toFront = vec3f(0.f, 0.f, desc.depth);

		// Create the front and back faces:
		{
			int const baseIdx = int(vertices.size());

			vertices.push_back(Vertex(v0, vec3f(0.f, 0.f, -1.f)));
			vertices.push_back(Vertex(v1, vec3f(0.f, 0.f, -1.f)));
			vertices.push_back(Vertex(v2, vec3f(0.f, 0.f, -1.f)));
			vertices.push_back(Vertex(v3, vec3f(0.f, 0.f, -1.f)));

			vertices.push_back(Vertex(v0 + toFront, vec3f(0.f, 0.f, +1.f)));
			vertices.push_back(Vertex(v1 + toFront, vec3f(0.f, 0.f, +1.f)));
			vertices.push_back(Vertex(v2 + toFront, vec3f(0.f, 0.f, +1.f)));
			vertices.push_back(Vertex(v3 + toFront, vec3f(0.f, 0.f, +1.f)));

			// Back
			indices.push_back(baseIdx + 0);
			indices.push_back(baseIdx + 2);
			indices.push_back(baseIdx + 1);

			indices.push_back(baseIdx + 0);
			indices.push_back(baseIdx + 3);
			indices.push_back(baseIdx + 2);

			// Front
			indices.push_back(baseIdx + 4);
			indices.push_back(baseIdx + 5);
			indices.push_back(baseIdx + 6);

			indices.push_back(baseIdx + 4);
			indices.push_back(baseIdx + 6);
			indices.push_back(baseIdx + 7);
		}

		// Create the top faces.
		{
			int const baseIdx = int(vertices.size());

			vertices.push_back(Vertex(v3, vec3f(0.f, 1.f, 0.f)));
			vertices.push_back(Vertex(v2, vec3f(0.f, 1.f, 0.f)));
			vertices.push_back(Vertex(v3 + toFront, vec3f(0.f, 1.f, 0.f)));
			vertices.push_back(Vertex(v2 + toFront, vec3f(0.f, 1.f, 0.f)));

			indices.push_back(baseIdx + 0);
			indices.push_back(baseIdx + 2);
			indices.push_back(baseIdx + 1);

			indices.push_back(baseIdx + 1);
			indices.push_back(baseIdx + 2);
			indices.push_back(baseIdx + 3);
		}

		// TODO: this could be a single face!
		// Create the bottom faces.
		{
			int const baseIdx = int(vertices.size());

			vertices.push_back(Vertex(v1, vec3f(0.f, -1.f, 0.f)));
			vertices.push_back(Vertex(v0, vec3f(0.f, -1.f, 0.f)));
			vertices.push_back(Vertex(v1 + toFront, vec3f(0.f, -1.f, 0.f)));
			vertices.push_back(Vertex(v0 + toFront, vec3f(0.f, -1.f, 0.f)));

			indices.push_back(baseIdx + 0);
			indices.push_back(baseIdx + 2);
			indices.push_back(baseIdx + 1);

			indices.push_back(baseIdx + 1);
			indices.push_back(baseIdx + 2);
			indices.push_back(baseIdx + 3);
		}

		// Create the left-side faces.
		// TODO: Create a smaller faces for these in order to reduce the fill rate!
		{
			int const baseIdx = int(vertices.size());

			vertices.push_back(Vertex(v0, vec3f(-1.f, 0.f, 0.f)));
			vertices.push_back(Vertex(v3, vec3f(-1.f, 0.f, 0.f)));
			vertices.push_back(Vertex(v0 + toFront, vec3f(-1.f, 0.f, 0.f)));
			vertices.push_back(Vertex(v3 + toFront, vec3f(-1.f, 0.f, 0.f)));

			indices.push_back(baseIdx + 0);
			indices.push_back(baseIdx + 2);
			indices.push_back(baseIdx + 1);

			indices.push_back(baseIdx + 1);
			indices.push_back(baseIdx + 2);
			indices.push_back(baseIdx + 3);
		}

		// Create the right-size faces. They are needed only for the last stair
		if (t == (desc.numStairs - 1)) {
			int const baseIdx = int(vertices.size());

			vertices.push_back(Vertex(v1, vec3f(+1.f, 0.f, 0.f)));
			vertices.push_back(Vertex(v2, vec3f(+1.f, 0.f, 0.f)));
			vertices.push_back(Vertex(v1 + toFront, vec3f(+1.f, 0.f, 0.f)));
			vertices.push_back(Vertex(v2 + toFront, vec3f(+1.f, 0.f, 0.f)));

			indices.push_back(baseIdx + 0);
			indices.push_back(baseIdx + 3);
			indices.push_back(baseIdx + 2);

			indices.push_back(baseIdx + 0);
			indices.push_back(baseIdx + 1);
			indices.push_back(baseIdx + 3);
		}
	}


	// Add the bounding boxes.
	for (int t = 0; t < desc.numStairs; ++t) {
		AABox3f box;

		box.min.x = (t + 0) * singleStairWidth;
		box.min.y = 0.f;
		box.min.z = 0.f;

		box.max.x = (t + 1) * singleStairWidth;
		box.max.y = (t + 1) * singleStairHeight;
		box.max.z = desc.depth;

		bboxes.push_back(box);
	}

	if (pNumVertsAdded)
		*pNumVertsAdded = int(vertices.size()) - initalVertexCount;
	if (pNumIndicesAdded)
		*pNumIndicesAdded = int(indices.size()) - initalIndexCount;
	if (pNumBoxesAdded)
		*pNumBoxesAdded = int(bboxes.size()) - initalBoxesCount;

	return true;
}

bool TerrainGenerator::generateSlope(std::vector<Vertex>& vertices,
                                     std::vector<int>& indices,
                                     ModelCollisionMesh& outCollisionMesh,
                                     const SlopeDesc& desc,
                                     int* pNumVertsAdded,
                                     int* pNumIndicesAdded) {
	outCollisionMesh.freeMemory();

	if (desc.isValid() == false) {
		return false;
	}

	const int initalVertexCount = int(vertices.size());
	const int initalIndexCount = int(indices.size());

	// The front face control points.
	vec3f const v0 = vec3f(0.f, 0.f, 0.f);
	vec3f const v1 = vec3f(desc.width, 0.f, 0.f);
	vec3f const v2 = vec3f(desc.width, desc.height, 0.f);

	vec3f const toFront = vec3f(0.f, 0.f, desc.depth);

	// Create the back face
	{
		int const baseIdx = int(vertices.size());

		vertices.push_back(Vertex(v0, vec3f(0.f, 0.f, -1.f)));
		vertices.push_back(Vertex(v1, vec3f(0.f, 0.f, -1.f)));
		vertices.push_back(Vertex(v2, vec3f(0.f, 0.f, -1.f)));

		indices.push_back(baseIdx + 0);
		indices.push_back(baseIdx + 2);
		indices.push_back(baseIdx + 1);
	}

	// Create the front face.
	{
		int const baseIdx = int(vertices.size());

		vertices.push_back(Vertex(v0 + toFront, vec3f(0.f, 0.f, +1.f)));
		vertices.push_back(Vertex(v1 + toFront, vec3f(0.f, 0.f, +1.f)));
		vertices.push_back(Vertex(v2 + toFront, vec3f(0.f, 0.f, +1.f)));

		indices.push_back(baseIdx + 0);
		indices.push_back(baseIdx + 1);
		indices.push_back(baseIdx + 2);
	}

	// Create the top faces(the slope itself).
	{
		int const baseIdx = int(vertices.size());
		vec3f const slopeNormal = normalized(vec3f(-desc.height, desc.width, 0.f));

		vertices.push_back(Vertex(v0, slopeNormal));
		vertices.push_back(Vertex(v2, slopeNormal));
		vertices.push_back(Vertex(v0 + toFront, slopeNormal));
		vertices.push_back(Vertex(v2 + toFront, slopeNormal));

		indices.push_back(baseIdx + 0);
		indices.push_back(baseIdx + 3);
		indices.push_back(baseIdx + 1);

		indices.push_back(baseIdx + 0);
		indices.push_back(baseIdx + 2);
		indices.push_back(baseIdx + 3);
	}

	// Create the bottom faces(the slope itself).
	{
		int const baseIdx = int(vertices.size());

		vec3f const slopeNormal = normalized(vec3f(-desc.height, desc.width, 0.f));

		vertices.push_back(Vertex(v1, vec3f(0.f, 0.f, -1.f)));
		vertices.push_back(Vertex(v0, vec3f(0.f, 0.f, -1.f)));
		vertices.push_back(Vertex(v1 + toFront, vec3f(0.f, 0.f, -1.f)));
		vertices.push_back(Vertex(v0 + toFront, vec3f(0.f, 0.f, -1.f)));

		indices.push_back(baseIdx + 0);
		indices.push_back(baseIdx + 3);
		indices.push_back(baseIdx + 1);

		indices.push_back(baseIdx + 0);
		indices.push_back(baseIdx + 2);
		indices.push_back(baseIdx + 3);
	}

	// Create the faces on the right.
	{
		int const baseIdx = int(vertices.size());

		vertices.push_back(Vertex(v1, vec3f(1.f, 0.f, 0.f)));
		vertices.push_back(Vertex(v2, vec3f(1.f, 0.f, 0.f)));
		vertices.push_back(Vertex(v1 + toFront, vec3f(1.f, 0.f, 0.f)));
		vertices.push_back(Vertex(v2 + toFront, vec3f(1.f, 0.f, 0.f)));

		indices.push_back(baseIdx + 0);
		indices.push_back(baseIdx + 1);
		indices.push_back(baseIdx + 2);

		indices.push_back(baseIdx + 3);
		indices.push_back(baseIdx + 2);
		indices.push_back(baseIdx + 1);
	}

	// Finally add the triangles for the collision mesh. Just use the onces we've created for the rendering.
	outCollisionMesh.vertices.resize(vertices.size());
	outCollisionMesh.indices.resize(indices.size());

	for (int iVert = 0; iVert < outCollisionMesh.vertices.size(); ++iVert) {
		outCollisionMesh.vertices[iVert] = vertices[iVert].p;
	}

	for (int iIdx = 0; iIdx < outCollisionMesh.indices.size(); ++iIdx) {
		outCollisionMesh.indices[iIdx] = indices[iIdx];
	}

	sgeAssert(outCollisionMesh.indices.size() % 3 == 0);

	if (pNumVertsAdded)
		*pNumVertsAdded = int(vertices.size()) - initalVertexCount;
	if (pNumIndicesAdded)
		*pNumIndicesAdded = int(indices.size()) - initalIndexCount;

	return true;
}

bool TerrainGenerator::generateSlantedBlock(std::vector<Vertex>& vertices,
                                            std::vector<int>& indices,
                                            ModelCollisionMesh& outCollisionMesh,
                                            const SlantedBlockDesc& desc,
                                            int* pNumVertsAdded,
                                            int* pNumIndicesAdded) {
	if (!desc.isValid()) {
		return false;
	}

	const int initalVertexCount = int(vertices.size());
	const int initalIndexCount = int(indices.size());

	// Generate the vertices for each box.
	{
		// the front control points for the stair (the look direction is assumed to be -z).

		float const stairStartX = 0;
		float const stairEndX = desc.width;
		float const stairHeight = desc.height;

		vec3f const v0 = vec3f(stairStartX, -desc.thickness, 0);
		vec3f const v1 = vec3f(stairEndX, stairHeight - desc.thickness, 0);
		vec3f const v2 = vec3f(stairEndX, stairHeight, 0);
		vec3f const v3 = vec3f(stairStartX, 0, 0);

		vec3f const toFront = vec3f(0.f, 0.f, desc.depth);

		// Create the front and back faces:
		{
			int const baseIdx = int(vertices.size());

			vertices.push_back(Vertex(v0, vec3f(0.f, 0.f, -1.f)));
			vertices.push_back(Vertex(v1, vec3f(0.f, 0.f, -1.f)));
			vertices.push_back(Vertex(v2, vec3f(0.f, 0.f, -1.f)));
			vertices.push_back(Vertex(v3, vec3f(0.f, 0.f, -1.f)));

			vertices.push_back(Vertex(v0 + toFront, vec3f(0.f, 0.f, +1.f)));
			vertices.push_back(Vertex(v1 + toFront, vec3f(0.f, 0.f, +1.f)));
			vertices.push_back(Vertex(v2 + toFront, vec3f(0.f, 0.f, +1.f)));
			vertices.push_back(Vertex(v3 + toFront, vec3f(0.f, 0.f, +1.f)));

			// Back
			indices.push_back(baseIdx + 0);
			indices.push_back(baseIdx + 2);
			indices.push_back(baseIdx + 1);

			indices.push_back(baseIdx + 0);
			indices.push_back(baseIdx + 3);
			indices.push_back(baseIdx + 2);

			// Front
			indices.push_back(baseIdx + 4);
			indices.push_back(baseIdx + 5);
			indices.push_back(baseIdx + 6);

			indices.push_back(baseIdx + 4);
			indices.push_back(baseIdx + 6);
			indices.push_back(baseIdx + 7);
		}

		// Create the top faces.
		{
			int const baseIdx = int(vertices.size());

			vertices.push_back(Vertex(v3, vec3f(0.f, 1.f, 0.f)));
			vertices.push_back(Vertex(v2, vec3f(0.f, 1.f, 0.f)));
			vertices.push_back(Vertex(v3 + toFront, vec3f(0.f, 1.f, 0.f)));
			vertices.push_back(Vertex(v2 + toFront, vec3f(0.f, 1.f, 0.f)));

			indices.push_back(baseIdx + 0);
			indices.push_back(baseIdx + 2);
			indices.push_back(baseIdx + 1);

			indices.push_back(baseIdx + 1);
			indices.push_back(baseIdx + 2);
			indices.push_back(baseIdx + 3);
		}

		// TODO: this could be a single face!
		// Create the bottom faces.
		{
			int const baseIdx = int(vertices.size());

			vertices.push_back(Vertex(v1, vec3f(0.f, -1.f, 0.f)));
			vertices.push_back(Vertex(v0, vec3f(0.f, -1.f, 0.f)));
			vertices.push_back(Vertex(v1 + toFront, vec3f(0.f, -1.f, 0.f)));
			vertices.push_back(Vertex(v0 + toFront, vec3f(0.f, -1.f, 0.f)));

			indices.push_back(baseIdx + 0);
			indices.push_back(baseIdx + 2);
			indices.push_back(baseIdx + 1);

			indices.push_back(baseIdx + 1);
			indices.push_back(baseIdx + 2);
			indices.push_back(baseIdx + 3);
		}

		// Create the left-side faces.
		// TODO: Create a smaller faces for these in order to reduce the fill rate!
		{
			int const baseIdx = int(vertices.size());

			vertices.push_back(Vertex(v0, vec3f(-1.f, 0.f, 0.f)));
			vertices.push_back(Vertex(v3, vec3f(-1.f, 0.f, 0.f)));
			vertices.push_back(Vertex(v0 + toFront, vec3f(-1.f, 0.f, 0.f)));
			vertices.push_back(Vertex(v3 + toFront, vec3f(-1.f, 0.f, 0.f)));

			indices.push_back(baseIdx + 0);
			indices.push_back(baseIdx + 2);
			indices.push_back(baseIdx + 1);

			indices.push_back(baseIdx + 1);
			indices.push_back(baseIdx + 2);
			indices.push_back(baseIdx + 3);
		}

		// Create the right-size faces. They are needed only for the last stair
		{
			int const baseIdx = int(vertices.size());

			vertices.push_back(Vertex(v1, vec3f(+1.f, 0.f, 0.f)));
			vertices.push_back(Vertex(v2, vec3f(+1.f, 0.f, 0.f)));
			vertices.push_back(Vertex(v1 + toFront, vec3f(+1.f, 0.f, 0.f)));
			vertices.push_back(Vertex(v2 + toFront, vec3f(+1.f, 0.f, 0.f)));

			indices.push_back(baseIdx + 0);
			indices.push_back(baseIdx + 3);
			indices.push_back(baseIdx + 2);

			indices.push_back(baseIdx + 0);
			indices.push_back(baseIdx + 1);
			indices.push_back(baseIdx + 3);
		}
	}

	outCollisionMesh.vertices.resize(vertices.size());
	outCollisionMesh.indices.resize(indices.size());

	for (int iVert = 0; iVert < outCollisionMesh.vertices.size(); ++iVert) {
		outCollisionMesh.vertices[iVert] = vertices[iVert].p;
	}

	for (int iIdx = 0; iIdx < outCollisionMesh.indices.size(); ++iIdx) {
		outCollisionMesh.indices[iIdx] = indices[iIdx];
	}

	sgeAssert(outCollisionMesh.indices.size() % 3 == 0);

	if (pNumVertsAdded)
		*pNumVertsAdded = int(vertices.size()) - initalVertexCount;
	if (pNumIndicesAdded)
		*pNumIndicesAdded = int(indices.size()) - initalIndexCount;

	return true;
}

} // namespace sge
