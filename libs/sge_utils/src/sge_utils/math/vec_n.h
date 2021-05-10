#pragma once

#include "common.h"
#include "math_base.h"
#include <cmath> // isnan


namespace sge {

template <typename TDATA_TYPE, unsigned int TNUM_ELEMS>
struct vec_n {
	typedef TDATA_TYPE DATA_TYPE;
	typedef vec_n SELF_TYPE;
	static const unsigned int NUM_ELEMS = TNUM_ELEMS;

	DATA_TYPE data[NUM_ELEMS];

	vec_n() = default;


	/// Returns only the i-th axis being non-zero with value @axisLen
	static vec_n getAxis(const unsigned int& axisIndex, const DATA_TYPE& axisLen = DATA_TYPE(1.0)) {
		vec_n result;
		for (int t = 0; t < NUM_ELEMS; ++t) {
			result[t] = (DATA_TYPE)(0.0);
		}
		result[axisIndex] = axisLen;
		return result;
	}

	static vec_n getY(const DATA_TYPE& axisLen = DATA_TYPE(1.0)) {
		vec_n result;
		for (int t = 0; t < NUM_ELEMS; ++t) {
			result[t] = (DATA_TYPE)(0.0);
		}
		result[1] = axisLen;
		return result;
	}

	/// Returns a vector containing only zeroes.
	static vec_n getZero() {
		vec_n result;
		for (int t = 0; t < NUM_ELEMS; ++t) {
			result[t] = DATA_TYPE(0.0);
		}

		return result;
	}

	/// Sets the data of the vector form assumingly properly sized c-array.
	void set_data(const DATA_TYPE* const pData) {
		for (int t = 0; t < NUM_ELEMS; ++t) {
			data[t] = pData[t];
		}
	}

	// Less and greater operators.
	friend bool operator<(const vec_n& a, const vec_n& b) {
		for (int t = 0; t < NUM_ELEMS; ++t) {
			if (a[t] < b[t])
				return true;
		}

		return false;
	}

	friend bool operator<=(const vec_n& a, const vec_n& b) {
		for (int t = 0; t < NUM_ELEMS; ++t) {
			if (a[t] <= b[t])
				return true;
		}

		return false;
	}


	friend bool operator>(const vec_n& a, const vec_n& b) {
		for (int t = 0; t < NUM_ELEMS; ++t) {
			if (a[t] > b[t])
				return true;
		}

		return false;
	}

	friend bool operator>=(const vec_n& a, const vec_n& b) {
		for (int t = 0; t < NUM_ELEMS; ++t) {
			if (a[t] >= b[t])
				return true;
		}

		return false;
	}

	// Indexing operators.
	DATA_TYPE& operator[](const unsigned int t) { return data[t]; }
	const DATA_TYPE& operator[](const unsigned int t) const { return data[t]; }

	// Operator == and != implemented by direct comparison.
	bool operator==(const vec_n& v) const {
		bool result = true;
		for (unsigned t = 0; t < NUM_ELEMS; ++t) {
			result = result && (data[t] == v[t]);
			// not autovect friendly,
			// if(data[t] != v[t]) return false;
		}

		return result;
	}

	bool operator!=(const vec_n& v) const { return !((*this) == v); }

	// Unary operators - +
	vec_n operator-() const {
		vec_n result;
		for (unsigned int t = 0; t < NUM_ELEMS; ++t) {
			result[t] = -data[t];
		}
		return result;
	}

	vec_n operator+() const { return *this; }

	// vec + vec
	vec_n& operator+=(const vec_n& v) {
		for (unsigned int t = 0; t < NUM_ELEMS; ++t) {
			data[t] += v[t];
		}
		return *this;
	}

	vec_n operator+(const vec_n& v) const {
		vec_n r(*this);
		r += v;
		return r;
	}

	// vec - vec
	vec_n& operator-=(const vec_n& v) {
		for (unsigned int t = 0; t < NUM_ELEMS; ++t) {
			data[t] -= v[t];
		}
		return *this;
	}


	vec_n operator-(const vec_n& v) const {
		vec_n r(*this);
		r -= v;
		return r;
	}

