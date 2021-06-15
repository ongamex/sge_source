#pragma once

#include "common.h"
#include "mat3.h"
#include "quat.h"
#include "vec4.h"

namespace sge {

////////////////////////////////////////////////////////////////////////
// struct mat44 A column major storage matrix type
////////////////////////////////////////////////////////////////////////
template <typename TDATA_TYPE>
struct mat4 {
	typedef TDATA_TYPE DATA_TYPE;
	typedef mat4 SELF_TYPE;

	static const unsigned int NUM_ROW = 4;
	static const unsigned int NUM_COL = 4;

	static const unsigned int VEC_SIZE = NUM_ROW;
	static const unsigned int NUM_VECS = NUM_COL;

	typedef vec4<DATA_TYPE> VEC_TYPE;

	union {
		VEC_TYPE data[NUM_VECS];
		struct {
			VEC_TYPE c0;
			VEC_TYPE c1;
			VEC_TYPE c2;
			VEC_TYPE c3;
		};
	};

#include "matrix_common.inl"

	friend vec4<DATA_TYPE> mat_mul_vec(const SELF_TYPE& m, const vec4<DATA_TYPE>& v) {
		vec4<DATA_TYPE> result;

		result[0] = m.data[0][0] * v[0];
		result[0] += m.data[1][0] * v[1];
		result[0] += m.data[2][0] * v[2];
		result[0] += m.data[3][0] * v[3];

		result[1] = m.data[0][1] * v[0];
		result[1] += m.data[1][1] * v[1];
		result[1] += m.data[2][1] * v[2];
		result[1] += m.data[3][1] * v[3];

		result[2] = m.data[0][2] * v[0];
		result[2] += m.data[1][2] * v[1];
		result[2] += m.data[2][2] * v[2];
		result[2] += m.data[3][2] * v[3];

		result[3] = m.data[0][3] * v[0];
		result[3] += m.data[1][3] * v[1];
		result[3] += m.data[2][3] * v[2];
		result[3] += m.data[3][3] * v[3];

		return result;
	}

	friend vec3<DATA_TYPE> mat_mul_pos(const SELF_TYPE& m, const vec3<DATA_TYPE>& v) {
		vec3<DATA_TYPE> result;

		result[0] = m.data[0][0] * v[0];
		result[0] += m.data[1][0] * v[1];
		result[0] += m.data[2][0] * v[2];
		result[0] += m.data[3][0];

		result[1] = m.data[0][1] * v[0];
		result[1] += m.data[1][1] * v[1];
		result[1] += m.data[2][1] * v[2];
		result[1] += m.data[3][1];

		result[2] = m.data[0][2] * v[0];
		result[2] += m.data[1][2] * v[1];
		result[2] += m.data[2][2] * v[2];
		result[2] += m.data[3][2];

		return result;
	}

	vec3<DATA_TYPE> transfPos(const vec3<DATA_TYPE>& pos) const { return mat_mul_pos(*this, pos); }

	friend vec3<DATA_TYPE> mat_mul_dir(const SELF_TYPE& m, const vec3<DATA_TYPE>& v) {
		vec3<DATA_TYPE> result;

		result[0] = m.data[0][0] * v[0];
		result[0] += m.data[1][0] * v[1];
		result[0] += m.data[2][0] * v[2];

		result[1] = m.data[0][1] * v[0];
		result[1] += m.data[1][1] * v[1];
		result[1] += m.data[2][1] * v[2];

		result[2] = m.data[0][2] * v[0];
		result[2] += m.data[1][2] * v[1];
		result[2] += m.data[2][2] * v[2];

		return result;
	}

	vec3<DATA_TYPE> transfDir(const vec3<DATA_TYPE>& dir) const { return mat_mul_dir(*this, dir); }

