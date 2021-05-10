#pragma once

#include "common.h"
#include "math_base.h"
#include <cmath> // isnan

#include "vec2.h"

namespace sge {

template <typename TDATA_TYPE>
struct vec3 {
	typedef TDATA_TYPE DATA_TYPE;
	typedef vec3 SELF_TYPE;
	static const unsigned int NUM_ELEMS = 3;

	union {
		struct {
			DATA_TYPE x, y, z;
		};
		DATA_TYPE data[NUM_ELEMS];
	};

	vec3() = default;

	explicit vec3(const DATA_TYPE& s) {
		for (unsigned int t = 0; t < NUM_ELEMS; ++t) {
			data[t] = s;
		}
	}

	vec3(const DATA_TYPE& _x, const DATA_TYPE& _y, const DATA_TYPE& _z)
	    : x(_x)
	    , y(_y)
	    , z(_z) {}

	vec3(const vec2<DATA_TYPE>& xy, const DATA_TYPE& _z)
	    : x(xy[0])
	    , y(xy[1])
	    , z(_z) {}

	vec3(const DATA_TYPE& _x, const vec2<DATA_TYPE>& yz)
	    : x(_x)
	    , y(yz[0])
	    , z(yz[1]) {}

	template <typename T>
	explicit vec3(const vec3<T>& ref) {
		for (int t = 0; t < NUM_ELEMS; ++t) {
			data[t] = DATA_TYPE(ref.data[t]);
		}
	}

	/// Returns only the i-th axis being non-zero with value @axisLen
	static vec3 getAxis(const unsigned int& axisIndex, const DATA_TYPE& axisLen = DATA_TYPE(1.0)) {
		vec3 result(0);
		result[axisIndex] = axisLen;
		return result;
	}

	static vec3 axis_x(const DATA_TYPE& axisLen = DATA_TYPE(1.0)) { return vec3(axisLen, 0, 0); }

	static vec3 axis_y(const DATA_TYPE& axisLen = DATA_TYPE(1.0)) { return vec3(0, axisLen, 0); }

	static vec3 axis_z(const DATA_TYPE& axisLen = DATA_TYPE(1.0)) { return vec3(0, 0, axisLen); }

	static vec3 getY(const DATA_TYPE& axisLen = DATA_TYPE(1.0)) {
		vec3 result(0);
		result[1] = axisLen;
		return result;
	}

	/// Returns a vector containing only zeroes.
	static vec3 getZero() { return vec3(0); }

	/// Sets the data of the vector form assumingly properly sized c-array.
	void set_data(const DATA_TYPE* const pData) {
		data[0] = pData[0];
		data[1] = pData[1];
		data[2] = pData[2];
	}

	// Less and greater operators.
	friend bool operator<(const vec3& a, const vec3& b) {
		if (a[0] < b[0])
			return true;
		if (a[1] < b[1])
			return true;
		if (a[2] < b[2])
			return true;

		return false;
	}

	friend bool operator<=(const vec3& a, const vec3& b) {
		if (a[0] <= b[0])
			return true;
		if (a[1] <= b[1])
			return true;
		if (a[2] <= b[2])
			return true;

		return false;
	}


	friend bool operator>(const vec3& a, const vec3& b) {
		if (a[0] > b[0])
			return true;
		if (a[1] > b[1])
			return true;
		if (a[2] > b[2])
			return true;

		return false;
	}

	friend bool operator>=(const vec3& a, const vec3& b) {
		if (a[0] >= b[0])
			return true;
		if (a[1] >= b[1])
			return true;
		if (a[2] >= b[2])
			return true;

		return false;
	}

	// Indexing operators.
	DATA_TYPE& operator[](const int t) {
		sgeAssert(t >= 0 && t < NUM_ELEMS);
		return data[t];
	}
	const DATA_TYPE& operator[](const int t) const {
		sgeAssert(t >= 0 && t < NUM_ELEMS);
		return data[t];
	}

	// Operator == and != implemented by direct comparison.
	bool operator==(const vec3& v) const { return (data[0] == v[0]) && (data[1] == v[1]) && (data[2] == v[2]); }

	bool operator!=(const vec3& v) const { return !((*this) == v); }

	// Unary operators - +
	vec3 operator-() const {
		vec3 result;

		result[0] = -data[0];
		result[1] = -data[1];
		result[2] = -data[2];

		return result;
	}

