#pragma once

#include "sge_engine/GameDrawer/GameDrawer.h"

namespace sge {

struct DrawReasonInfo;

struct IRenderItem {
	IRenderItem() = default;
	virtual ~IRenderItem() = default;

	vec3f zSortingPositionWs = vec3f(0.f); ///< The point in world space to be used for zSorting.
	bool needsAlphaSorting = false; ///< True if the item needs to be sorted for correct alpha blending.

	float zSortingValue = 0;
};

} // namespace sge