	// Vector * Scalar (and vice versa)
	vec_n& operator*=(const DATA_TYPE& s) {
		for (unsigned int t = 0; t < NUM_ELEMS; ++t) {
			data[t] *= s;
		}
		return *this;
	}

	vec_n operator*(const DATA_TYPE& s) const {
		vec_n r(*this);
		r *= s;
		return r;
	}

	friend vec_n operator*(const DATA_TYPE& s, const vec_n& v) { return v * s; }


	// Vector / Scalar
	vec_n& operator/=(const DATA_TYPE& s) {
		for (unsigned int t = 0; t < NUM_ELEMS; ++t) {
			data[t] /= s;
		}
		return *this;
	}

	vec_n operator/(const DATA_TYPE& s) const {
		vec_n r(*this);
		r /= s;
		return r;
	}

	// Vector * Vector
	vec_n& operator*=(const vec_n& v) {
		for (unsigned int t = 0; t < NUM_ELEMS; ++t) {
			data[t] *= v.data[t];
		}
		return *this;
	}

	vec_n operator*(const vec_n& s) const {
		vec_n r(*this);
		r *= s;
		return r;
	}

	// Vector / Vector
	vec_n& operator/=(const vec_n& v) {
		for (unsigned int t = 0; t < NUM_ELEMS; ++t) {
			data[t] /= v.data[t];
		}
		return *this;
	}

	vec_n operator/(const vec_n& s) const {
		vec_n r(*this);
		r /= s;
		return r;
	}

	/// Performs a component-wise division of two vectors.
	/// Assumes that non of the elements in @b are 0
	friend vec_n cdiv(const vec_n& a, const vec_n& b) {
		vec_n r;
		for (unsigned int t = 0; t < NUM_ELEMS; ++t) {
			r[t] = a[t] / b[t];
		}
		return r;
	}

	/// Returns a sum of all elements in the vector.
	DATA_TYPE hsum() const {
		DATA_TYPE r = data[0];
		for (unsigned int t = 1; t < NUM_ELEMS; ++t) {
			r += data[t];
		}
		return r;
	}

	friend DATA_TYPE hsum(const vec_n& v) { return v.hsum(); }

	/// Computes the dot product between two vectors.
	DATA_TYPE dot(const vec_n& v) const {
		DATA_TYPE r = data[0] * v[0];
		for (unsigned int t = 1; t < NUM_ELEMS; ++t) {
			r += data[t] * v[t];
		}
		return r;
	}

	friend DATA_TYPE dot(const vec_n& a, const vec_n& b) { return a.dot(b); }

	/// Computes the length of the vector.
	DATA_TYPE lengthSqr() const { return dot(*this); }

	DATA_TYPE length() const {
		// [TODO]
		return sqrtf(lengthSqr());
	}

	friend DATA_TYPE length(const vec_n& v) { return v.length(); }

	/// Computes the unsigned-volume of a cube.
	DATA_TYPE volume() const {
		DATA_TYPE r = abs(data[0]);
		for (unsigned int t = 1; t < NUM_ELEMS; ++t) {
			r *= abs(data[t]);
		}

		return r;
	}


	/// Returns the normalized vector (with length 1.f).
	/// Assumes that the vector current size is not 0.
	vec_n normalized() const {
		const DATA_TYPE invLength = DATA_TYPE(1.0) / length();

		vec_n result;
		for (unsigned int t = 0; t < vec_n::NUM_ELEMS; ++t) {
			result[t] = data[t] * invLength;
		}

		return result;
	}

	friend vec_n normalized(const vec_n& v) { return v.normalized(); }

	/// Returns the normalized vector (with length 1.f).
	/// If the size of the vector is 0, the zero vector is returned.
	vec_n normalized0() const {
		if (lengthSqr() < 1e-6f)
			return vec_n(0);

		const DATA_TYPE invLength = DATA_TYPE(1.0) / length();

		vec_n result;
		for (unsigned int t = 0; t < vec_n::NUM_ELEMS; ++t) {
			result[t] = data[t] * invLength;
		}

		return result;
	}