	vec3 operator+() const { return *this; }

	// vec + vec
	vec3& operator+=(const vec3& v) {
		data[0] += v[0];
		data[1] += v[1];
		data[2] += v[2];
		return *this;
	}

	vec3 operator+(const vec3& v) const {
		vec3 r(*this);
		r += v;
		return r;
	}

	// vec - vec
	vec3& operator-=(const vec3& v) {
		data[0] -= v[0];
		data[1] -= v[1];
		data[2] -= v[2];
		return *this;
	}


	vec3 operator-(const vec3& v) const {
		vec3 r(*this);
		r -= v;
		return r;
	}

	// Vector * Scalar (and vice versa)
	vec3& operator*=(const DATA_TYPE& s) {
		data[0] *= s;
		data[1] *= s;
		data[2] *= s;
		return *this;
	}

	vec3 operator*(const DATA_TYPE& s) const {
		vec3 r(*this);
		r *= s;
		return r;
	}

	friend vec3 operator*(const DATA_TYPE& s, const vec3& v) { return v * s; }


	// Vector / Scalar
	vec3& operator/=(const DATA_TYPE& s) {
		data[0] /= s;
		data[1] /= s;
		data[2] /= s;
		return *this;
	}

	vec3 operator/(const DATA_TYPE& s) const {
		vec3 r(*this);
		r /= s;
		return r;
	}

	// Vector * Vector
	vec3& operator*=(const vec3& v) {
		data[0] *= v.data[0];
		data[1] *= v.data[1];
		data[2] *= v.data[2];
		return *this;
	}

	vec3 operator*(const vec3& s) const {
		vec3 r(*this);
		r *= s;
		return r;
	}

	// Vector / Vector
	vec3& operator/=(const vec3& v) {
		data[0] /= v.data[0];
		data[1] /= v.data[1];
		data[2] /= v.data[2];
		return *this;
	}

	vec3 operator/(const vec3& s) const {
		vec3 r(*this);
		r /= s;
		return r;
	}

	/// Performs a component-wise division of two vectors.
	/// Assumes that non of the elements in @b are 0
	friend vec3 cdiv(const vec3& a, const vec3& b) {
		vec3 r;
		r[0] = a[0] / b[0];
		r[1] = a[1] / b[1];
		r[2] = a[2] / b[2];
		return r;
	}

	/// Returns a sum of all elements in the vector.
	DATA_TYPE hsum() const {
		DATA_TYPE r = data[0];
		r += data[1];
		r += data[2];
		return r;
	}

	friend DATA_TYPE hsum(const vec3& v) { return v.hsum(); }

	/// Computes the dot product between two vectors.
	DATA_TYPE dot(const vec3& v) const {
		DATA_TYPE r = data[0] * v[0];
		r += data[1] * v[1];
		r += data[2] * v[2];

		return r;
	}

	friend DATA_TYPE dot(const vec3& a, const vec3& b) { return a.dot(b); }

	/// Computes the length of the vector.
	DATA_TYPE lengthSqr() const { return dot(*this); }

	DATA_TYPE length() const {
		// [TODO]
		return sqrtf(lengthSqr());
	}

	friend DATA_TYPE length(const vec3& v) { return v.length(); }

	/// Computes the unsigned-volume of a cube.
	DATA_TYPE volume() const {
		DATA_TYPE r = abs(data[0]);
		r *= abs(data[1]);
		r *= abs(data[2]);

		return r;
	}

	/// Returns the normalized vector (with length 1.f).
	/// Assumes that the vector current size is not 0.
	vec3 normalized() const {
		const DATA_TYPE invLength = DATA_TYPE(1.0) / length();

		vec3 result;
		result[0] = data[0] * invLength;
		result[1] = data[1] * invLength;
		result[2] = data[2] * invLength;

		return result;
	}

	friend vec3 normalized(const vec3& v) { return v.normalized(); }

	/// Returns the normalized vector (with length 1.f).
	/// If the size of the vector is 0, the zero vector is returned.
	vec3 normalized0() const {
		DATA_TYPE lenSqr = lengthSqr();
		if (lengthSqr() < 1e-6f) {
			return vec3(0);
		}

		const DATA_TYPE invLength = DATA_TYPE(1.0) / sqrt(lenSqr);

		vec3 result;
		result[0] = data[0] * invLength;
		result[1] = data[1] * invLength;
		result[2] = data[2] * invLength;

		return result;
	}

