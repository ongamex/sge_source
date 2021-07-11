#pragma once

#include "sge_engine/GameDrawer/IRenderItem.h"

namespace sge {

struct TraitViewportIcon;

struct TraitViewportIconRenderItem : public IRenderItem {
	TraitViewportIcon* traitIcon = nullptr;
};

} // namespace sge
