#pragma once

#include "common.h"
#include "math_base.h"
#include <cmath> // isnan

#include "vec3.h" // Asuming includes vec2.h

namespace sge {

template <typename TDATA_TYPE>
struct vec4 {
	typedef TDATA_TYPE DATA_TYPE;
	typedef vec4 SELF_TYPE;
	static const unsigned int NUM_ELEMS = 4;

	union {
		struct {
			DATA_TYPE x, y, z, w;
		};
		DATA_TYPE data[NUM_ELEMS];
	};

	vec4() = default;

	explicit vec4(const DATA_TYPE& s) {
		data[0] = s;
		data[1] = s;
		data[2] = s;
		data[3] = s;
	}

	vec4(const DATA_TYPE& _x, const DATA_TYPE& _y, const DATA_TYPE& _z, const DATA_TYPE& _w)
	    : x(_x)
	    , y(_y)
	    , z(_z)
	    , w(_w) {}

	vec4(const vec2<DATA_TYPE>& xy, const vec2<DATA_TYPE>& zw)
	    : x(xy[0])
	    , y(xy[1])
	    , z(zw[0])
	    , w(zw[1]) {}

	vec4(const vec2<DATA_TYPE>& xy, const DATA_TYPE& z, const DATA_TYPE& w)
	    : x(xy[0])
	    , y(xy[1])
	    , z(z)
	    , w(w) {}

	vec4(const DATA_TYPE& x, const DATA_TYPE& y, const vec2<DATA_TYPE>& zw)
	    : x(x)
	    , y(y)
	    , z(zw[0])
	    , w(zw[1]) {}

	vec4(const DATA_TYPE& _x, const vec3<DATA_TYPE>& v)
	    : x(_x)
	    , y(v[0])
	    , z(v[1])
	    , w(v[2]) {}

	vec4(const vec3<DATA_TYPE>& v, const DATA_TYPE& _w)
	    : x(v[0])
	    , y(v[1])
	    , z(v[2])
	    , w(_w) {}

	template <typename T>
	explicit vec4(const vec4<T>& ref) {
		data[0] = DATA_TYPE(ref.data[0]);
		data[1] = DATA_TYPE(ref.data[1]);
		data[2] = DATA_TYPE(ref.data[2]);
		data[3] = DATA_TYPE(ref.data[3]);
	}

	void setXyz(const vec3<DATA_TYPE>& v3) {
		data[0] = v3.data[0];
		data[1] = v3.data[1];
		data[2] = v3.data[2];
	}

	vec3<DATA_TYPE> xyz() const { return vec3<DATA_TYPE>(data[0], data[1], data[2]); }

	vec3<DATA_TYPE> wyz() const { return vec3<DATA_TYPE>(w, y, z); }

	vec3<DATA_TYPE> xwz() const { return vec3<DATA_TYPE>(x, w, z); }

	vec3<DATA_TYPE> xyw() const { return vec3<DATA_TYPE>(x, y, w); }

	vec2<DATA_TYPE> xy() const { return vec2<DATA_TYPE>(data[0], data[1]); }


	/// Returns only the i-th axis being non-zero with value @axisLen
	static vec4 getAxis(const unsigned int& axisIndex, const DATA_TYPE& axisLen = DATA_TYPE(1.0)) {
		vec4 result(0);
		result[axisIndex] = axisLen;
		return result;
	}

	/// Returns a vector containing only zeroes.
	static vec4 getZero() { return vec4(0); }

	/// Sets the data of the vector form assumingly properly sized c-array.
	void set_data(const DATA_TYPE* const pData) {
		data[0] = pData[0];
		data[1] = pData[1];
		data[2] = pData[2];
		data[3] = pData[3];
	}

	// Less and greater operators.
	friend bool operator<(const vec4& a, const vec4& b) {
		if (a[0] < b[0])
			return true;
		if (a[1] < b[1])
			return true;
		if (a[2] < b[2])
			return true;
		if (a[3] < b[3])
			return true;

		return false;
	}

	friend bool operator<=(const vec4& a, const vec4& b) {
		if (a[0] <= b[0])
			return true;
		if (a[1] <= b[1])
			return true;
		if (a[2] <= b[2])
			return true;
		if (a[3] <= b[3])
			return true;

		return false;
	}


	friend bool operator>(const vec4& a, const vec4& b) {
		if (a[0] > b[0])
			return true;
		if (a[1] > b[1])
			return true;
		if (a[2] > b[2])
			return true;
		if (a[3] > b[3])
			return true;

		return false;
	}