	friend vec3 normalized0(const vec3& v) { return v.normalized0(); }

	vec3 reciprocalSafe(DATA_TYPE fallbackIfZero = DATA_TYPE(1e-6f)) const {
		vec3 v;
		v.data[0] = fabsf(data[0]) > DATA_TYPE(1e-6f) ? 1 / data[0] : fallbackIfZero;
		v.data[1] = fabsf(data[1]) > DATA_TYPE(1e-6f) ? 1 / data[1] : fallbackIfZero;
		v.data[2] = fabsf(data[2]) > DATA_TYPE(1e-6f) ? 1 / data[2] : fallbackIfZero;

		return v;
	}

	/// Interpolates two vectors with the a speed (defined in units).
	/// Asuumes that @speed is >= 0
	friend vec3 speedLerp(const vec3& a, const vec3& b, const DATA_TYPE speed, const DATA_TYPE epsilon = 1e-6f) {
		vec3 const diff = b - a;
		DATA_TYPE const diffLen = diff.length();

		// if the two points are too close together just return the target point.
		if (diffLen < epsilon) {
			return b;
		}

		// TODO: handle double here.
		float k = float(speed) / float(diffLen);

		if (k > 1.f) {
			k = 1.f;
		}

		return a + DATA_TYPE(k) * diff;
	}


	/// Computes the reflected vector based on the normal specified. Assuming IOR=1
	vec3 reflect(const vec3& normal) const { return (*this) + DATA_TYPE(2.0) * dot(normal) * normal; }

	friend DATA_TYPE reflect(const vec3& incident, const vec3& normal) { return incident.reflect(normal); }

	/// Computes the refracted vector based on the normal specified.
	vec3 refract(const vec3& normal, const DATA_TYPE& eta) const {
		const DATA_TYPE one(1.0);
		const DATA_TYPE zero(0.0);

		const DATA_TYPE p = Dot(normal);
		const DATA_TYPE k = one - eta * eta * (one - p * p);

		if (k < zero)
			return vec3(zero);
		else
			return (*this) * eta - (eta * p + sqrt(k)) * normal;
	}

	friend vec3 refract(const vec3& inc, const vec3& n, DATA_TYPE& eta) { return inc.refract(n, eta); }

	/// Computes the distance between two vectors.
	DATA_TYPE distance(const vec3& other) const { return (*this - other).length(); }

	friend DATA_TYPE distance(const vec3& a, const vec3& b) { return a.distance(b); }

	/// Retrieves the minimum of each pairs of components by picking the smallest one.
	vec3 pickMin(const vec3& other) const {
		vec3 result;

		result.data[0] = minOf(data[0], other.data[0]);
		result.data[1] = minOf(data[1], other.data[1]);
		result.data[2] = minOf(data[2], other.data[2]);

		return result;
	}

	/// Retrieves the minimum of each pairs of components by picking the biggest one.
	vec3 pickMax(const vec3& other) const {
		vec3 result;
		result.data[0] = maxOf(data[0], other.data[0]);
		result.data[1] = maxOf(data[1], other.data[1]);
		result.data[2] = maxOf(data[2], other.data[2]);

		return result;
	}

	/// Returns the min component in the vector.
	DATA_TYPE componentMin() const {
		DATA_TYPE retval = data[0];

		if (data[1] < retval) {
			retval = data[1];
		}

		if (data[2] < retval) {
			retval = data[2];
		}

		return retval;
	}

	/// @brief Computes the absolute values of each compoents in the vector and returns the smallest one.
	DATA_TYPE componentMinAbs() const {
		DATA_TYPE retval = std::abs(data[0]);

		{
			float abs = std::abs(data[1]);
			if (abs < retval) {
				retval = abs;
			}
		}

		{
			float abs = std::abs(data[2]);
			if (abs < retval) {
				retval = abs;
			}
		}

		return retval;
	}
	
	/// Returns the max component in the vector.
	DATA_TYPE componentMax() const {
		DATA_TYPE retval = data[0];

		if (data[1] > retval) {
			retval = data[1];
		}

		if (data[2] > retval) {
			retval = data[2];
		}

		return retval;
	}

	friend vec3 pick_max(const vec3& v) { return v.componentMax(); }

