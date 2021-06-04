#include "GeomGen.h"
#include <vector>

namespace sge {

int GeomGen::ndcQuad3DUV(Buffer* resultVertBuffer) {
	if (resultVertBuffer == nullptr) {
		sgeAssert(false);
		return 0;
	}

	struct Vertex {
		vec3f p;
		vec2f uv;
	};

	Vertex verts[6] = {
	    {vec3f(-1.f, -1.f, 0.f), vec2f(0.f, 0.f)}, 
		{vec3f(1.f, -1.f, 0.f), vec2f(1.f, 0.f)}, 
		{vec3f(1.f, 1.f, 0.f), vec2f(1.f, 1.f)},

	    {vec3f(1.f, 1.f, 0.f), vec2f(1.f, 1.f)},
		{vec3f(-1.f, 1.f, 0.f), vec2f(0.f, 1.f)},
		{vec3f(-1.f, -1.f, 0.f), vec2f(0.f, 0.f)},
	};

	BufferDesc bd = BufferDesc::GetDefaultVertexBuffer(sizeof(verts));
	resultVertBuffer->create(bd, verts);

	return SGE_ARRSZ(verts);
}

int GeomGen::sphere(Buffer* resultVertBuffer, int numRings, int numSectors) {
	if (resultVertBuffer == nullptr) {
		sgeAssert(false);
		return 0;
	}

	if (numRings < 3) {
		sgeAssert(false);
		numRings = 3;
	}
	if (numSectors < 3) {
		sgeAssert(false);
		numSectors = 3;
	}

	const int numTriangles = (2 * (numRings - 2) * numSectors) + (numSectors * 2);

	std::vector<vec3f> verts;
	verts.reserve(numTriangles);

	const float fRingStep = sgePi / (float)numRings;
	const float fSectorStep = sge2Pi / (float)numSectors;

	for (int iRing = 1; iRing < numRings + 1; ++iRing)
		for (int iSector = 0; iSector < numSectors; ++iSector) {
			//[TODO] change this with matrix multiplication, try creation only a hemisphere
			const float sr0 = sinf(fRingStep * (float)(iRing - 1));
			const float cr0 = cosf(fRingStep * (float)(iRing - 1));

			const float ss0 = -sinf(fSectorStep * (float)(iSector - 1));
			const float cs0 = cosf(fSectorStep * (float)(iSector - 1));

			const float sr1 = sinf(fRingStep * (float)iRing);
			const float cr1 = cosf(fRingStep * (float)iRing);

			const float ss1 = -sinf(fSectorStep * (float)iSector);
			const float cs1 = cosf(fSectorStep * (float)iSector);

			if (iRing == numRings) {
				verts.push_back(vec3f(cs0 * sr0, cr0, ss0 * sr0));
				verts.push_back(vec3f(0, -1, 0));
				verts.push_back(vec3f(cs1 * sr0, cr0, ss1 * sr0));
				continue;
			}

			if (iRing != 1) {
				verts.push_back(vec3f(cs0 * sr0, cr0, ss0 * sr0));
				verts.push_back(vec3f(cs1 * sr1, cr1, ss1 * sr1));
				verts.push_back(vec3f(cs1 * sr0, cr0, ss1 * sr0));
			}

			verts.push_back(vec3f(cs0 * sr0, cr0, ss0 * sr0));
			verts.push_back(vec3f(cs0 * sr1, cr1, ss0 * sr1));
			verts.push_back(vec3f(cs1 * sr1, cr1, ss1 * sr1));
		}

	const BufferDesc bd = BufferDesc::GetDefaultVertexBuffer(verts.size() * sizeof(vec3f));
	resultVertBuffer->create(bd, verts.data());

	return (int)verts.size();
}

int GeomGen::plane(Buffer* resultVertBuffer, const vec3f& up, const vec3f& right, const bool addNormals) {
	const vec3f norm = cross(right, up);

	std::vector<vec3f> verts;
	verts.reserve(6);

	verts.push_back(-up - right);
	if (addNormals)
		verts.push_back(norm);
	verts.push_back(-up + right);
	if (addNormals)
		verts.push_back(norm);
	verts.push_back(+up + right);
	if (addNormals)
		verts.push_back(norm);

	verts.push_back(+up + right);
	if (addNormals)
		verts.push_back(norm);
	verts.push_back(+up - right);
	if (addNormals)
		verts.push_back(norm);
	verts.push_back(-up - right);
	if (addNormals)
		verts.push_back(norm);

	BufferDesc bd = BufferDesc::GetDefaultVertexBuffer(verts.size() * sizeof(vec3f));
	resultVertBuffer->create(bd, verts.data());

	return 6;
}

int GeomGen::cylinder(Buffer* resultVertBuffer, const vec3f position, float height, float radius, int numSlices) {
	if (resultVertBuffer == nullptr) {
		sgeAssert(false);
		return 0;
	}

	// Cylinder with height along Y axis at position.
	const float theta = 2.f * sgePi / numSlices;
	const float c = cosf(theta);
	const float s = sinf(theta);

	// Coordinates on top of the circle, on xz plane.
	float x2 = radius;
	float z2 = 0;

	std::vector<vec3f> verts;
	verts.reserve(numSlices * 6);

	// verts for the wall of the cylinder
	for (int i = 0; i <= numSlices; ++i) {
		verts.push_back(vec3f(position.x + x2, position.y, position.z + z2));
		verts.push_back(vec3f(position.x + x2, position.y + height, position.z + z2));

		// Next position... Think of this as a expanded matrix2x2 multiplication.
		const float x3 = x2;
		x2 = c * x2 - s * z2;
		z2 = s * x3 + c * z2;
	}

	// [NOTE] Some additional triangles will be generated inside of the cylinder, becase of the
	// Triangle Strip primitive topology.

	// Verts for the bottom base of the cylinder.
	for (int i = 0; i <= numSlices; ++i) {
		verts.push_back(vec3f(position.x, position.y, position.z));
		verts.push_back(
		    vec3f(position.x + (radius * cos(i * sge2Pi / numSlices)), position.y, position.z + (radius * sinf(i * sge2Pi / numSlices))));
	}

	// Verts for the top base of the cylinder.
	for (int i = 0; i <= numSlices; ++i) {
		verts.push_back(vec3f(position.x, position.y, position.z));
		verts.push_back(vec3f(position.x - (radius * cos(i * sge2Pi / numSlices)), position.y + height,
		                      position.z + (radius * sinf(i * sge2Pi / numSlices))));
	}

	const BufferDesc bd = BufferDesc::GetDefaultVertexBuffer(verts.size() * sizeof(vec3f));
	resultVertBuffer->create(bd, verts.data());

	return int(verts.size());
}

void GeomGen::wiredLine(std::vector<PosColorVert>& verts, const vec3f& a, const vec3f b, const uint32 rgba) {
	verts.push_back(PosColorVert(a, rgba));
	verts.push_back(PosColorVert(b, rgba));
}

void GeomGen::wiredBox(std::vector<PosColorVert>& verts, const mat4f& transform, const uint32 rgba) {
	verts.reserve(verts.size() + 12 * 2);

	auto addVert = [&](const vec3f& pos) { verts.push_back(PosColorVert(pos, rgba)); };

	const vec3f xAxis = transform.data[0].xyz();
	const vec3f yAxis = transform.data[1].xyz();
	const vec3f zAxis = transform.data[2].xyz();
	const vec3f base = transform.data[3].xyz();

	// top front
	addVert(base + (-xAxis + yAxis - zAxis));
	addVert(base + (xAxis + yAxis - zAxis));

	// bottom front
	addVert(base + (-xAxis - yAxis - zAxis));
	addVert(base + (+xAxis - yAxis - zAxis));

	// top back
	addVert(base + (-xAxis + yAxis + zAxis));
	addVert(base + (+xAxis + yAxis + zAxis));

	// bottom back
	addVert(base + (-xAxis - yAxis + zAxis));
	addVert(base + (+xAxis - yAxis + zAxis));

	// left top
	addVert(base + (+xAxis + yAxis - zAxis));
	addVert(base + (+xAxis + yAxis + zAxis));

	// left bottom
	addVert(base + (+xAxis - yAxis - zAxis));
	addVert(base + (+xAxis - yAxis + zAxis));

	// right top
	addVert(base + (-xAxis + yAxis - zAxis));
	addVert(base + (-xAxis + yAxis + zAxis));

	// right bottom
	addVert(base + (-xAxis - yAxis - zAxis));
	addVert(base + (-xAxis - yAxis + zAxis));

	// front left edge
	addVert(base + (-xAxis + yAxis - zAxis));
	addVert(base + (-xAxis - yAxis - zAxis));

	// front right edge
	addVert(base + (+xAxis + yAxis - zAxis));
	addVert(base + (+xAxis - yAxis - zAxis));

	// back left edge
	addVert(base + (-xAxis + yAxis + zAxis));
	addVert(base + (-xAxis - yAxis + zAxis));

	// back right edge
	addVert(base + (+xAxis + yAxis + zAxis));
	addVert(base + (+xAxis - yAxis + zAxis));
}

void GeomGen::wiredBox(std::vector<PosColorVert>& verts, const AABox3f& aabb, const uint32 rgba) {
	const vec3f halfDiag = aabb.halfDiagonal();
	const vec3f center = aabb.center();

	mat4f transf = mat4f::getZero();

	transf.data[3][0] = center.x;
	transf.data[3][1] = center.y;
	transf.data[3][2] = center.z;

	transf.data[0][0] = halfDiag.x;
	transf.data[1][1] = halfDiag.y;
	transf.data[2][2] = halfDiag.z;
	transf.data[3][3] = 1.f;

	wiredBox(verts, transf, rgba);
}

void GeomGen::wiredCapsule(std::vector<PosColorVert>& verts,
                           const mat4f& transform,
                           const uint32 rgba,
                           float height,
                           float radius,
                           int numSideDivs,
                           const Origin origin) {
	sgeAssert(numSideDivs > 0);

	auto addVert = [&](const vec3f& pos) { verts.push_back(PosColorVert(pos, rgba)); };

	const vec3f xAxisRadius = transform.data[0].xyz() * radius;
	const vec3f yAxis = transform.data[1].xyz();
	const vec3f zAxisRadius = transform.data[2].xyz() * radius;
	const vec3f base = [&]() -> vec3f {
		// The algorithm below generatesa capsule that uses "bases" as the most bottom point of the capsule.
		if (origin == Bottom)
			return transform.data[3].xyz();
		if (origin == center)
			return transform.data[3].xyz() - (height * 0.5f + radius) * yAxis;

		sgeAssert(false && "Unimplemented center type.");
		return transform.data[3].xyz(); // Just return something default.
	}();


	const vec3f yAxisTopCircleLevel = yAxis * (height + radius);
	const vec3f yAxisBottomCircleLevel = yAxis * radius;

	const int numCircleSegments = numSideDivs * 4;
	const int numArcSegments = numCircleSegments / 2;
	const float sideStepRadiands = sge2Pi / (float)numCircleSegments;

	// Top circle.
	for (int t = 0; t < numCircleSegments + 1; ++t) {
		float s, c;
		SinCos(sideStepRadiands * (float)t, s, c);
		addVert(base + xAxisRadius * c + zAxisRadius * s + yAxisTopCircleLevel);
		if (t != 0 && t != numCircleSegments)
			verts.push_back(verts.back());
	}

	// Bottom circle.
	for (int t = 0; t < numCircleSegments + 1; ++t) {
		float s, c;
		SinCos(sideStepRadiands * (float)t, s, c);
		addVert(base + xAxisRadius * c + zAxisRadius * s + yAxisBottomCircleLevel);
		if (t != 0 && t != numCircleSegments)
			verts.push_back(verts.back());
	}

	// Top X-axis arc.
	for (int t = 0; t < numArcSegments + 1; ++t) {
		float s, c;
		SinCos(sideStepRadiands * (float)t, s, c);
		addVert(base + xAxisRadius * c + (yAxis * radius * s + yAxisTopCircleLevel));
		if (t != 0 && t != numArcSegments)
			verts.push_back(verts.back());
	}

	// Top Z-axis arc.
	for (int t = 0; t < numArcSegments + 1; ++t) {
		float s, c;
		SinCos(sideStepRadiands * (float)t, s, c);
		addVert(base + zAxisRadius * c + (yAxis * radius * s + yAxisTopCircleLevel));
		if (t != 0 && t != numArcSegments)
			verts.push_back(verts.back());
	}

	// Bottom X-axis arc.
	for (int t = 0; t < numArcSegments + 1; ++t) {
		float s, c;
		SinCos(sideStepRadiands * (float)t, s, c);
		addVert(base + xAxisRadius * c + (-yAxis * radius * s + yAxisBottomCircleLevel));
		if (t != 0 && t != numArcSegments)
			verts.push_back(verts.back());
	}

	// Bottom Z-axis arc.
	for (int t = 0; t < numArcSegments + 1; ++t) {
		float s, c;
		SinCos(sideStepRadiands * (float)t, s, c);
		addVert(base + zAxisRadius * c + (-yAxis * radius * s + yAxisBottomCircleLevel));
		if (t != 0 && t != numArcSegments)
			verts.push_back(verts.back());
	}

	// Add the 4 side lines along the capsule height...
	// +x
	addVert(base + (xAxisRadius + yAxisBottomCircleLevel));
	addVert(base + (xAxisRadius + yAxisTopCircleLevel));

	// -x
	addVert(base + (-xAxisRadius + yAxisBottomCircleLevel));
	addVert(base + (-xAxisRadius + yAxisTopCircleLevel));

	// +z
	addVert(base + (zAxisRadius + yAxisBottomCircleLevel));
	addVert(base + (zAxisRadius + yAxisTopCircleLevel));

	// -z
	addVert(base + (-zAxisRadius + yAxisBottomCircleLevel));
	addVert(base + (-zAxisRadius + yAxisTopCircleLevel));
}

void GeomGen::wiredSphere(std::vector<PosColorVert>& verts, const mat4f& transform, const uint32 rgba, float radius, int numSideDivs) {
	auto addVert = [&](const vec3f& pos) { verts.push_back(PosColorVert(pos, rgba)); };

	const vec3f xAxis = transform.data[0].xyz() * radius;
	const vec3f yAxis = transform.data[1].xyz() * radius;
	const vec3f zAxis = transform.data[2].xyz() * radius;
	const vec3f base = transform.data[3].xyz();

	const int numCircleSegments = numSideDivs * 4;
	const int numArcSegments = numCircleSegments / 2;
	const float sideStepRadiands = sge2Pi / (float)numCircleSegments;

	// XY circle.
	for (int t = 0; t < numCircleSegments + 1; ++t) {
		float s, c;
		SinCos(sideStepRadiands * (float)t, s, c);
		addVert(base + xAxis * c + yAxis * s);
		if (t != 0 && t != numCircleSegments)
			verts.push_back(verts.back());
	}

	// ZY circle.
	for (int t = 0; t < numCircleSegments + 1; ++t) {
		float s, c;
		SinCos(sideStepRadiands * (float)t, s, c);
		addVert(base + zAxis * c + yAxis * s);
		if (t != 0 && t != numCircleSegments)
			verts.push_back(verts.back());
	}

	// XZ circle.
	for (int t = 0; t < numCircleSegments + 1; ++t) {
		float s, c;
		SinCos(sideStepRadiands * (float)t, s, c);
		addVert(base + xAxis * c + zAxis * s);
		if (t != 0 && t != numCircleSegments)
			verts.push_back(verts.back());
	}
}

void GeomGen::wiredCylinder(std::vector<PosColorVert>& verts,
                            const mat4f& transform,
                            uint32 rgba,
                            float height,
                            float radius,
                            int numSideDivs,
                            const Origin origin) {
	sgeAssert(numSideDivs > 0.f);

	auto addVert = [&](const vec3f& pos) { verts.push_back(PosColorVert(pos, rgba)); };

	const vec3f xAxis = transform.data[0].xyz() * radius;
	const vec3f yAxis = transform.data[1].xyz();
	const vec3f zAxis = transform.data[2].xyz() * radius;
	const vec3f base = [&]() -> vec3f {
		// The algorithm below generatesa capsule that uses "bases" as the most bottom point of the capsule.
		if (origin == Bottom)
			return transform.data[3].xyz();
		if (origin == center)
			return transform.data[3].xyz() - (height * 0.5f) * yAxis;

		sgeAssert(false && "Unimplemented center type.");
		return transform.data[3].xyz(); // Just return something default.
	}();

	const vec3f yAxisSized = yAxis * height;

	const int numCircleSegments = numSideDivs * 4;
	const float sideStepRadiands = sge2Pi / (float)numCircleSegments;

	// Bottom circle.
	for (int t = 0; t < numCircleSegments + 1; ++t) {
		float s, c;
		SinCos(sideStepRadiands * (float)t, s, c);
		addVert(base + xAxis * c + zAxis * s + yAxisSized);
		if (t != 0 && t != numCircleSegments)
			verts.push_back(verts.back());
	}

	// Top circle.
	for (int t = 0; t < numCircleSegments + 1; ++t) {
		float s, c;
		SinCos(sideStepRadiands * (float)t, s, c);
		addVert(base + xAxis * c + zAxis * s);
		if (t != 0 && t != numCircleSegments)
			verts.push_back(verts.back());
	}

	// Add the 4 side lines along the cylinder height...
	// +x
	addVert(base + (xAxis));
	addVert(base + (xAxis + yAxisSized));

	// -x
	addVert(base + (-xAxis));
	addVert(base + (-xAxis + yAxisSized));

	// +z
	addVert(base + (zAxis));
	addVert(base + (zAxis + yAxisSized));

	// -z
	addVert(base + (-zAxis));
	addVert(base + (-zAxis + yAxisSized));
}

void GeomGen::wiredCone(std::vector<PosColorVert>& verts,
                        const mat4f& transform,
                        uint32 rgba,
                        float height,
                        float radius,
                        int numSideDivs,
                        const Origin origin) {
	sgeAssert(numSideDivs > 0.f);

	auto addVert = [&](const vec3f& pos) { verts.push_back(PosColorVert(pos, rgba)); };

	const vec3f xAxis = transform.data[0].xyz() * radius;
	const vec3f yAxis = transform.data[1].xyz();
	const vec3f zAxis = transform.data[2].xyz() * radius;
	const vec3f base = [&]() -> vec3f {
		// The algorithm below generatesa capsule that uses "bases" as the most bottom point of the capsule.
		if (origin == Bottom)
			return transform.data[3].xyz();
		if (origin == center)
			return transform.data[3].xyz() - (height * 0.5f) * yAxis;

		sgeAssert(false && "Unimplemented center type.");
		return transform.data[3].xyz(); // Just return something default.
	}();

	const vec3f yAxisSized = yAxis * height;

	const int numCircleSegments = numSideDivs * 4;
	const float sideStepRadiands = sge2Pi / (float)numCircleSegments;

	// Bottom circle.
	for (int t = 0; t < numCircleSegments + 1; ++t) {
		float s, c;
		SinCos(sideStepRadiands * (float)t, s, c);
		addVert(base + xAxis * c + zAxis * s + yAxisSized);
		if (t != 0 && t != numCircleSegments)
			verts.push_back(verts.back());
	}

	// Add the 4 side lines along the capsule height...
	// +x
	addVert(base);
	addVert(base + (xAxis + yAxisSized));

	// -x
	addVert(base);
	addVert(base + (-xAxis + yAxisSized));

	// +z
	addVert(base);
	addVert(base + (zAxis + yAxisSized));

	// -z
	addVert(base);
	addVert(base + (-zAxis + yAxisSized));
}

int GeomGen::wiredBasis(std::vector<PosColorVert>& verts, const mat4f& transform) {
	const vec3f& base = transform.data[3].xyz();

	// X axis
	verts.push_back(PosColorVert(base, 0xFF0000FF));
	verts.push_back(PosColorVert(base + transform.data[0].xyz(), 0xFF0000FF));

	// Y axis
	verts.push_back(PosColorVert(base, 0xFF00FF00));
	verts.push_back(PosColorVert(base + transform.data[1].xyz(), 0xFF00FF00));

	// Z axis
	verts.push_back(PosColorVert(base, 0xFFFF0000));
	verts.push_back(PosColorVert(base + transform.data[2].xyz(), 0xFFFF0000));

	return (int)verts.size();
}

void GeomGen::wiredGrid(
    std::vector<PosColorVert>& verts, const vec3f& origin, const vec3f& xAxis, const vec3f& zAxis, int xLines, int zLines, int color) {
	// +X
	verts.push_back(PosColorVert(origin, 0xFF0000FF));
	verts.push_back(PosColorVert(origin + xAxis * (float)(xLines), 0xFF0000FF));

	// -X
	verts.push_back(PosColorVert(origin, 0xFF000055));
	verts.push_back(PosColorVert(origin - xAxis * (float)(xLines), 0xFF000055));

	// +Z
	verts.push_back(PosColorVert(origin, 0xFFFF0000));
	verts.push_back(PosColorVert(origin + zAxis * (float)(zLines), 0xFFFF0000));

	// -Z
	verts.push_back(PosColorVert(origin, 0xFF550000));
	verts.push_back(PosColorVert(origin - zAxis * (float)(zLines), 0xFF550000));

	for (int t = -xLines; t < xLines + 1; ++t) {
		if (t == 0)
			continue;

		verts.push_back(PosColorVert(origin + xAxis * (float)t - zAxis * (float)zLines, color));
		verts.push_back(PosColorVert(origin + xAxis * (float)t + zAxis * (float)zLines, color));
	}

	for (int t = -zLines; t < zLines + 1; ++t) {
		if (t == 0)
			continue;

		verts.push_back(PosColorVert(origin - xAxis * (float)xLines + zAxis * (float)(t), color));
		verts.push_back(PosColorVert(origin + xAxis * (float)xLines + zAxis * (float)(t), color));
	}
}

void GeomGen::createChecker(Texture* texture, const int size, const int checkSize, const SamplerDesc& prefferedSampler) {
	TextureDesc td;
	td.format = TextureFormat::R8G8B8A8_UNORM;
	td.usage = TextureUsage::ImmutableResource;
	td.textureType = UniformType::Texture2D;
	td.texture2D.arraySize = 1;
	td.texture2D.width = size;
	td.texture2D.height = size;
	td.texture2D.numMips = 1;
	td.texture2D.numSamples = 1;
	td.texture2D.sampleQuality = 0;

	std::vector<char> data(4 * size * size);

	struct rgb {
		unsigned char r, g, b, a;
	};

	rgb* const d = (rgb*)data.data();

	for (int u = 0; u < size; ++u)
		for (int v = 0; v < size; ++v) {
			const int c0 = (u / checkSize) % 2;
			const int c1 = (v / checkSize) % 2;
			const int c = (c0 + c1) % 2 ? 255 : 0;

			d[u + size * v].r = char(c);
			d[u + size * v].g = char(c);
			d[u + size * v].b = char(c);
			d[u + size * v].a = 255;
		}

	const TextureData texData(d, size * 4);
	texture->create(td, &texData, prefferedSampler);
}

} // namespace sge
