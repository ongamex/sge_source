#pragma once

#include "sge_engine/GameDrawer/GameDrawer.h"


namespace sge {

struct GeneralDrawMod;

struct IRenderItem {
	IRenderItem() = default;
	virtual ~IRenderItem() = default;

	AABox3f bboxWs; ///< the bounding box of the item being rendered. Use empty if none can be defined, Its needed for alpha sorting.
	bool needsAlphaSorting = false; ///< True if the item needs to be sorted for correct alpha blending.
};

} // namespace sge