	//---------------------------------------------------
	//
	//---------------------------------------------------
	DATA_TYPE determinant() const {
		const DATA_TYPE s[6] = {
		    data[0][0] * data[1][1] - data[1][0] * data[0][1], data[0][0] * data[1][2] - data[1][0] * data[0][2],
		    data[0][0] * data[1][3] - data[1][0] * data[0][3], data[0][1] * data[1][2] - data[1][1] * data[0][2],
		    data[0][1] * data[1][3] - data[1][1] * data[0][3], data[0][2] * data[1][3] - data[1][2] * data[0][3],
		};

		const DATA_TYPE c[6] = {
		    data[2][0] * data[3][1] - data[3][0] * data[2][1], data[2][0] * data[3][2] - data[3][0] * data[2][2],
		    data[2][0] * data[3][3] - data[3][0] * data[2][3], data[2][1] * data[3][2] - data[3][1] * data[2][2],
		    data[2][1] * data[3][3] - data[3][1] * data[2][3], data[2][2] * data[3][3] - data[3][2] * data[2][3],
		};

		const DATA_TYPE det = (DATA_TYPE)(s[0] * c[5] - s[1] * c[4] + s[2] * c[3] + s[3] * c[2] - s[4] * c[1] + s[5] * c[0]);
		return det;
	}

	friend DATA_TYPE determinant(const SELF_TYPE& m) { return m.determinant(); }


	//---------------------------------------------------
	// inverse
	//---------------------------------------------------
	SELF_TYPE inverse() const {
		const DATA_TYPE s[6] = {
		    data[0][0] * data[1][1] - data[1][0] * data[0][1], data[0][0] * data[1][2] - data[1][0] * data[0][2],
		    data[0][0] * data[1][3] - data[1][0] * data[0][3], data[0][1] * data[1][2] - data[1][1] * data[0][2],
		    data[0][1] * data[1][3] - data[1][1] * data[0][3], data[0][2] * data[1][3] - data[1][2] * data[0][3],
		};

		const DATA_TYPE c[6] = {
		    data[2][0] * data[3][1] - data[3][0] * data[2][1], data[2][0] * data[3][2] - data[3][0] * data[2][2],
		    data[2][0] * data[3][3] - data[3][0] * data[2][3], data[2][1] * data[3][2] - data[3][1] * data[2][2],
		    data[2][1] * data[3][3] - data[3][1] * data[2][3], data[2][2] * data[3][3] - data[3][2] * data[2][3],
		};

		const DATA_TYPE invDet = (DATA_TYPE)(1.0) / (s[0] * c[5] - s[1] * c[4] + s[2] * c[3] + s[3] * c[2] - s[4] * c[1] + s[5] * c[0]);

		SELF_TYPE result;

		result.data[0][0] = (data[1][1] * c[5] - data[1][2] * c[4] + data[1][3] * c[3]) * invDet;
		result.data[0][1] = (-data[0][1] * c[5] + data[0][2] * c[4] - data[0][3] * c[3]) * invDet;
		result.data[0][2] = (data[3][1] * s[5] - data[3][2] * s[4] + data[3][3] * s[3]) * invDet;
		result.data[0][3] = (-data[2][1] * s[5] + data[2][2] * s[4] - data[2][3] * s[3]) * invDet;

		result.data[1][0] = (-data[1][0] * c[5] + data[1][2] * c[2] - data[1][3] * c[1]) * invDet;
		result.data[1][1] = (data[0][0] * c[5] - data[0][2] * c[2] + data[0][3] * c[1]) * invDet;
		result.data[1][2] = (-data[3][0] * s[5] + data[3][2] * s[2] - data[3][3] * s[1]) * invDet;
		result.data[1][3] = (data[2][0] * s[5] - data[2][2] * s[2] + data[2][3] * s[1]) * invDet;

		result.data[2][0] = (data[1][0] * c[4] - data[1][1] * c[2] + data[1][3] * c[0]) * invDet;
		result.data[2][1] = (-data[0][0] * c[4] + data[0][1] * c[2] - data[0][3] * c[0]) * invDet;
		result.data[2][2] = (data[3][0] * s[4] - data[3][1] * s[2] + data[3][3] * s[0]) * invDet;
		result.data[2][3] = (-data[2][0] * s[4] + data[2][1] * s[2] - data[2][3] * s[0]) * invDet;

		result.data[3][0] = (-data[1][0] * c[3] + data[1][1] * c[1] - data[1][2] * c[0]) * invDet;
		result.data[3][1] = (data[0][0] * c[3] - data[0][1] * c[1] + data[0][2] * c[0]) * invDet;
		result.data[3][2] = (-data[3][0] * s[3] + data[3][1] * s[1] - data[3][2] * s[0]) * invDet;
		result.data[3][3] = (data[2][0] * s[3] - data[2][1] * s[1] + data[2][2] * s[0]) * invDet;

		return result;
	}