	DATA_TYPE componentMaxAbs() const {
		DATA_TYPE retval = std::abs(data[0]);

		{
			float abs = std::abs(data[1]);
			if (abs > retval) {
				retval = abs;
			}
		}

		{
			float abs = std::abs(data[2]);
			if (abs > retval) {
				retval = abs;
			}
		}

		return retval;
	}

	vec3 getAbs() const {
		vec3 retval = *this;
		retval.x = std::abs(retval.x);
		retval.y = std::abs(retval.y);
		retval.z = std::abs(retval.z);

		return retval;
	}

	vec3 getSorted() const {
		vec3 retval = *this;

		for (int t = 0; t < vec3::NUM_ELEMS; ++t)
			for (int k = t + 1; k < vec3::NUM_ELEMS; ++k) {
				if (retval.data[t] > retval.data[k]) {
					DATA_TYPE x = retval.data[t];
					retval.data[t] = retval.data[k];
					retval.data[k] = x;
				}
			}

		return retval;
	}

	static vec3 getSignedAxis(const SignedAxis sa) {
		switch (sa) {
			case axis_x_pos:
				return vec3(1, 0, 0);
			case axis_y_pos:
				return vec3(0, 1, 0);
			case axis_z_pos:
				return vec3(0, 0, 1);
			case axis_x_neg:
				return vec3(-1, 0, 0);
			case axis_y_neg:
				return vec3(0, -1, 0);
			case axis_z_neg:
				return vec3(0, 0, -1);
		}

		sgeAssert(false);
		return vec3(DATA_TYPE(0));
	}

	//---------------------------------------------------
	// cross product
	//---------------------------------------------------
	SELF_TYPE cross(const SELF_TYPE& v) const {
		const float tx = (y * v.z) - (v.y * z);
		const float ty = (v.x * z) - (x * v.z);
		const float tz = (x * v.y) - (v.x * y);

		return SELF_TYPE(tx, ty, tz);
	}

	friend vec3 cross(const vec3& a, const vec3& b) { return a.cross(b); }

	//---------------------------------------------------
	// tiple product: dot(v, cross(a,b))
	//---------------------------------------------------
	friend DATA_TYPE triple(const vec3& v, const vec3& a, const vec3& b) { return v.dot(a.cross(b)); }


	vec3 anyPerpendicular() const {
		vec3 r = vec3(-y, x, z);
		if (!isEpsZero(r.dot(*this))) {
			r = vec3(-z, 0, x);
		}
		if (!isEpsZero(r.dot(*this))) {
			r = vec3(0, -z, y);
		}
		if (!isEpsZero(r.dot(*this))) {
			r = vec3(-y, x, 0);
		}
		sgeAssert(isEpsZero(r.dot(*this)));
		return r;
	}

	//---------------------------------------------------
	// spherical(right-handed with z-up) to cartesian
	// azimuth - the angle on xy plane starting form +x
	// polar - the angle stating form +z and goes to xy plane
	//---------------------------------------------------
	// void spherical_to_cartesian_rh_z_up(const DATA_TYPE& azimuth, const DATA_TYPE& polar, const DATA_TYPE& radius) const
	//{
	//	radius = length();
	//	azimuth = sgm::atan2(data[1], data[0]);
	//	polar = sgm::acos(data[3] / radius);
	//}

	vec2<DATA_TYPE> xz() const { return vec2<DATA_TYPE>(data[0], data[2]); }

	vec2<DATA_TYPE> xy() const { return vec2<DATA_TYPE>(data[0], data[1]); }

	vec3 x00() const { return vec3(data[0], 0.f, 0.f); }
	vec3 x0z() const { return vec3(data[0], 0.f, data[2]); }
	vec3 xy0() const { return vec3(data[0], data[1], 0.f); }

	vec3 xOnly() const { return vec3(data[0], 0.f, 0.f); }
	vec3 yOnly() const { return vec3(0.f, data[1], 0.f); }
	vec3 zOnly() const { return vec3(0.f, 0.f, data[2]); }

	vec3 _0yz() const { return vec3(0.f, data[1], data[2]); }

	bool hasNan() const { return std::isnan(data[0]) || std::isnan(data[1]) || std::isnan(data[1]); }
};

typedef vec3<int> vec3i;
typedef vec3<unsigned int> vec3u;
typedef vec3<float> vec3f;
typedef vec3<double> vec3d;

} // namespace sge
