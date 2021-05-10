#pragma once

#include "sge_engine_api.h"
#include "sge_utils/math/transform.h"

namespace sge {

/// Describes a textured plane orientation that in object space is facing +X with (bottom,left) point being at (0,0,0) in object space.
enum Billboarding : int {
	billboarding_none,
	billboarding_yOnly,
	billboarding_faceCamera,
};

enum Anchor : int {
	anchor_bottomMid,
	anchor_mid,
	anchor_bottomLeft,
	anchor_topLeft,
};

/// Assuming that the quad faces +X and has corners (0,0,0) and (0,1,1),
/// computes the matrix that is going to align the plane, so the (0,0,0) coordinates
/// related to the anchor position according to the size of the plane.
/// Usually @planeSizeZY is proprtional ot the texture size that is going to be glued on the plane.
mat4f anchor_getPlaneAlignMatrix(const Anchor anchor, const vec2f& planeSizeZY);

/// Computes the transformation of an object, so it faces the camera, based on the
/// @billboarding parameter.
/// @param [in] makeFacingPosZ used only for @billboarding=billboarding_none it makes the defalt facing to be +Z, otherwise it is +X.
/// The object is assumed that the face being rendered faces +X axis.
SGE_ENGINE_API mat4f billboarding_getOrentationMtx(
    const Billboarding billboarding, const transf3d& objectTr, const vec3f& camPos, const mat4f& camViewMtx, const bool makeFacingPosZ);

} // namespace sge