	friend SELF_TYPE inverse(const SELF_TYPE& m) { return m.inverse(); }

	//----------------------------------------------------------------
	//
	//----------------------------------------------------------------
	DATA_TYPE getRC(int row, int column) const { return data[column][row]; }

	//----------------------------------------------------------------
	// Extracting and decompising the matrix
	// Assuming mat*vec multiplication order (column major)
	//----------------------------------------------------------------
	vec3<DATA_TYPE> extractUnsignedScalingVector() const {
		vec3<DATA_TYPE> res;
		res.x = c0.xyz().length();
		res.y = c1.xyz().length();
		res.z = c2.xyz().length();
		return res;
	}

	mat4 removedScaling() const {
		const DATA_TYPE axisXLenSqr = data[0].lengthSqr();
		const DATA_TYPE axisYLenSqr = data[1].lengthSqr();
		const DATA_TYPE axisZLenSqr = data[2].lengthSqr();
		const DATA_TYPE scaleX = DATA_TYPE(1e-6f) >= 0 ? invSqrt(axisXLenSqr) : 1;
		const DATA_TYPE scaleY = DATA_TYPE(1e-6f) >= 0 ? invSqrt(axisYLenSqr) : 1;
		const DATA_TYPE scaleZ = DATA_TYPE(1e-6f) >= 0 ? invSqrt(axisZLenSqr) : 1;

		mat4 result = *this;

		result.data[0][0] *= scaleX;
		result.data[0][1] *= scaleX;
		result.data[0][2] *= scaleX;

		result.data[1][0] *= scaleY;
		result.data[1][1] *= scaleY;
		result.data[1][2] *= scaleY;

		result.data[2][0] *= scaleZ;
		result.data[2][1] *= scaleZ;
		result.data[2][2] *= scaleZ;

		return result;
	}

	vec3<DATA_TYPE> extractTranslation() const { return c3.xyz(); }

	//----------------------------------------------------------------
	// 3d translation
	//----------------------------------------------------------------
	static SELF_TYPE getTranslation(const DATA_TYPE& tx, const DATA_TYPE& ty, const DATA_TYPE& tz) {
		SELF_TYPE result;

		result.data[0] = VEC_TYPE::getAxis(0);
		result.data[1] = VEC_TYPE::getAxis(1);
		result.data[2] = VEC_TYPE::getAxis(2);
		result.data[3] = VEC_TYPE(tx, ty, tz, (DATA_TYPE)1.0);

		return result;
	}

	static SELF_TYPE getTranslation(const vec3<DATA_TYPE>& t) {
		SELF_TYPE result;

		result.data[0] = VEC_TYPE::getAxis(0);
		result.data[1] = VEC_TYPE::getAxis(1);
		result.data[2] = VEC_TYPE::getAxis(2);
		result.data[3] = VEC_TYPE(t.x, t.y, t.z, (DATA_TYPE)1.0);

		return result;
	}

	//----------------------------------------------------------------
	// scaling
	//----------------------------------------------------------------
	static SELF_TYPE getScaling(const DATA_TYPE& s) {
		SELF_TYPE result;

		result.data[0] = VEC_TYPE::getAxis(0, s);
		result.data[1] = VEC_TYPE::getAxis(1, s);
		result.data[2] = VEC_TYPE::getAxis(2, s);
		result.data[3] = VEC_TYPE::getAxis(3);

		return result;
	}

