#pragma once

#include "common.h"
#include "math_base.h"
#include <cmath> // isnan

namespace sge {


template <typename TDATA_TYPE>
struct vec2 {
	typedef TDATA_TYPE DATA_TYPE;
	typedef vec2 SELF_TYPE;
	static const unsigned int NUM_ELEMS = 2;

	union {
		struct {
			DATA_TYPE x, y;
		};
		DATA_TYPE data[NUM_ELEMS];
	};

	vec2() = default;

	explicit vec2(const DATA_TYPE& s) {
		for (unsigned int t = 0; t < NUM_ELEMS; ++t) {
			data[t] = s;
		}
	}

	vec2(const DATA_TYPE& _x, const DATA_TYPE& _y)
	    : x(_x)
	    , y(_y) {}

	template <typename T>
	explicit vec2(const vec2<T>& ref) {
		for (int t = 0; t < NUM_ELEMS; ++t) {
			data[t] = DATA_TYPE(ref.data[t]);
		}
	}


	/// Returns only the i-th axis being non-zero with value @axisLen
	static vec2 getAxis(const unsigned int& axisIndex, const DATA_TYPE& axisLen = DATA_TYPE(1)) {
		vec2 result(0);
		result[axisIndex] = axisLen;
		return result;
	}

	static vec2 getY(const DATA_TYPE& axisLen = DATA_TYPE(1)) {
		vec2 result(0);
		result[1] = axisLen;
		return result;
	}

	/// Returns a vector containing only zeroes.
	static vec2 getZero() { return vec2(0); }

	/// Sets the data of the vector form assumingly properly sized c-array.
	void set_data(const DATA_TYPE* const pData) {
		data[0] = pData[0];
		data[1] = pData[1];
	}

	// Less and greater operators.
	friend bool operator<(const vec2& a, const vec2& b) {
		if (a[0] < b[0])
			return true;
		if (a[1] < b[1])
			return true;

		return false;
	}

	friend bool operator<=(const vec2& a, const vec2& b) {
		if (a[0] <= b[0])
			return true;
		if (a[1] <= b[1])
			return true;

		return false;
	}


	friend bool operator>(const vec2& a, const vec2& b) {
		if (a[0] > b[0])
			return true;
		if (a[1] > b[1])
			return true;

		return false;
	}

