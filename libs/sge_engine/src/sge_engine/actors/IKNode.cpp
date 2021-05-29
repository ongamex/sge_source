#include "IKNode.h"
#include "sge_core/DebugDraw.h"
#include "sge_core/ICore.h"
#include "sge_core/SGEImGui.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/windows/PropertyEditorWindow.h"
#include "sge_utils/math/FABRIKSolver.h"
#include "sge_utils/stl_algorithm_ex.h"

namespace sge {

// clang-format off
DefineTypeId(AIKNode, 21'18'11'0001);
ReflBlock() {
	ReflAddActor(AIKNode)
		ReflMember(AIKNode, isEnabled)
		ReflMember(AIKNode, maxIterations).uiRange(1, 20, 1.f)
		ReflMember(AIKNode, earlyExitDelta).uiRange(0.f, 0.1f, 0.001f)
		ReflMember(AIKNode, targetControlsOrientation)
		ReflMember(AIKNode, startId).setPrettyName("Start Bone")
		ReflMember(AIKNode, endId).setPrettyName("End Bone")
		ReflMember(AIKNode, targetId).setPrettyName("End Affector (Target)")
		ReflMember(AIKNode, poleId).setPrettyName("Pole")
		ReflMember(AIKNode, useNonInstantFollow).setPrettyName("Non-Instant Follow")
		ReflMember(AIKNode, nonInstantFollowSpeed).setPrettyName("Follow Speed").uiRange(0.1f, 100000.f, 0.1f)
	;
}
// clang-format on

AABox3f AIKNode::getBBoxOS() const {
	return kBBoxObjSpace;
}

void AIKNode::create() {
	registerTrait(m_traitViewportIcon);
	registerTrait(*(IActorCustomAttributeEditorTrait*)this);

	m_traitViewportIcon.setTexture("assets/editor/textures/icons/obj/AIKNode.png", true);
	m_traitViewportIcon.setObjectSpaceOffset(getBBoxOS().halfDiagonal());
}

void AIKNode::doAttributeEditor(GameInspector* inspector) {
	MemberChain chain;

	chain.add(sgeFindMember(AIKNode, isEnabled));
	ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
	chain.pop();

	ImGuiEx::BeginGroupPanel("Solver Accuracity", ImVec2(-1.f, -1.f));

	chain.add(sgeFindMember(AIKNode, maxIterations));
	ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
	chain.pop();

	chain.add(sgeFindMember(AIKNode, earlyExitDelta));
	ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
	chain.pop();

	ImGuiEx::EndGroupPanel();

	ImGuiEx::BeginGroupPanel("Affected Bones", ImVec2(-1.f, -1.f));
	chain.add(sgeFindMember(AIKNode, startId));
	ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
	chain.pop();

	chain.add(sgeFindMember(AIKNode, endId));
	ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
	chain.pop();
	ImGuiEx::EndGroupPanel();

	ImGuiEx::BeginGroupPanel("End Affector and Pole", ImVec2(-1.f, -1.f));
	chain.add(sgeFindMember(AIKNode, targetId));
	ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
	chain.pop();

	chain.add(sgeFindMember(AIKNode, targetControlsOrientation));
	ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
	chain.pop();

	chain.add(sgeFindMember(AIKNode, poleId));
	ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
	chain.pop();
	ImGuiEx::EndGroupPanel();

	ImGuiEx::BeginGroupPanel("Non-Instant Follow", ImVec2(-1.f, -1.f));
	chain.add(sgeFindMember(AIKNode, useNonInstantFollow));
	ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
	chain.pop();

	chain.add(sgeFindMember(AIKNode, nonInstantFollowSpeed));
	ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
	chain.pop();
	ImGuiEx::EndGroupPanel();

	chain.add(sgeFindMember(AIKNode, m_displayName));
	ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
	chain.pop();

	chain.add(sgeFindMember(AIKNode, m_logicTransform));
	ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
	chain.pop();

	if (ImGui::Button("Bind")) {
		bind(startId, endId);
	}
}

void AIKNode::bind(ObjectId start, ObjectId end) {
	// Clear all previous state.
	unbind();

	startId = start;
	endId = end;

	GameWorld* world = GameObject::getWorld();

	// Find the chain of bones that is going to be used to solve the IK.

	bool isChainCorrect = false;
	ObjectId current = endId;
	push_front(ikChainActorIds, current);
	while (true) {
		ObjectId parent = world->getParentId(current);
		if (parent.isNull()) {
			sgeAssertFalse("End is not in the hierarchy under end!");
			break;
		}

		push_front(ikChainActorIds, parent);

		if (parent == start) {
			isChainCorrect = true;
			break;
		}

		current = parent;
	}

	if (isChainCorrect == false) {
		unbind();
		return;
	}

	if (getChainActors(tempChainActors) == false) {
		unbind();
		return;
	}

	if (tempChainActors.size() < 2) {
		unbind();
		return;
	}

	chainLengthWs = 0.f;
	for (int t = 1; t < tempChainActors.size(); ++t) {
		float linkLen = distance(tempChainActors[t]->getPosition(), tempChainActors[t - 1]->getPosition());
		linksLengthWs.push_back(linkLen);
		chainLengthWs += linkLen;
	}

	Actor* targetActor = GameObject::getWorld()->getActorById(targetId);
	if (targetControlsOrientation && targetActor) {
		targetMatchOrientationDiff = tempChainActors.back()->getOrientation() * targetActor->getOrientation().inverse();
	}
}

bool AIKNode::getChainActors(std::vector<Actor*>& result) {
	result.clear();

	bool doesAllExist = true;
	for (ObjectId oid : ikChainActorIds) {
		Actor* a = GameObject::getWorld()->getActorById(oid);
		doesAllExist = doesAllExist && a != nullptr;
		result.push_back(a);
	}

	return doesAllExist;
}

void AIKNode::update(const GameUpdateSets& u) {
	if (isEnabled == false || ikChainActorIds.size() < 2 || chainLengthWs < 1e-6f) {
		return;
	}

	Actor* aTarget = GameObject::getWorld()->getActorById(targetId);
	if (aTarget == nullptr) {
		return;
	}

	Actor* aPole = GameObject::getWorld()->getActorById(poleId);

	if (getChainActors(tempChainActors) == false) {
		return;
	}

	const quatf originalEndOrientationWs = tempChainActors.back()->getOrientation();

	solverPositionWs.clear();
	for (Actor* actor : tempChainActors) {
		solverPositionWs.push_back(actor->getPosition());
	}

	const vec3f startPosWs = solverPositionWs[0];
	vec3f targetPosWs = aTarget->getPosition(); // usually called end affector.

	if (useNonInstantFollow) {
		vec3f current = tempChainActors.back()->getPosition();
		targetPosWs = speedLerp(current, targetPosWs, nonInstantFollowSpeed * u.dt);
	}

	FABRIKSolver(int(solverPositionWs.size()), solverPositionWs.data(), linksLengthWs.data(), targetPosWs,
	             aPole ? &aPole->getPosition() : nullptr, maxIterations, earlyExitDelta);

	// Move the bones.
	for (int iBone = 1; iBone < tempChainActors.size(); ++iBone) {
		Actor* child = tempChainActors[iBone];
		Actor* parent = GameObject::getWorld()->getParentActor(child->getId());

		vec3f currentDiff = (child->getPosition() - parent->getTransform().p).normalized0();
		vec3f targetDiff = solverPositionWs[iBone] - solverPositionWs[iBone - 1];

		quatf q = quatf::fromNormalizedVectors(currentDiff, targetDiff.normalized0(), currentDiff.anyPerpendicular());

		transf3d t = parent->getTransform();
		t.r = q * t.r;
		parent->setTransformEx(t, true, true, true);
	}

	if (targetControlsOrientation) {
		tempChainActors.back()->setOrientation(targetMatchOrientationDiff * aTarget->getOrientation());
	} else {
		tempChainActors.back()->setOrientation(originalEndOrientationWs);
	}

	//
	auto& g = getCore()->getDebugDraw().getGroup("IKDebug");
	g.clear(false);
	for (vec3f p : solverPositionWs) {
		g.getWiered().sphere(mat4f::getTranslation(p), 0xFF00FFFF, 0.1f, 6);
	}
}

} // namespace sge