	static SELF_TYPE getScaling(const DATA_TYPE& sx, const DATA_TYPE& sy, const DATA_TYPE& sz) {
		SELF_TYPE result;

		result.data[0] = VEC_TYPE::getAxis(0, sx);
		result.data[1] = VEC_TYPE::getAxis(1, sy);
		result.data[2] = VEC_TYPE::getAxis(2, sz);
		result.data[3] = VEC_TYPE::getAxis(3);

		return result;
	}

	static SELF_TYPE getScaling(const vec3<DATA_TYPE>& s) {
		SELF_TYPE result;

		result.data[0] = VEC_TYPE::getAxis(0, s.data[0]);
		result.data[1] = VEC_TYPE::getAxis(1, s.data[1]);
		result.data[2] = VEC_TYPE::getAxis(2, s.data[2]);
		result.data[3] = VEC_TYPE::getAxis(3);

		return result;
	}

	// Imagine that we are transforming a cube with side size = a
	// Transforming that cube with that matrix with scale X and Z axes equaly, based on the specified Y scaling,
	// so the volume of that cube will remain the same after the scaling.
	static SELF_TYPE getSquashyScalingY(const DATA_TYPE sy, DATA_TYPE sxzScale = DATA_TYPE(1)) {
		const DATA_TYPE sxz = (sy > DATA_TYPE(1e-6f)) ? sqrt(DATA_TYPE(1) / sy) * sxzScale + (1.f - sxzScale) : 0;
		return mat4::getScaling(sxz, sy, sxz);
	}

	//----------------------------------------------------------------
	// X rotation matrix
	//----------------------------------------------------------------
	static SELF_TYPE getRotationX(const DATA_TYPE& angle) {
		SELF_TYPE result;

		DATA_TYPE s, c;
		SinCos(angle, s, c);

		result.data[0] = VEC_TYPE::getAxis(0);
		result.data[1] = VEC_TYPE(0, c, s, 0);
		result.data[2] = VEC_TYPE(0, -s, c, 0);
		result.data[3] = VEC_TYPE::getAxis(3);

		return result;
	}

	//----------------------------------------------------------------
	// Y rotation matrix
	//----------------------------------------------------------------
	static SELF_TYPE getRotationY(const DATA_TYPE& angle) {
		SELF_TYPE result;

		DATA_TYPE s, c;
		SinCos(angle, s, c);

		result.data[0] = VEC_TYPE(c, 0, -s, 0);
		result.data[1] = VEC_TYPE::getAxis(1);
		result.data[2] = VEC_TYPE(s, 0, c, 0);
		result.data[3] = VEC_TYPE::getAxis(3);

		return result;
	}

	//----------------------------------------------------------------
	// Z rotation matrix
	//----------------------------------------------------------------
	static SELF_TYPE getRotationZ(const DATA_TYPE& angle) {
		SELF_TYPE result;

		DATA_TYPE s, c;
		SinCos(angle, s, c);

		result.data[0] = VEC_TYPE(c, s, 0, 0);
		result.data[1] = VEC_TYPE(-s, c, 0, 0);
		result.data[2] = VEC_TYPE::getAxis(2);
		result.data[3] = VEC_TYPE::getAxis(3);

		return result;
	}

	//----------------------------------------------------------------
	// Quaternion rotation
	// http://renderfeather.googlecode.com/hg-history/034a1900d6e8b6c92440382658d2b01fc732c5de/Doc/optimized%20Matrix%20quaternion%20conversion.pdf
	//----------------------------------------------------------------
	static SELF_TYPE getRotationQuat(const quat<DATA_TYPE>& q) {
		SELF_TYPE result;

		const DATA_TYPE& x = q[0];
		const DATA_TYPE& y = q[1];
		const DATA_TYPE& z = q[2];
		const DATA_TYPE& w = q[3];

		const DATA_TYPE one = (DATA_TYPE)1.0;
		const DATA_TYPE two = (DATA_TYPE)2.0;

#if 0
		result.data[0] = VEC_TYPE(1 - 2.0*(y*y + z*z), 2.0*(x*y + w*z), 2.0*(x*z - w*y), 0);
		result.data[1] = VEC_TYPE(2.0*(x*y - w*z), 1 - 2.0*(x*x + z*z), 2.0*(y*z + w*x), 0);
		result.data[2] = VEC_TYPE(2.0*(x*z + w*y), 2.0*(y*z - w*x), 1 - 2.0*(x*x + y*y), 0);
		result.data[3] = VEC_TYPE(0, 0, 0, 1);
#else
		const DATA_TYPE s = 2 / q.length();

		result.data[0] = VEC_TYPE(1 - s * (y * y + z * z), s * (x * y + w * z), s * (x * z - w * y), 0);
		result.data[1] = VEC_TYPE(s * (x * y - w * z), 1 - s * (x * x + z * z), s * (y * z + w * x), 0);
		result.data[2] = VEC_TYPE(s * (x * z + w * y), s * (y * z - w * x), 1 - s * (x * x + y * y), 0);
		result.data[3] = VEC_TYPE(0, 0, 0, 1);
#endif

		return result;
	}

