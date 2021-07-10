#include "DummyPlugin.h"
#include "sge_engine/GameDrawer/DefaultGameDrawer.h"

namespace sge {
IGameDrawer* DummyPlugin::allocateGameDrawer() {
	return new DefaultGameDrawer();
}

} // namespace sge