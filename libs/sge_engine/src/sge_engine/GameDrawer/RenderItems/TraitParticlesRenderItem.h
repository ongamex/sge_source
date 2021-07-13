#pragma once

#include "sge_engine/GameDrawer/IRenderItem.h"

namespace sge {

struct TraitParticles;

struct TraitParticlesRenderItem : public IRenderItem {
	TraitParticles* traitParticles = nullptr;
};

} // namespace sge