	// Todo: Propery implement this as it is commonly used thing!
	static SELF_TYPE getAxisRotation(const vec3<DATA_TYPE>& axis, const DATA_TYPE angle) {
		return getRotationQuat(quat<DATA_TYPE>::getAxisAngle(axis, angle));
	}

	// [TODO] Optimize this.
	static SELF_TYPE getTRS(const vec3f& translation, const quatf& rotQuat, const vec3f& scaling) {
		return getTranslation(translation) * getRotationQuat(rotQuat) * getScaling(scaling);
	}

	//----------------------------------------------------------------
	// Orthographic projection
	//----------------------------------------------------------------
	static SELF_TYPE getOrthoRH(const float left,
	                            const float right,
	                            const float top,
	                            const float bottom,
	                            const DATA_TYPE& nearZ,
	                            const DATA_TYPE& farZ,
	                            const bool d3dStyle) {
		if (d3dStyle) {
			SELF_TYPE result;
			result.data[0] = VEC_TYPE(2.f / (right - left), 0.f, 0.f, 0.f);
			result.data[1] = VEC_TYPE(0.f, 2.f / (top - bottom), 0.f, 0.f);
			result.data[2] = VEC_TYPE(0.f, 0.f, 1.f / (nearZ - farZ), 0.f);
			result.data[3] = VEC_TYPE((left + right) / (left - right), (top + bottom) / (bottom - top), nearZ / (nearZ - farZ), 1.f);

			return result;
		} else {
			SELF_TYPE result;

			result.data[0] = VEC_TYPE(2.f / (right - left), 0.f, 0.f, 0.f);
			result.data[1] = VEC_TYPE(0.f, 2.f / (top - bottom), 0.f, 0.f);
			result.data[2] = VEC_TYPE(0.f, 0.f, -2.f / (farZ - nearZ), 0.f);
			result.data[3] =
			    VEC_TYPE(-(right + left) / (right - left), -(top + bottom) / (top - bottom), -(farZ + nearZ) / (farZ - nearZ), 1.f);

			return result;
		}
	}

	static SELF_TYPE getOrthoRH(float width, float height, float nearZ, float farZ, const bool d3dStyle) {
		return getOrthoRH(0, width, 0, height, nearZ, farZ, d3dStyle);
	}

	static SELF_TYPE getOrthoRHCentered(float width, float height, float nearZ, float farZ, const bool d3dStyle) {
		width *= 0.5f;
		height *= 0.5f;
		return getOrthoRH(-width, width, height, -height, nearZ, farZ, d3dStyle);
	}

	//----------------------------------------------------------------
	// Perspective projection
	//----------------------------------------------------------------
	static SELF_TYPE getPerspectiveOffCenterRH_DirectX(float l, float r, float b, float t, float zn, float zf) {
		SELF_TYPE result = SELF_TYPE::getZero();

		result.data[0][0] = 2.f * zn / (r - l);

		result.data[1][1] = 2.f * zn / (t - b);

		result.data[2][0] = (l + r) / (r - l);
		result.data[2][1] = (t + b) / (t - b);
		result.data[2][2] = zf / (zn - zf);
		result.data[2][3] = -1.f;

		result.data[3][2] = zn * zf / (zn - zf);

		return result;
	}