	friend vec_n normalized0(const vec_n& v) { return v.normalized0(); }

	/// Interpolates two vectors with the a speed (defined in units).
	friend vec_n speedLerp(const vec_n& a, const vec_n& b, const DATA_TYPE speed, const DATA_TYPE epsilon = 1e-6f) {
		vec_n const diff = b - a;
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
	vec_n reflect(const vec_n& normal) const { return (*this) + DATA_TYPE(2.0) * dot(normal) * normal; }

	friend DATA_TYPE reflect(const vec_n& incident, const vec_n& normal) { return incident.reflect(normal); }

	/// Computes the refracted vector based on the normal specified. Assuming IOR=1
	vec_n refract(const vec_n& normal, const DATA_TYPE& eta) const {
		const DATA_TYPE one(1.0);
		const DATA_TYPE zero(0.0);

		const DATA_TYPE p = Dot(normal);
		const DATA_TYPE k = one - eta * eta * (one - p * p);

		if (k < zero)
			return vec_n(zero);
		else
			return (*this) * eta - (eta * p + sqrt(k)) * normal;
	}

	friend vec_n refract(const vec_n& inc, const vec_n& n, DATA_TYPE& eta) { return inc.refract(n, eta); }

	/// Computes the distance between two vectors.
	DATA_TYPE distance(const vec_n& other) const { return (*this - other).length(); }

	friend DATA_TYPE distance(const vec_n& a, const vec_n& b) { return a.distance(b); }

	/// Rentusn a vector containing min/max components from each vector.
	vec_n pickMin(const vec_n& other) const {
		vec_n result;

		for (unsigned int t = 0; t < vec_n::NUM_ELEMS; ++t) {
			result.data[t] = minOf(data[t], other.data[t]);
		}

		return result;
	}

	friend vec_n pick_min(const vec_n& a, const vec_n& b) { return a.pickMin(b); }

	vec_n pickMax(const vec_n& other) const {
		vec_n result;

		for (unsigned int t = 0; t < vec_n::NUM_ELEMS; ++t) {
			result.data[t] = maxOf(data[t], other.data[t]);
		}

		return result;
	}

	friend vec_n pick_max(const vec_n& a, const vec_n& b) { return a.pickMax(b); }


	/// Returns the min/max component in the vector.
	DATA_TYPE componentMin() const {
		DATA_TYPE retval = data[0];

		for (int t = 1; t < vec_n::NUM_ELEMS; ++t) {
			if (data[t] < retval) {
				retval = data[t];
			}
		}

		return retval;
	}

	friend vec_n pick_min(const vec_n& v) { return v.componentMin(); }

	DATA_TYPE componentMinAbs() const {
		DATA_TYPE retval = std::abs(data[0]);

		for (int t = 1; t < vec_n::NUM_ELEMS; ++t) {
			float abs = std::abs(data[t]);
			if (abs < retval) {
				retval = abs;
			}
		}

		return retval;
	}

	DATA_TYPE componentMax() const {
		DATA_TYPE retval = data[0];

		for (int t = 1; t < SELF_TYPE::NUM_ELEMS; ++t) {
			if (data[t] > retval) {
				retval = data[t];
			}
		}

		return retval;
	}

	friend SELF_TYPE pick_max(const SELF_TYPE& v) { return v.componentMax(); }

	DATA_TYPE componentMaxAbs() const {
		DATA_TYPE retval = std::abs(data[0]);

		for (int t = 1; t < SELF_TYPE::NUM_ELEMS; ++t) {
			float abs = std::abs(data[t]);
			if (abs > retval) {
				retval = abs;
			}
		}

		return retval;
	}

	SELF_TYPE getSorted() const {
		SELF_TYPE retval = *this;

		for (int t = 0; t < SELF_TYPE::NUM_ELEMS; ++t)
			for (int k = t + 1; k < SELF_TYPE::NUM_ELEMS; ++k) {
				if (retval.data[t] > retval.data[k]) {
					DATA_TYPE x = retval.data[t];
					retval.data[t] = retval.data[k];
					retval.data[k] = x;
				}
			}

		return retval;
	}
};

} // namespace sge
