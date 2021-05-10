#pragma once

#include <limits>

#include "common.h"
#include "mat4.h"
#include "math_base.h"
#include "vec.h"

#if defined(min) || defined(max)
#error min/max macros are defined
#endif

namespace sge {

template <class T, unsigned int N>
struct AABox {
	typedef T DATA_TYPE;
	typedef typename vec_type_picker<DATA_TYPE, N>::type VEC_TYPE;


	AABox() { setEmpty(); }

	AABox(const VEC_TYPE& min, const VEC_TYPE& max)
	    : min(min)
	    , max(max) {}

	static AABox getFromHalfDiagonal(const VEC_TYPE& halfDiagonal, const VEC_TYPE& offset = vec3f(0.f)) {
		sgeAssert(halfDiagonal.x >= 0 && halfDiagonal.y >= 0 && halfDiagonal.z >= 0);

		AABox retval;
		retval.max = halfDiagonal + offset;
		retval.min = -halfDiagonal + offset;

		return retval;
	}

	static AABox getFromHalfDiagonalWithOffset(const VEC_TYPE& halfDiagonal, const VEC_TYPE& offset) {
		sgeAssert(halfDiagonal.x >= 0 && halfDiagonal.y >= 0 && halfDiagonal.z >= 0);

		AABox retval;
		retval.max = halfDiagonal + offset;
		retval.min = -halfDiagonal + offset;

		return retval;
	}

	static AABox getFromXZBaseAndSize(const VEC_TYPE& base, const VEC_TYPE& sizes) {
		VEC_TYPE min = base;
		min.x -= sizes.x / (DATA_TYPE)2;
		min.z -= sizes.z / (DATA_TYPE)2;

		VEC_TYPE max = base;
		max.x += sizes.x / (DATA_TYPE)2;
		max.y += sizes.y;
		max.z += sizes.z / (DATA_TYPE)2;

		return AABox(min, max);
	}

	void move(const VEC_TYPE& offset) {
		min += offset;
		max += offset;
	}

	VEC_TYPE center() const { return (max + min) / (DATA_TYPE)2; }

	VEC_TYPE halfDiagonal() const { return (max - min) / (DATA_TYPE)2; }

	VEC_TYPE diagonal() const { return (max - min); }

	VEC_TYPE bottomMiddle() const {
		VEC_TYPE ret = center() - halfDiagonal().yOnly();
		return ret;
	}

	VEC_TYPE topMiddle() const {
		VEC_TYPE ret = center() + halfDiagonal().yOnly();
		return ret;
	}

	VEC_TYPE size() const { return max - min; }

	void setEmpty() {
		min = VEC_TYPE(std::numeric_limits<DATA_TYPE>::max());
		max = VEC_TYPE(std::numeric_limits<DATA_TYPE>::lowest());
	}

	bool IsEmpty() const {
		return min == VEC_TYPE(std::numeric_limits<DATA_TYPE>::max()) && max == VEC_TYPE(std::numeric_limits<DATA_TYPE>::lowest());
	}

	void expand(const VEC_TYPE& point) {
		min = min.pickMin(point);
		max = max.pickMax(point);
	}

	void expandEdgesByCoefficient(float value) {
		VEC_TYPE exp = halfDiagonal() * value;
		min -= exp;
		max += exp;
	}

	void scale(const DATA_TYPE& s) {
		min *= s;
		max *= s;
	}

	void scale(const VEC_TYPE& s) {
		for (int t = 0; t < VEC_TYPE::NUM_ELEMS; ++t) {
			min.data[t] *= s.data[t];
			max.data[t] *= s.data[t];
		}
	}

	AABox getScaled(const VEC_TYPE& s) const {
		AABox result = *this;
		for (int t = 0; t < VEC_TYPE::NUM_ELEMS; ++t) {
			result.min.data[t] *= s.data[t];
			result.max.data[t] *= s.data[t];
		}

		return result;
	}

	AABox getScaledCentered(const VEC_TYPE& s) const {
		const VEC_TYPE c = center();
		VEC_TYPE h = halfDiagonal();
		AABox result;
		result.expand(c - h * s);
		result.expand(c + h * s);
		return result;
	}

	void scaleAroundCenter(const VEC_TYPE& s) {
		const VEC_TYPE oldCenter = center();
		const VEC_TYPE oldSize = size();

		const VEC_TYPE newSize = oldSize * s;

		setEmpty();
		expand(oldCenter + newSize * 0.5f);
		expand(oldCenter - newSize * 0.5f);
	}

	void expand(const AABox& other) {
		if (other.IsEmpty() == false) {
			min = min.pickMin(other.min);
			min = min.pickMin(other.max);

			max = max.pickMax(other.min);
			max = max.pickMax(other.max);
		}
	}