	static SELF_TYPE getPerspectiveOffCenterRH_OpenGL(float l, float r, float b, float t, float zn, float zf) {
		SELF_TYPE result = SELF_TYPE::getZero();

		result.data[0][0] = 2.f * zn / (r - l);

		result.data[1][1] = 2.f * zn / (t - b);

		result.data[2][0] = (l + r) / (r - l);
		result.data[2][1] = (t + b) / (t - b);
		result.data[2][2] = -(zf + zn) / (zf - zn);
		result.data[2][3] = -1.f;

		result.data[3][2] = -(2.f * zf * zn) / (zf - zn);

		return result;
	}

	static SELF_TYPE
	    getPerspectiveOffCenterRH(float left, float right, float bottom, float top, float nearZ, float farZ, const bool d3dStyle) {
		if (d3dStyle)
			return getPerspectiveOffCenterRH_DirectX(left, right, bottom, top, nearZ, farZ);
		else
			return getPerspectiveOffCenterRH_OpenGL(left, right, bottom, top, nearZ, farZ);
	}

	static SELF_TYPE getPerspectiveFovRH_DirectX(const DATA_TYPE& verticalFieldOfView,
	                                             const DATA_TYPE& ratioWidthByHeight,
	                                             const DATA_TYPE& nearZ,
	                                             const DATA_TYPE& farZ) {
		float SinFov;
		float CosFov;
		SinCos(0.5f * verticalFieldOfView, SinFov, CosFov);

		float Height = (SinFov * nearZ) / CosFov;
		float Width = ratioWidthByHeight * Height;

		return getPerspectiveOffCenterRH_DirectX(-Width, Width, -Height, Height, nearZ, farZ);
	}

	static SELF_TYPE getPerspectiveFovRH_OpenGL(const DATA_TYPE& verticalFieldOfView,
	                                            const DATA_TYPE& ratioWidthByHeight,
	                                            const DATA_TYPE& nearZ,
	                                            const DATA_TYPE& farZ) {
		float SinFov;
		float CosFov;
		SinCos(0.5f * verticalFieldOfView, SinFov, CosFov);

		float Height = (SinFov * nearZ) / CosFov;
		float Width = ratioWidthByHeight * Height;

		return getPerspectiveOffCenterRH_OpenGL(-Width, Width, -Height, Height, nearZ, farZ);
	}

	static SELF_TYPE getPerspectiveFovRH(
	    const DATA_TYPE& fov, const DATA_TYPE& aspect, const DATA_TYPE& nearZ, const DATA_TYPE& farZ, const bool d3dStyle) {
		if (d3dStyle)
			return getPerspectiveFovRH_DirectX(fov, aspect, nearZ, farZ);
		else
			return getPerspectiveFovRH_OpenGL(fov, aspect, nearZ, farZ);
	}

	//----------------------------------------------------------------
	// look at view d3d right-handed
	//----------------------------------------------------------------
	static SELF_TYPE getLookAtRH(const vec3<DATA_TYPE>& eye, const vec3<DATA_TYPE>& at, const vec3<DATA_TYPE>& up) {
		SELF_TYPE result = SELF_TYPE::getIdentity();

		vec3<DATA_TYPE> f = normalized(at - eye);
		vec3<DATA_TYPE> u = normalized(up);
		vec3<DATA_TYPE> s = normalized(cross(f, u));
		u = normalized(cross(s, f));

		result.at(0, 0) = s.x;
		result.at(0, 1) = s.y;
		result.at(0, 2) = s.z;

		result.at(1, 0) = u.x;
		result.at(1, 1) = u.y;
		result.at(1, 2) = u.z;

		result.at(2, 0) = -f.x;
		result.at(2, 1) = -f.y;
		result.at(2, 2) = -f.z;

		result.at(0, 3) = -dot(s, eye);
		result.at(1, 3) = -dot(u, eye);
		result.at(2, 3) = dot(f, eye);

		return result;
	}

