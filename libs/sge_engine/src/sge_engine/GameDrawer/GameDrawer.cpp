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
	struct ActorsNeedingZSort {
		float zSortDistance = 0.f;
		Actor* actor = nullptr;

		bool operator<(const ActorsNeedingZSort& other) const { return zSortDistance < other.zSortDistance; }
	};

	std::vector<ActorsNeedingZSort> m_zSortedActorsToDraw;

	const vec3f zSortingPlanePosWs = drawSets.drawCamera->getCameraPosition();
	const vec3f zSortingPlaneNormal = drawSets.drawCamera->getCameraLookDir();
	const Plane zSortPlane = Plane::FromPosAndDir(zSortingPlanePosWs, zSortingPlaneNormal);

	getWorld()->iterateOverPlayingObjects(
	    [&](GameObject* object) -> bool {
		    // TODO: Skip this check for whole types. We know they are not actors...
		    if (Actor* actor = object->getActor()) {
			    bool shouldDrawTheActorNow = true;
			    if (actor->m_forceAlphaZSort) {
				    AABox3f actorBboxOS = actor->getBBoxOS();
				    if (actorBboxOS.IsEmpty() == false) {
					    vec3f actorBBoxCenterWs = mat_mul_pos(actor->getTransformMtx(), actorBboxOS.center());

					    ActorsNeedingZSort sortingArgs;
					    sortingArgs.actor = actor;
					    sortingArgs.zSortDistance = zSortPlane.Distance(actorBBoxCenterWs);

					    m_zSortedActorsToDraw.push_back(sortingArgs);
					    shouldDrawTheActorNow = false;
				    }
			    }

			    if (shouldDrawTheActorNow) {
				    drawActor(drawSets, editMode_actors, actor, 0, drawReason);
			    }
		    }

		    return true;
	    },
	    false);
	
	std::sort(m_zSortedActorsToDraw.rbegin(), m_zSortedActorsToDraw.rend());
	for (ActorsNeedingZSort& actorZSort : m_zSortedActorsToDraw) {
		drawActor(drawSets, editMode_actors, actorZSort.actor, 0, drawReason);
	}

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