	friend bool operator>=(const vec2& a, const vec2& b) {
		if (a[0] >= b[0])
			return true;
		if (a[1] >= b[1])
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
	bool operator==(const vec2& v) const { return (data[0] == v[0]) && (data[1] == v[1]); }

	bool operator!=(const vec2& v) const { return !((*this) == v); }

	// Unary operators - +
	vec2 operator-() const {
		vec2 result;

		result[0] = -data[0];
		result[1] = -data[1];

		return result;
	}

	vec2 operator+() const { return *this; }

	// vec + vec
	vec2& operator+=(const vec2& v) {
		data[0] += v[0];
		data[1] += v[1];
		return *this;
	}

	vec2 operator+(const vec2& v) const {
		vec2 r(*this);
		r += v;
		return r;
	}

	// vec - vec
	vec2& operator-=(const vec2& v) {
		data[0] -= v[0];
		data[1] -= v[1];
		return *this;
	}


	vec2 operator-(const vec2& v) const {
		vec2 r(*this);
		r -= v;
		return r;
	}

	// Vector * Scalar (and vice versa)
	vec2& operator*=(const DATA_TYPE& s) {
		data[0] *= s;
		data[1] *= s;
		return *this;
	}

	vec2 operator*(const DATA_TYPE& s) const {
		vec2 r(*this);
		r *= s;
		return r;
	}

	friend vec2 operator*(const DATA_TYPE& s, const vec2& v) { return v * s; }


	// Vector / Scalar
	vec2& operator/=(const DATA_TYPE& s) {
		data[0] /= s;
		data[1] /= s;
		return *this;
	}

	vec2 operator/(const DATA_TYPE& s) const {
		vec2 r(*this);
		r /= s;
		return r;
	}

	// Vector * Vector
	vec2& operator*=(const vec2& v) {
		data[0] *= v.data[0];
		data[1] *= v.data[1];
		return *this;
	}

	vec2 operator*(const vec2& s) const {
		vec2 r(*this);
		r *= s;
		return r;
	}

	// Vector / Vector
	vec2& operator/=(const vec2& v) {
		data[0] /= v.data[0];
		data[1] /= v.data[1];
		return *this;
	}

	vec2 operator/(const vec2& s) const {
		vec2 r(*this);
		r /= s;
		return r;
	}

	/// Performs a component-wise division of two vectors.
	/// Assumes that non of the elements in @b are 0
	friend vec2 cdiv(const vec2& a, const vec2& b) {
		vec2 r;
		r[0] = a[0] / b[0];
		r[1] = a[1] / b[1];
		return r;
	}

	/// Returns a sum of all elements in the vector.
	DATA_TYPE hsum() const {
		DATA_TYPE r = data[0];
		r += data[1];
		return r;
	}

	friend DATA_TYPE hsum(const vec2& v) { return v.hsum(); }

	/// Computes the dot product between two vectors.
	DATA_TYPE dot(const vec2& v) const {
		DATA_TYPE r = data[0] * v[0];
		r += data[1] * v[1];

		return r;
	}

	friend DATA_TYPE dot(const vec2& a, const vec2& b) { return a.dot(b); }

	/// Computes the length of the vector.
	DATA_TYPE lengthSqr() const { return dot(*this); }

	DATA_TYPE length() const {
		// [TODO]
		return sqrtf(lengthSqr());
	}

	friend DATA_TYPE length(const vec2& v) { return v.length(); }

	/// Computes the unsigned-volume of a cube.
	DATA_TYPE volume() const {
		DATA_TYPE r = abs(data[0]);
		r *= abs(data[1]);

		return r;
	}

	/// Returns the normalized vector (with length 1).
	/// Assumes that the vector current size is not 0.
	vec2 normalized() const {
		const DATA_TYPE invLength = DATA_TYPE(1) / length();

		vec2 result;
		result[0] = data[0] * invLength;
		result[1] = data[1] * invLength;

		return result;
	}

	friend vec2 normalized(const vec2& v) { return v.normalized(); }

	/// Returns the normalized vector (with length 1).
	/// If the size of the vector is 0, the zero vector is returned.
	vec2 normalized0() const {
		DATA_TYPE lenSqr = lengthSqr();
		if (lengthSqr() < 1e-6f) {
			return vec2(0);
		}

		const DATA_TYPE invLength = DATA_TYPE(1) / sqrt(lenSqr);

		vec2 result;
		result[0] = data[0] * invLength;
		result[1] = data[1] * invLength;

		return result;
	}

	friend vec2 normalized0(const vec2& v) { return v.normalized0(); }

	/// Interpolates two vectors with the a speed (defined in units).
	/// Asuumes that @speed is >= 0
	friend vec2 speedLerp(const vec2& a, const vec2& b, const DATA_TYPE speed, const DATA_TYPE epsilon = 1e-6f) {
		vec2 const diff = b - a;
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
	vec2 reflect(const vec2& normal) const { return (*this) + DATA_TYPE(2) * dot(normal) * normal; }

	friend DATA_TYPE reflect(const vec2& incident, const vec2& normal) { return incident.reflect(normal); }

	/// Computes the refracted vector based on the normal specified.
	vec2 refract(const vec2& normal, const DATA_TYPE& eta) const {
		const DATA_TYPE one(1);
		const DATA_TYPE zero(0);

		const DATA_TYPE p = Dot(normal);
		const DATA_TYPE k = one - eta * eta * (one - p * p);

		if (k < zero)
			return vec2(zero);
		else
			return (*this) * eta - (eta * p + sqrt(k)) * normal;
	}

	friend vec2 refract(const vec2& inc, const vec2& n, DATA_TYPE& eta) { return inc.refract(n, eta); }

	/// Computes the distance between two vectors.
	DATA_TYPE distance(const vec2& other) const { return (*this - other).length(); }

	friend DATA_TYPE distance(const vec2& a, const vec2& b) { return a.distance(b); }

	/// Rentusn a vector containing min/max components from each vector.
	vec2 pickMin(const vec2& other) const {
		vec2 result;

		result.data[0] = minOf(data[0], other.data[0]);
		result.data[1] = minOf(data[1], other.data[1]);

		return result;
	}

	vec2 pickMax(const vec2& other) const {
		vec2 result;
		result.data[0] = maxOf(data[0], other.data[0]);
		result.data[1] = maxOf(data[1], other.data[1]);

		return result;
	}

	/// Returns the min/max component in the vector.
	DATA_TYPE componentMin() const {
		DATA_TYPE retval = data[0];

		if (data[1] < retval) {
			retval = data[1];
		}

		return retval;
	}

	friend vec2 pick_min(const vec2& v) { return v.componentMin(); }

	DATA_TYPE componentMinAbs() const {
		DATA_TYPE retval = std::abs(data[0]);

		{
			float abs = std::abs(data[1]);
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

		return retval;
	}

	DATA_TYPE componentMaxAbs() const {
		DATA_TYPE retval = std::abs(data[0]);

		{
			float abs = std::abs(data[1]);
			if (abs > retval) {
				retval = abs;
			}
		}

		return retval;
	}

	vec2 getSorted() const {
		vec2 retval = *this;

		for (int t = 0; t < vec2::NUM_ELEMS; ++t)
			for (int k = t + 1; k < vec2::NUM_ELEMS; ++k) {
				if (retval.data[t] > retval.data[k]) {
					DATA_TYPE x = retval.data[t];
					retval.data[t] = retval.data[k];
					retval.data[k] = x;
				}
			}

		return retval;
	}

	////---------------------------------------------------
	////polar to cartesian
	////---------------------------------------------------
	// void polar_to_cartesian(const DATA_TYPE& angle, const DATA_TYPE& radius)
	//{
	//	DATA_TYPE s,c; sgm::sincos(angle, s, c);
	//	data[0] = s * radius;
	//	data[1] = c * radius;
	//}

	// friend SELF_TYPE polar_to_cartesian(const DATA_TYPE& angle, const DATA_TYPE& radius)
	//{
	//	SELF_TYPE result;
	//	result.polar_to_cartesian(angle, radius);
	//	return result;
	//}

	////---------------------------------------------------
	////cartesian to polar
	////---------------------------------------------------
	// void cartesian_to_polar(DATA_TYPE& angle, DATA_TYPE& radius) const
	//{
	//	angle = sgm::atan2(data[1], data[0]);
	//	radius = length();
	//}

	// friend void cartesian_to_polar(const SELF_TYPE& vector, DATA_TYPE& angle, DATA_TYPE& radius)
	//{
	//	vector.cartesian_to_polar(angle, radius);
	//}
};

typedef vec2<int> vec2i;
typedef vec2<unsigned int> vec2u;
typedef vec2<float> vec2f;
typedef vec2<double> vec2d;

} // namespace sge
