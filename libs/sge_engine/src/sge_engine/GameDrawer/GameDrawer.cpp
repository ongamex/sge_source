#include "GameDrawer.h"

#include "sge_core/DebugDraw.h"
#include "sge_core/ICore.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GameInspector.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/IWorldScript.h"

namespace sge {

void IGameDrawer::initialize(GameWorld* const world) {
	m_world = world;
}

} // namespace sge
