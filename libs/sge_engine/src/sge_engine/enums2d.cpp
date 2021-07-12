#include "enums2d.h"
#include "sge_engine/TypeRegister.h"

namespace sge {

// clang-format off
DefineTypeId(Anchor, 20'03'01'0003);
DefineTypeId(Billboarding, 20'03'01'0002);

ReflBlock() {
	ReflAddType(Billboarding)
		ReflEnumVal(billboarding_none, "None")
		ReflEnumVal(billboarding_yOnly, "UpOnly")
		ReflEnumVal(billboarding_faceCamera, "FacingCamera");

	ReflAddType(Anchor)
		ReflEnumVal(anchor_bottomLeft, "BottomLeft")
		ReflEnumVal(anchor_bottomMid, "BottomMid")
		ReflEnumVal(anchor_mid, "Middle")
		ReflEnumVal(anchor_topLeft, "TopLeft");
}
// clang-format on


mat4f anchor_getPlaneAlignMatrix(const Anchor anchor, const vec2f& planeSizeZY) {
	mat4f orientationMtx = mat4f::getScaling(1.f, planeSizeZY.y, planeSizeZY.x);

	switch (anchor) {
		case anchor_bottomLeft: {
			return mat4f::getTranslation(0.f, -0.f, -planeSizeZY.x) * orientationMtx;
		};
		case anchor_bottomMid: {
			return mat4f::getTranslation(0.f, 0.f, -planeSizeZY.x * 0.5f) * orientationMtx;
		};
		case anchor_mid: {
			return mat4f::getTranslation(0.f, -planeSizeZY.y * 0.5f, -planeSizeZY.x * 0.5f) * orientationMtx;
		}
		case anchor_topLeft: {
			return mat4f::getTranslation(0.f, -planeSizeZY.y, -planeSizeZY.x) * orientationMtx;
		};
		default: {
			sgeAssert(false && "Unknown Anchor type");
			return orientationMtx;
		}
	}
}

mat4f billboarding_getOrentationMtx(
    const Billboarding billboarding, const transf3d& objectTr, const vec3f& camPos, const mat4f& camViewMtx, const bool makeFacingPosZ) {
	switch (billboarding) {
		case billboarding_none: {
			mat4f result = objectTr.toMatrix();

			if (makeFacingPosZ) {
				result = result * mat4f::getRotationY(deg2rad(-90.f));
			}
			return result;
		} break;
		case billboarding_yOnly: {
			vec3f diff = objectTr.p - camPos;
			float angle = atan2(-diff.z, diff.x) + sgePi;
			transf3d trNoRotation = objectTr;
			trNoRotation.r = quatf::getIdentity();

			mat4f result = trNoRotation.toMatrix() * mat4f::getRotationY(angle);
			return result;
		} break;
		case billboarding_faceCamera: {
			transf3d trNoRotation = objectTr;
			trNoRotation.r = quatf::getIdentity();

			mat4f faceCameraMtx = camViewMtx;
			faceCameraMtx.c3 = vec4f(0.f, 0.f, 0.f, 1.f);                                 // kill the translation.
			faceCameraMtx = inverse(faceCameraMtx) * mat4f::getRotationY(-deg2rad(90.f)); // TODO: Optimize.

			mat4f result = trNoRotation.toMatrix() * faceCameraMtx;
			return result;
		} break;
		default: {
			sgeAssert(false && "Unknown Billboarding type");
			return objectTr.toMatrix();
		}
	}
}

} // namespace sge
