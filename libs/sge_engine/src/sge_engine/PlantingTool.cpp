#include "PlantingTool.h"
#include "GameInspector.h"
#include "Physics.h"
#include "sge_core/AssetLibrary.h"
#include "sge_core/ICore.h"
#include "sge_core/application/input.h"
#include "sge_engine/GameDrawer/GameDrawer.h"
#include "sge_engine/PhysicsHelpers.h"
#include "sge_engine/traits/TraitCamera.h"

namespace sge {

struct AInvisibleRigidObstacle;

void PlantingTool::setup(Actor* actorToPlant) {
	if (actorToPlant == nullptr)
		return;

	vector_set<ObjectId> a;
	a.insert(actorToPlant->getId());
	setup(a, *actorToPlant->getWorld());
}

void PlantingTool::setup(const vector_set<ObjectId>& actorsToPlant, GameWorld& world) {
	actorsToPlantOriginalTrasnforms.clear();

	for (ObjectId actorId : actorsToPlant) {
		Actor* actor = world.getActorById(actorId);
		if (actor) {
			actorsToPlantOriginalTrasnforms[actor->getId()] = actor->getTransform();
		}
	}
}

void PlantingTool::onSetActive(GameInspector* const UNUSED(inspector)) {
}

void PlantingTool::onUI(GameInspector* UNUSED(inspector)) {
}

InspectorToolResult PlantingTool::updateTool(GameInspector* const inspector,
                                             bool isAllowedToTakeInput,
                                             const InputState& is,
                                             const GameDrawSets& drawSets) {
	InspectorToolResult result;

	// Compute the picking ray.
	vec2f const viewportSize(drawSets.rdest.viewport.width, drawSets.rdest.viewport.height);
	vec2f const cursorUV = is.GetCursorPos() / viewportSize;
	const Ray pickRay = drawSets.drawCamera->perspectivePickWs(cursorUV);

	if (isAllowedToTakeInput && is.IsKeyPressed(Key::Key_Escape)) {
		inspector->setTool(nullptr);
		result.propagateInput = false;
		return result;
	}

	GameWorld* const world = inspector->getWorld();

	const btVector3 rayPos = toBullet(pickRay.pos);
	const btVector3 rayTar = toBullet(pickRay.pos + pickRay.dir * 1e6f);

	if (actorsToPlantOriginalTrasnforms.empty()) {
		result.propagateInput = false;
		result.isDone = true;
		return result;
	}

	Actor* const primaryActor = inspector->getWorld()->getActorById(actorsToPlantOriginalTrasnforms.begin().key());
	if (primaryActor == nullptr) {
		result.propagateInput = false;
		result.isDone = true;
		return result;
	}

	std::function<bool(const Actor*)> actorFilterFn = [&](const Actor* a) -> bool {
		return actorsToPlantOriginalTrasnforms.find_element(a->getId()) != nullptr || a->getType() == sgeTypeId(AInvisibleRigidObstacle);
	};

	RayResultActor rayResult;
	rayResult.setup(nullptr, rayPos, rayTar, actorFilterFn);
	world->physicsWorld.dynamicsWorld->rayTest(rayPos, rayTar, rayResult);

	if (rayResult.hasHit()) {
		const vec3f hitNormal = normalized0(fromBullet(rayResult.m_hitNormalWorld));

		MemberChain mfc;
		mfc.add(typeLib().findMember(&Actor::m_logicTransform));

		const vec3f rotateAxis = cross(vec3f::getAxis(1), hitNormal);
		const float rotationAngle = asinf(rotateAxis.length());

		transf3d primaryActorNewTransf = actorsToPlantOriginalTrasnforms.begin().value();
		primaryActorNewTransf.p = fromBullet(rayResult.m_hitPointWorld);
		primaryActorNewTransf.r = quatf::getAxisAngle(rotateAxis.normalized0(), rotationAngle);

		const AABox3f bbox = primaryActor->getBBoxOS();
		if (bbox.IsEmpty() == false) {
			if (bbox.min.y < 0.f) {
				primaryActorNewTransf.p -= hitNormal * bbox.min.y;
			}
		}

		// Apply the snapping form the transform tool.
		if (inspector->m_transformTool.m_useSnapSettings) {
			primaryActorNewTransf.p = inspector->m_transformTool.m_snapSettings.applySnappingTranslation(primaryActorNewTransf.p);
		}

		const transf3d diffTrasform = primaryActorNewTransf * actorsToPlantOriginalTrasnforms.begin().value().inverseSimple();

		if (isAllowedToTakeInput && is.IsKeyReleased(Key::Key_MouseLeft)) {
			CmdCompound* cmdAll = new CmdCompound();

			for (auto pair : actorsToPlantOriginalTrasnforms) {
				Actor* actor = world->getActorById(pair.key());
				if (actor) {
					CmdMemberChange* const cmd = new CmdMemberChange();
					transf3d newTransform = diffTrasform * pair.value();
					cmd->setupLogicTransformChange(*actor, pair.value(), newTransform);
					cmdAll->addCommand(cmd);
				}
			}

			inspector->appendCommand(cmdAll, true);

			result.propagateInput = false;
			result.isDone = true;
			return result;
		} else {
			for (auto pair : actorsToPlantOriginalTrasnforms) {
				Actor* actor = world->getActorById(pair.key());
				if (actor) {
					actor->setTransform(diffTrasform * pair.value(), true);
				}
			}
		}
	}

	result.propagateInput = false;
	return result;
}

void PlantingTool::onCancel(GameInspector* UNUSED(inspector)) {
}

void PlantingTool::drawOverlay(const GameDrawSets& UNUSED(drawSets)) {
}

} // namespace sge