	bool isInside(const VEC_TYPE& point) const {
		bool res = true;
		for (int t = 0; t < VEC_TYPE::NUM_ELEMS; ++t) {
			res &= point.data[t] >= min.data[t] && point.data[t] <= max.data[t];
		}

		return res;
	}

	bool overlaps(const AABox& other) const {
		const bool xOverlap = doesOverlap1D(min.x, max.x, other.min.x, other.max.x);
		const bool yOverlap = doesOverlap1D(min.y, max.y, other.min.y, other.max.y);
		const bool zOverlap = doesOverlap1D(min.z, max.z, other.min.z, other.max.z);
		return xOverlap && yOverlap && zOverlap;
	}

	AABox getOverlapBox(const AABox& other) const {
		if (this->IsEmpty() || other.IsEmpty()) {
			return AABox();
		}

		AABox result;

		for (unsigned int t = 0; t < N; ++t) {
			float omin, omax;
			overlapSegment1D(min.data[t], max.data[t], other.min.data[t], other.max.data[t], omin, omax);

			// If the overlap is negative, that means that the boxes do not overlap.
			// In this case just return an empty box.
			if (omin > omax) {
				return AABox();
			}

			result.min[t] = omin;
			result.max[t] = omax;
		}

		return result;
	}

	/// Retireves the AABB corner points by index (0-7).
	VEC_TYPE getPoint(const int idx) const {
		if (idx == 0)
			return min;
		else if (idx == 1)
			return VEC_TYPE(min.x, min.y, max.z);
		else if (idx == 2)
			return VEC_TYPE(max.x, min.y, max.z);
		else if (idx == 3)
			return VEC_TYPE(max.x, min.y, min.z);
		else if (idx == 4)
			return VEC_TYPE(min.x, max.y, min.z);
		else if (idx == 5)
			return VEC_TYPE(min.x, max.y, max.z);
		else if (idx == 6)
			return max;
		else if (idx == 7)
			return VEC_TYPE(max.x, max.y, min.z);

		sgeAssert(false && "idx out of bounds! Needs to be in [0;7]");
		return VEC_TYPE(0.f);
	}

	/// Returns the transformed AXIS ALIGNED Bounding box.
	/// Performs a column-major matrix multiplication for every point.
	AABox getTransformed(const mat4<DATA_TYPE>& transform) const {
		if (this->IsEmpty())
			return *this;

		AABox retval;
		for (int t = 0; t < 8; ++t) {
			retval.expand((transform * vec4<DATA_TYPE>(getPoint(t), (DATA_TYPE)1)).xyz());
		}

		return retval;
	}

	bool intersectFast(const vec3f& origin, const vec3f& invDir, float& t) const {
		const float t1 = (min.x - origin.x) * invDir.x;
		const float t2 = (max.x - origin.x) * invDir.x;
		const float t3 = (min.y - origin.y) * invDir.y;
		const float t4 = (max.y - origin.y) * invDir.y;
		const float t5 = (min.z - origin.z) * invDir.z;
		const float t6 = (max.z - origin.z) * invDir.z;

		float tmin = maxOf(maxOf(minOf(t1, t2), minOf(t3, t4)), minOf(t5, t6));
		float tmax = minOf(minOf(maxOf(t1, t2), maxOf(t3, t4)), maxOf(t5, t6));

		// if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behing us.
		if (tmax < 0) {
			t = tmax;
			return false;
		}

		// if tmin > tmax, ray doesn't intersect AABB
		if (tmin > tmax) {
			t = tmax;
			return false;
		}

		t = tmin;
		return true;
	}

	bool intersect(const vec3f& origin, const vec3f& dir, float& t0, float& t1) const {
		float tmin, tmax, tymin, tymax, tzmin, tzmax;

		if (dir.x >= 0.f) {
			tmin = (min.x - origin.x) / dir.x;
			tmax = (max.x - origin.x) / dir.x;
		} else {
			tmin = (max.x - origin.x) / dir.x;
			tmax = (min.x - origin.x) / dir.x;
		}

		if (dir.y >= 0.f) {
			tymin = (min.y - origin.y) / dir.y;
			tymax = (max.y - origin.y) / dir.y;
		} else {
			tymin = (max.y - origin.y) / dir.y;
			tymax = (min.y - origin.y) / dir.y;
		}

		if ((tmin > tymax) || (tymin > tmax)) {
			return false;
		}

		if (tymin > tmin)
			tmin = tymin;
		if (tymax < tmax)
			tmax = tymax;

		if (dir.z >= 0.f) {
			tzmin = (min.z - origin.z) / dir.z;
			tzmax = (max.z - origin.z) / dir.z;
		} else {
			tzmin = (max.z - origin.z) / dir.z;
			tzmax = (min.z - origin.z) / dir.z;
		}

		if ((tmin > tzmax) || (tzmin > tmax)) {
			return false;
		}

		if (tzmin > tmin)
			tmin = tzmin;
		if (tzmax < tmax)
			tmax = tzmax;

		t0 = tmin;
		t1 = tmax;

		return true;
		// return ((tmin < t1) && (tmax > t0));
	}

