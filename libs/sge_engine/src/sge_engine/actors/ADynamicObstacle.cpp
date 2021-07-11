#pragma once

#include "ADynamicObstacle.h"
#include "sge_core/ICore.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/RigidBodyFromModel.h"

namespace sge {
//-----------------------------------------------------
// ADynamicObstacle
//-----------------------------------------------------
// clang-format off
DefineTypeId(ADynamicObstacle, 20'03'02'0007);
ReflBlock() {
	ReflAddActor(ADynamicObstacle) 
		ReflMember(ADynamicObstacle, m_traitModel)
	    ReflMember(ADynamicObstacle, m_rbConfig)
	;
}
// clang-format on

void ADynamicObstacle::create() {
	m_traitModel.setModel("assets/editor/models/box.mdl", false);

	registerTrait(m_traitRB);
	registerTrait(m_traitModel);
}

void ADynamicObstacle::onPlayStateChanged(bool const isStartingToPlay) {
	Actor::onPlayStateChanged(isStartingToPlay);

	if (isStartingToPlay) {
		if (m_traitModel.postUpdate()) {
			m_rbConfig.apply(*this, true);
		}
	}
}

void ADynamicObstacle::onDuplocationComplete() {
	m_traitModel.invalidateCachedAssets();
}

void ADynamicObstacle::postUpdate(const GameUpdateSets& UNUSED(u)) {
	if (m_traitModel.postUpdate()) {
		m_rbConfig.apply(*this, true);
	}
}

AABox3f ADynamicObstacle::getBBoxOS() const {
	return m_traitModel.getBBoxOS();
}

} // namespace sge