	/// Converts an assumed orthogonal matrix with no scaling to a quaternion.
	quat<DATA_TYPE> toQuat() const {
#if 0
		// See for more details :
		// https://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/

		const bool isZeroRotationMatrix = data[0].lengthSqr() < 1e-6f || data[1].lengthSqr() < 1e-6f || data[2].lengthSqr() < 1e-6f;

		if (isZeroRotationMatrix) {
			return quat<DATA_TYPE>::getIdentity();
		}

		const DATA_TYPE mtxTrace = data[0][0] + data[1][1] + data[2][2];

		if (mtxTrace > 0) {
			quat<DATA_TYPE> result;

			const DATA_TYPE invSqrtScaling = DATA_TYPE(1.0) / sqrt(mtxTrace + DATA_TYPE(1.0));
			const DATA_TYPE s = DATA_TYPE(0.5) * invSqrtScaling;

			result.x = (data[1][2] - data[2][1]) * s;
			result.y = (data[2][0] - data[0][2]) * s;
			result.z = (data[0][1] - data[1][0]) * s;
			result.w = 0.5f * (DATA_TYPE(1.0) / invSqrtScaling);

			return result;
		} else {
			quat<DATA_TYPE> result;

			int i = 0;

			if (data[1][1] > data[0][0]) {
				i = 1;
			}

			if (data[2][2] > data[i][i]) {
				i = 2;
			}

			const int nextAxis[3] = {1, 2, 0};
			const int j = nextAxis[i];
			const int k = nextAxis[j];

			DATA_TYPE s = data[i][i] - data[j][j] - data[k][k] + DATA_TYPE(1.0);

			const DATA_TYPE invSqrtScaling = DATA_TYPE(1.0) / sqrt(s);

			DATA_TYPE qt[4];
			qt[3] = DATA_TYPE(0.5) * (DATA_TYPE(1.0) / invSqrtScaling);

			s = DATA_TYPE(0.5) * invSqrtScaling;

			qt[i] = (data[j][k] - data[k][j]) * s;
			qt[j] = (data[i][j] + data[j][i]) * s;
			qt[k] = (data[i][k] + data[k][i]) * s;

			result.x = qt[0];
			result.y = qt[1];
			result.z = qt[2];
			result.w = qt[3];

			return result;
		}
#else
		quat<DATA_TYPE> result;

		result.w = sqrt(maxOf<DATA_TYPE>(0, 1 + data[0][0] + data[1][1] + data[2][2])) / 2;
		result.x = sqrt(maxOf<DATA_TYPE>(0, 1 + data[0][0] - data[1][1] - data[2][2])) / 2;
		result.y = sqrt(maxOf<DATA_TYPE>(0, 1 - data[0][0] + data[1][1] - data[2][2])) / 2;
		result.z = sqrt(maxOf<DATA_TYPE>(0, 1 - data[0][0] - data[1][1] + data[2][2])) / 2;
		result.x = std::copysign(result.x, data[1][2] - data[2][1]);
		result.y = std::copysign(result.y, data[2][0] - data[0][2]);
		result.z = std::copysign(result.z, data[0][1] - data[1][0]);

		return result;
#endif
	}
};

typedef mat4<float> mat4f;

// vec3* helpers
template <typename T>
vec3<T> rotateTowards(const vec3<T>& from, const vec3<T>& to, const float maxRotationAmount, const vec3<T>& fallbackAxis) {
	T const angle = acos(clamp(from.dot(to) / (from.length() * to.length()), -1.f, 1.f));

	if (angle <= maxRotationAmount) {
		return to;
	}

	vec3<T> c = from.cross(to);
	const T cLengthSqr = c.lengthSqr();
	if (c.lengthSqr() < 1e-5f) {
		c = fallbackAxis;
	} else {
		// Normalize the rotation axis.
		c = c / sqrtf(cLengthSqr);
	}

	mat4<T> const rMtx = mat4<T>::getAxisRotation(c, maxRotationAmount);
	vec3 const result = mat_mul_dir(rMtx, from) * to.length();
	return result;
}

} // namespace sge
