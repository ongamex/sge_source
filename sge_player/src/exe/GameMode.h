#pragma once

#include "sge_engine/GameDrawer/GameDrawer.h"
#include "sge_engine/SceneInstance.h"
#include "sge_utils/utils/timer.h"

namespace sge {

struct GameMode {
	void create(IGameDrawer* gameDrawer, const char* openingLevelPath);
	void update(const InputState& is);
	void draw(const RenderDestination& rdest);

	SceneInstance m_sceneInstance;
	IGameDrawer* m_gameDrawer = nullptr;
	Timer m_timer;
};

} // namespace sge
