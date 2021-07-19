#pragma once

#include "sge_engine/GameDrawer/IRenderItem.h"

namespace sge {

struct TraitParticlesSimple;
struct TraitParticlesProgrammable;

struct TraitParticlesSimpleRenderItem : public IRenderItem {
	TraitParticlesSimple* traitParticles = nullptr;
};

struct TraitParticlesProgrammableRenderItem : public IRenderItem {
	TraitParticlesProgrammable* traitParticles = nullptr;
};

} // namespace sge