	/// Moves the specified face of the bounding box along its normal.
	// AABox3f movedFaceBy(const SignedAxis axis, float amount) const {
	//	AABox3f tmp = *this;
	//	switch (axis) {
	//		case axis_x_pos: {
	//			tmp.max.x += amonut;
	//		} break;
	//		case axis_y_pos: {
	//			tmp.max.y += amonut;
	//		} break;
	//		case axis_z_pos: {
	//			tmp.max.z += amonut;
	//		} break;
	//		case axis_x_neg: {
	//			tmp.min.x -= amonut;
	//		} break;
	//		case axis_y_neg: {
	//			tmp.min.y -= amonut;
	//		} break;
	//		case axis_z_neg: {
	//			tmp.min.z += amonut;
	//		} break;
	//	}

	//	// Handle the situation where the amonut swapped the position of min/max points.
	//	AABox3f result;
	//	result.expand(tmp.min);
	//	result.expand(tmp.max);

	//	return result;
	//}

	/// Moves the specified face of the bounding box to the specified value along it's normal.
	AABox movedFaceTo(const SignedAxis axis, float pos) const {
		AABox tmp = *this;
		switch (axis) {
			case axis_x_pos: {
				tmp.max.x = pos;
			} break;
			case axis_y_pos: {
				tmp.max.y = pos;
			} break;
			case axis_z_pos: {
				tmp.max.z = pos;
			} break;
			case axis_x_neg: {
				tmp.min.x = pos;
			} break;
			case axis_y_neg: {
				tmp.min.y = pos;
			} break;
			case axis_z_neg: {
				tmp.min.z = pos;
			} break;
		}

		// Handle the situation where the amonut swapped the position of min/max points.
		AABox result;
		result.expand(tmp.min);
		result.expand(tmp.max);

		return result;
	}

	vec3f getFaceCenter(SignedAxis axis) const {
		const VEC_TYPE c = center();
		const VEC_TYPE halfDiag = halfDiagonal();

		if (axis == axis_x_pos)
			return c + halfDiag.xOnly();
		else if (axis == axis_y_pos)
			return c + halfDiag.yOnly();
		else if (axis == axis_z_pos)
			return c + halfDiag.zOnly();
		else if (axis == axis_x_neg)
			return c - halfDiag.xOnly();
		else if (axis == axis_y_neg)
			return c - halfDiag.yOnly();
		else if (axis == axis_z_neg)
			return c - halfDiag.zOnly();

		sgeAssert(false);
		return vec3f(0.f);
	}

	void getFacesCenters(VEC_TYPE facesCenters[signedAxis_numElements]) const {
		const VEC_TYPE c = center();
		const VEC_TYPE halfDiag = halfDiagonal();

		facesCenters[axis_x_pos] = c + halfDiag.xOnly();
		facesCenters[axis_y_pos] = c + halfDiag.yOnly();
		facesCenters[axis_z_pos] = c + halfDiag.zOnly();
		facesCenters[axis_x_neg] = c - halfDiag.xOnly();
		facesCenters[axis_y_neg] = c - halfDiag.yOnly();
		facesCenters[axis_z_neg] = c - halfDiag.zOnly();
	}

	void getFacesNormals(VEC_TYPE facesCenters[signedAxis_numElements]) const {
		facesCenters[axis_x_pos] = VEC_TYPE::getAxis(0);
		facesCenters[axis_y_pos] = VEC_TYPE::getAxis(1);
		facesCenters[axis_z_pos] = VEC_TYPE::getAxis(2);
		facesCenters[axis_x_neg] = -VEC_TYPE::getAxis(0);
		facesCenters[axis_y_neg] = -VEC_TYPE::getAxis(1);
		facesCenters[axis_z_neg] = -VEC_TYPE::getAxis(2);
	}

	bool operator==(const AABox& other) const { return (min == other.min) && (max == other.max); }

	bool operator!=(const AABox& other) const { return !(*this == other); }

	VEC_TYPE min, max;
};

typedef AABox<float, 2> AABox2f;
typedef AABox<int, 2> AABox2i;
typedef AABox<short, 2> AABox2s;
typedef AABox<float, 3> AABox3f;
typedef AABox<int, 3> AABox3i;

} // namespace sge
