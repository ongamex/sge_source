#include "GameDrawer.h"

#include "GameInspector.h"
#include "GameWorld.h"
#include "IWorldScript.h"
#include "sge_core/DebugDraw.h"
#include "sge_core/ICore.h"

namespace sge {

void IGameDrawer::initialize(GameWorld* const world) {
	m_world = world;
}

bool IGameDrawer::drawItem(const GameDrawSets& drawSets, const SelectedItem& item, bool const selectionMode) {
	Actor* actor = m_world->getActorById(item.objectId);
	if (actor == nullptr) {
		return false;
	}

	// Todo: Draw reason
	drawActor(drawSets, item.editMode, actor, item.index, selectionMode ? drawReason_visualizeSelection : drawReason_editing);
	return true;
}

void IGameDrawer::drawWorld(const GameDrawSets& drawSets, const DrawReason drawReason) {
	getWorld()->iterateOverPlayingObjects(
	    [&](GameObject* object) -> bool {
		    // TODO: Skip this check for whole types. We know they are not actors...
		    Actor* actor = object->getActor();
		    if (actor) {
			    drawActor(drawSets, editMode_actors, actor, 0, drawReason);
		    }

		    return true;
	    },
	    false);

	if (getWorld()->inspector && getWorld()->inspector->m_physicsDebugDrawEnabled) {
		drawSets.rdest.sgecon->clearDepth(drawSets.rdest.frameTarget, 1.f);

		getWorld()->m_physicsDebugDraw.preDebugDraw(drawSets.drawCamera->getProjView(), drawSets.quickDraw, drawSets.rdest);
		getWorld()->physicsWorld.dynamicsWorld->debugDrawWorld();
		getWorld()->m_physicsDebugDraw.postDebugDraw();
	}

	if (drawReason == drawReason_gameplay) {
		for (ObjectId scriptObj : getWorld()->m_scriptObjects) {
			if (IWorldScript* script = dynamic_cast<IWorldScript*>(getWorld()->getObjectById(scriptObj))) {
				script->onPostDraw(drawSets);
			}
		}
	}

	getCore()->getDebugDraw().draw(drawSets.rdest, drawSets.drawCamera->getProjView());
}

} // namespace sge
