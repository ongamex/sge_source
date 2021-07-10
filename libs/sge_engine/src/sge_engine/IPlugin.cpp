#include "IPlugin.h"
#include "sge_engine/GameDrawer/DefaultGameDrawer.h"

namespace sge {
IGameDrawer* IPlugin::allocateGameDrawer() {
	return new DefaultGameDrawer();
}
} // namespace sge