	friend bool operator>=(const vec4& a, const vec4& b) {
		if (a[0] >= b[0])
			return true;
		if (a[1] >= b[1])
			return true;
		if (a[2] >= b[2])
			return true;
		if (a[3] >= b[3])
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
	bool operator==(const vec4& v) const { return (data[0] == v[0]) && (data[1] == v[1]) && (data[2] == v[2]) && (data[3] == v[3]); }

	bool operator!=(const vec4& v) const { return !((*this) == v); }

	// Unary operators - +
	vec4 operator-() const {
		vec4 result;

		result[0] = -data[0];
		result[1] = -data[1];
		result[2] = -data[2];
		result[3] = -data[3];

		return result;
	}

	vec4 operator+() const { return *this; }

	// vec + vec
	vec4& operator+=(const vec4& v) {
		data[0] += v[0];
		data[1] += v[1];
		data[2] += v[2];
		data[3] += v[3];
		return *this;
	}

	vec4 operator+(const vec4& v) const {
		vec4 r(*this);
		r += v;
		return r;
	}

	// vec - vec
	vec4& operator-=(const vec4& v) {
		data[0] -= v[0];
		data[1] -= v[1];
		data[2] -= v[2];
		data[3] -= v[3];
		return *this;
	}


	vec4 operator-(const vec4& v) const {
		vec4 r(*this);
		r -= v;
		return r;
	}

	// Vector * Scalar (and vice versa)
	vec4& operator*=(const DATA_TYPE& s) {
		data[0] *= s;
		data[1] *= s;
		data[2] *= s;
		data[3] *= s;
		return *this;
	}

	vec4 operator*(const DATA_TYPE& s) const {
		vec4 r(*this);
		r *= s;
		return r;
	}

	friend vec4 operator*(const DATA_TYPE& s, const vec4& v) { return v * s; }


	// Vector / Scalar
	vec4& operator/=(const DATA_TYPE& s) {
		data[0] /= s;
		data[1] /= s;
		data[2] /= s;
		data[3] /= s;
		return *this;
	}

	vec4 operator/(const DATA_TYPE& s) const {
		vec4 r(*this);
		r /= s;
		return r;
	}

	// Vector * Vector
	vec4& operator*=(const vec4& v) {
		data[0] *= v.data[0];
		data[1] *= v.data[1];
		data[2] *= v.data[2];
		data[3] *= v.data[3];
		return *this;
	}

	vec4 operator*(const vec4& s) const {
		vec4 r(*this);
		r *= s;
		return r;
	}

	// Vector / Vector
	vec4& operator/=(const vec4& v) {
		data[0] /= v.data[0];
		data[1] /= v.data[1];
		data[2] /= v.data[2];
		data[3] /= v.data[3];
		return *this;
	}

	vec4 operator/(const vec4& s) const {
		vec4 r(*this);
		r /= s;
		return r;
	}

	/// Performs a component-wise division of two vectors.
	/// Assumes that non of the elements in @b are 0
	friend vec4 cdiv(const vec4& a, const vec4& b) {
		vec4 r;
		r[0] = a[0] / b[0];
		r[1] = a[1] / b[1];
		r[2] = a[2] / b[2];
		r[3] = a[3] / b[3];
		return r;
	}

	/// Returns a sum of all elements in the vector.
	DATA_TYPE hsum() const {
		DATA_TYPE r = data[0];
		r += data[1];
		r += data[2];
		r += data[3];
		return r;
	}

	friend DATA_TYPE hsum(const vec4& v) { return v.hsum(); }

	/// Computes the dot product between two vectors.
	DATA_TYPE dot(const vec4& v) const {
		DATA_TYPE r = data[0] * v[0];
		r += data[1] * v[1];
		r += data[2] * v[2];
		r += data[3] * v[3];

		return r;
	}

	friend DATA_TYPE dot(const vec4& a, const vec4& b) { return a.dot(b); }

	/// Computes the length of the vector.
	DATA_TYPE lengthSqr() const { return dot(*this); }

	DATA_TYPE length() const {
		// [TODO]
		return sqrtf(lengthSqr());
	}

	friend DATA_TYPE length(const vec4& v) { return v.length(); }

	/// Computes the unsigned-volume of a cube.
	DATA_TYPE volume() const {
		DATA_TYPE r = abs(data[0]);
		r *= abs(data[1]);
		r *= abs(data[2]);
		r *= abs(data[3]);

		return r;
	}

	/// Returns the normalized vector (with length 1.f).
	/// Assumes that the vector current size is not 0.
	vec4 normalized() const {
		const DATA_TYPE invLength = DATA_TYPE(1.0) / length();

		vec4 result;
		result[0] = data[0] * invLength;
		result[1] = data[1] * invLength;
		result[2] = data[2] * invLength;
		result[3] = data[3] * invLength;

		return result;
	}

	friend vec4 normalized(const vec4& v) { return v.normalized(); }

	/// Returns the normalized vector (with length 1.f).
	/// If the size of the vector is 0, the zero vector is returned.
	vec4 normalized0() const {
		DATA_TYPE lenSqr = lengthSqr();
		if (lengthSqr() < 1e-6f) {
			return vec4(0);
		}

		const DATA_TYPE invLength = DATA_TYPE(1.0) / sqrt(lenSqr);

		vec4 result;
		result[0] = data[0] * invLength;
		result[1] = data[1] * invLength;
		result[2] = data[2] * invLength;
		result[3] = data[3] * invLength;

		return result;
	}

	friend vec4 normalized0(const vec4& v) { return v.normalized0(); }

	/// Interpolates two vectors with the a speed (defined in units).
	/// Asuumes that @speed is >= 0
	friend vec4 speedLerp(const vec4& a, const vec4& b, const DATA_TYPE speed, const DATA_TYPE epsilon = 1e-6f) {
		vec4 const diff = b - a;
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
	vec4 reflect(const vec4& normal) const { return (*this) + DATA_TYPE(2.0) * dot(normal) * normal; }

	friend DATA_TYPE reflect(const vec4& incident, const vec4& normal) { return incident.reflect(normal); }

	/// Computes the refracted vector based on the normal specified.
	vec4 refract(const vec4& normal, const DATA_TYPE& eta) const {
		const DATA_TYPE one(1.0);
		const DATA_TYPE zero(0.0);

		const DATA_TYPE p = Dot(normal);
		const DATA_TYPE k = one - eta * eta * (one - p * p);

		if (k < zero)
			return vec4(zero);
		else
			return (*this) * eta - (eta * p + sqrt(k)) * normal;
	}

	friend vec4 refract(const vec4& inc, const vec4& n, DATA_TYPE& eta) { return inc.refract(n, eta); }

	/// Computes the distance between two vectors.
	DATA_TYPE distance(const vec4& other) const { return (*this - other).length(); }

	friend DATA_TYPE distance(const vec4& a, const vec4& b) { return a.distance(b); }

	/// Rentusn a vector containing min/max components from each vector.
	vec4 pickMin(const vec4& other) const {
		vec4 result;

		result.data[0] = minOf(data[0], other.data[0]);
		result.data[1] = minOf(data[1], other.data[1]);
		result.data[2] = minOf(data[2], other.data[2]);
		result.data[3] = minOf(data[3], other.data[3]);

		return result;
	}

	friend vec4 pick_min(const vec4& a, const vec4& b) { return a.pickMin(b); }

	vec4 pickMax(const vec4& other) const {
		vec4 result;
		result.data[0] = maxOf(data[0], other.data[0]);
		result.data[1] = maxOf(data[1], other.data[1]);
		result.data[2] = maxOf(data[2], other.data[2]);
		result.data[3] = maxOf(data[3], other.data[3]);

		return result;
	}

	/// Returns the min/max component in the vector.
	DATA_TYPE componentMin() const {
		DATA_TYPE retval = data[0];

		if (data[1] < retval) {
			retval = data[1];
		}

		if (data[2] < retval) {
			retval = data[2];
		}

		if (data[3] < retval) {
			retval = data[3];
		}

		return retval;
	}

	friend vec4 pick_min(const vec4& v) { return v.componentMin(); }

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

		{
			float abs = std::abs(data[3]);
			if (abs < retval) {
				retval = abs;
			}
		}

		return retval;
	}

	DATA_TYPE componentMax() const {
		DATA_TYPE retval = data[0];

		if (data[1] > retval) {
			retval = data[1];
		}

		if (data[2] > retval) {
			retval = data[2];
		}

		if (data[3] > retval) {
			retval = data[3];
		}

		return retval;
	}

	friend vec4 pick_max(const vec4& v) { return v.componentMax(); }

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

		{
			float abs = std::abs(data[3]);
			if (abs > retval) {
				retval = abs;
			}
		}

		return retval;
	}

	vec4 getSorted() const {
		vec4 retval = *this;

		for (int t = 0; t < vec4::NUM_ELEMS; ++t)
			for (int k = t + 1; k < vec4::NUM_ELEMS; ++k) {
				if (retval.data[t] > retval.data[k]) {
					DATA_TYPE x = retval.data[t];
					retval.data[t] = retval.data[k];
					retval.data[k] = x;
				}
			}

		return retval;
	}
};

typedef vec4<int> vec4i;
typedef vec4<unsigned int> vec4u;
typedef vec4<float> vec4f;
typedef vec4<double> vec4d;

} // namespace sge
