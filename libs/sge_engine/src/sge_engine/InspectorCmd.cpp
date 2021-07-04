#include <functional>

#include "GameInspector.h"
#include "GameSerialization.h"
#include "InspectorCmd.h"
#include "sge_core/ICore.h"
#include "sge_engine/DynamicProperties.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_utils/utils/strings.h"

namespace sge {

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void CmdMemberChange::setActorLogicTransform(CmdMemberChange* cmd, GameInspector* inspector, void* UNUSED(dest), void* src) {
	Actor* const actor = inspector->m_world->getActorById(cmd->m_objectId);

	sgeAssert(actor);
	if (actor) {
		actor->setTransformEx(*(transf3d*)(src), true, true, true);
	}
}

void CmdMemberChange::setActorLocalTransform(CmdMemberChange* cmd, GameInspector* inspector, void* UNUSED(dest), void* src) {
	Actor* const actor = inspector->m_world->getActorById(cmd->m_objectId);

	sgeAssert(actor);
	if (actor) {
		actor->setLocalTransform(*(transf3d*)(src), true);
	}
}

CmdMemberChange::~CmdMemberChange() {
	const TypeDesc* const typeDesc = m_chainToDynamicProp.getType();

	if (typeDesc) {
		if (m_orginaldata) {
			typeDesc->destructorFn(m_orginaldata.get());
		}

		if (m_newData) {
			typeDesc->destructorFn(m_newData.get());
		}
	}

	m_orginaldata.reset();
	m_newData.reset();
}

void CmdMemberChange::setup(ObjectId const objId,
                            const MemberChain& chain,
                            const void* originalValue,
                            const void* newValue,
                            void (*customCopyFn)(CmdMemberChange* cmd, GameInspector* inspector, void* dest, void* src)) {
	sgeAssert(!objId.isNull() && originalValue && newValue);

	m_objectId = objId;
	m_chainToDynamicProp = chain;
	m_customCopyFn = customCopyFn;

	// Todo: Error checking.
	const TypeDesc* const typeDesc = chain.getType();

	m_orginaldata.reset(new char[typeDesc->sizeBytes]);
	m_newData.reset(new char[typeDesc->sizeBytes]);

	typeDesc->constructorFn(m_orginaldata.get());
	typeDesc->copyFn(m_orginaldata.get(), originalValue);

	typeDesc->constructorFn(m_newData.get());
	typeDesc->copyFn(m_newData.get(), newValue);
}

void CmdMemberChange::apply(GameInspector* inspector) {
	GameObject* const actor = inspector->m_world->getObjectById(m_objectId);

	if (!actor) {
		sgeAssert(false);
		return;
	}

	// Copy the new data.
	void* dest = m_chainToDynamicProp.follow(actor);
	const TypeDesc* const typeDesc = m_chainToDynamicProp.getType();

	if (m_customCopyFn)
		m_customCopyFn(this, inspector, dest, m_newData.get());
	else
		typeDesc->copyFn(dest, m_newData.get());

	actor->onMemberChanged();

	// HACK: When we've got a node selected with the transform tool and move it a few time,
	// if we undo while selected the gizmo with override the transform.
	// We know that the gizmo watches the m_selectionChangeIdx so we are going to chage it in order
	// to force it to "reset"
	inspector->m_selectionChangeIdx++;
}

void CmdMemberChange::undo(GameInspector* inspector) {
	GameObject* const actor = inspector->m_world->getObjectById(m_objectId);

	if (!actor) {
		sgeAssert(false);
		return;
	}

	// Copy the old data.
	void* dest = m_chainToDynamicProp.follow(actor);
	const TypeDesc* const typeDesc = m_chainToDynamicProp.getType();

	if (m_customCopyFn)
		m_customCopyFn(this, inspector, dest, m_orginaldata.get());
	else
		typeDesc->copyFn(dest, m_orginaldata.get());

	actor->onMemberChanged();

	// HACK: When we've got a node selected with the transform tool and move it a few time,
	// if we undo while selected the gizmo with override the transform.
	// We know that the gizmo watches the m_selectionChangeIdx so we are going to chage it in order
	// to force it to "reset"
	inspector->m_selectionChangeIdx++;
}

void CmdMemberChange::getText(std::string& text) {
	text = "GameWorldCommandChangeMember";
}

//--------------------------------------------------------------------
// CmdDynamicProperyChanged
//--------------------------------------------------------------------
CmdDynamicProperyChanged::~CmdDynamicProperyChanged() {
	const TypeDesc* const typeDesc = typeLib().find(m_propertyTypeId);
	if (typeDesc) {
		if (m_orginaldata) {
			typeDesc->destructorFn(m_orginaldata.get());
		}

		if (m_newData) {
			typeDesc->destructorFn(m_newData.get());
		}
	}

	m_orginaldata.reset();
	m_newData.reset();
}

void CmdDynamicProperyChanged::setup(
    GameObject* obj, const MemberChain& chain, std::string dynamicPropName, const void* originalValue, const void* newValue) {
	sgeAssert(obj != nullptr && originalValue && newValue);

	m_objectId = obj->getId();
	m_chainToDynamicProp = chain;
	m_dynamicPropName = std::move(dynamicPropName);

	DynamicProperties& dynamicProps = *(DynamicProperties*)m_chainToDynamicProp.follow(obj);
	DynamicProperties::Property* pProperty = dynamicProps.find(m_dynamicPropName);

	if (pProperty == nullptr) {
		sgeAssert(false);
		return;
	}

	// Todo: Error checking.
	const TypeDesc* const typeDesc = typeLib().find(pProperty->type);
	if (typeDesc == nullptr) {
		sgeAssert(false);
		return;
	}

	m_propertyTypeId = typeDesc->typeId;

	m_orginaldata.reset(new char[typeDesc->sizeBytes]);
	m_newData.reset(new char[typeDesc->sizeBytes]);

	typeDesc->constructorFn(m_orginaldata.get());
	typeDesc->copyFn(m_orginaldata.get(), originalValue);

	typeDesc->constructorFn(m_newData.get());
	typeDesc->copyFn(m_newData.get(), newValue);
}

void CmdDynamicProperyChanged::apply(GameInspector* inspector) {
	if (m_propertyTypeId.isNull()) {
		sgeAssert(false);
		return;
	}

	GameObject* const object = inspector->m_world->getObjectById(m_objectId);

	if (!object) {
		sgeAssert(false);
		return;
	}

	// Copy the new data.
	DynamicProperties& dynamicProps = *(DynamicProperties*)m_chainToDynamicProp.follow(object);

	DynamicProperties::Property* pProperty = dynamicProps.find(m_dynamicPropName);

	if (pProperty == nullptr || pProperty->type != m_propertyTypeId) {
		sgeAssert(false);
		return;
	}

	void* dest = pProperty->propData.get();
	const TypeDesc* const typeDesc = typeLib().find(m_propertyTypeId);
	if (typeDesc == nullptr) {
		sgeAssert(false);
		return;
	}

	typeDesc->copyFn(dest, m_newData.get());

	object->onMemberChanged();

	// HACK: When we've got a node selected with the transform tool and move it a few time,
	// if we undo while selected the gizmo with override the transform.
	// We know that the gizmo watches the m_selectionChangeIdx so we are going to chage it in order
	// to force it to "reset"
	inspector->m_selectionChangeIdx++;
}

void CmdDynamicProperyChanged::undo(GameInspector* inspector) {
	if (m_propertyTypeId.isNull()) {
		sgeAssert(false);
		return;
	}

	GameObject* const object = inspector->m_world->getObjectById(m_objectId);

	if (!object) {
		sgeAssert(false);
		return;
	}

	// Copy the new data.
	DynamicProperties& dynamicProps = *(DynamicProperties*)m_chainToDynamicProp.follow(object);

	DynamicProperties::Property* pProperty = dynamicProps.find(m_dynamicPropName);

	if (pProperty == nullptr || pProperty->type != m_propertyTypeId) {
		sgeAssert(false);
		return;
	}

	void* dest = pProperty->propData.get();
	const TypeDesc* const typeDesc = typeLib().find(m_propertyTypeId);
	if (typeDesc == nullptr) {
		sgeAssert(false);
		return;
	}

	typeDesc->copyFn(dest, m_orginaldata.get());

	object->onMemberChanged();

	// HACK: When we've got a node selected with the transform tool and move it a few time,
	// if we undo while selected the gizmo with override the transform.
	// We know that the gizmo watches the m_selectionChangeIdx so we are going to chage it in order
	// to force it to "reset"
	inspector->m_selectionChangeIdx++;
}

void CmdDynamicProperyChanged::getText(std::string& text) {
	text = "CmdDynamicProperyChanged";
}

//--------------------------------------------------------------------
// Duplicate a game actor.
//
// Caution:
// This does not call "destObject->onDuplocationComplete()"
// as the caller might want to do some additional processing.
//
// To complate the process of duplication the caller must call
// onDuplocationComplete() on the reated object!
//--------------------------------------------------------------------
GameObject* duplicateGameObjectIncompleate(const GameObject* const srcObject) {
	if (!srcObject) {
		sgeAssert(false);
		return nullptr;
	}

	GameWorld* const world = srcObject->getWorldMutable();

	// Generate the name of the new game object.
	std::string displayName;
	{
		// Check if the name is in format "Something_<numbers>", if so
		// take the "Something" part and use it as a base, otherwise append "_<number>".
		std::string baseName = srcObject->getDisplayName();
		size_t lastUnderscore = baseName.find_last_of('_');
		if (lastUnderscore != std::string::npos) {
			bool allDigits = false;
			for (int t = int(lastUnderscore) + 1; t < int(baseName.size()); ++t) {
				allDigits |= !!isdigit(baseName[t]);
				if (allDigits == false)
					break;
			}

			if (allDigits)
				baseName = baseName.substr(0, lastUnderscore);
		}

		// Check if the name is already taken.
		for (int safeCnt = 0; safeCnt < 1000; ++safeCnt) {
			displayName = baseName;
			displayName += string_format("_%d", world->getNextNameIndex());

			if (world->getObjectByName(displayName.c_str()) != nullptr) {
				displayName.clear();
			} else {
				break;
			}
		}

		// Fall back to something.
		if (displayName.empty()) {
			displayName = displayName + "_cpy";
		}
	}

	const TypeDesc* const objTypeDesc = typeLib().find(srcObject->getType());

	GameObject* const destObject = world->allocObject(srcObject->getType(), ObjectId(), displayName.c_str());
	if (!objTypeDesc || !destObject) {
		sgeAssert(false);
		return nullptr;
	}

	// Copy the members.
	for (const MemberDesc& mfd : objTypeDesc->members) {
		char* const destMemberBytes = (char*)destObject + mfd.byteOffset;
		const char* const srcMemberBytes = (const char*)srcObject + mfd.byteOffset;

		if ((mfd.flags & MFF_NonEditable) == 0) {
			const TypeDesc* const memberTypeDesc = typeLib().find(mfd.typeId);

			const bool isActorTransform = mfd.is(&Actor::m_logicTransform);
			const bool isDisplayName = mfd.is(&Actor::m_displayName);

			// Handle the special cases first.
			if (isActorTransform) {
				Actor* destActor = dynamic_cast<Actor*>(destObject);
				if (destActor) {
					destActor->setTransform(*(transf3d*)(srcMemberBytes));
				}
			} else if (isDisplayName) {
				// Do nothing here...
			} else if (memberTypeDesc && memberTypeDesc->copyFn) {
				// Usual member.
				memberTypeDesc->copyFn(destMemberBytes, srcMemberBytes);
			} else {
				SGE_DEBUG_ERR("Cannot duplicate game object! Field %s::%s is not copyable!\n", objTypeDesc->name, mfd.name);
				sgeAssert(false);
			}
		}
	}

	return destObject;
}

//--------------------------------------------------------------------
// CmdCompound
//--------------------------------------------------------------------
void CmdCompound::addCommand(InspectorCmd* const cmd) {
	sgeAssert(cmd);
	if (cmd) {
		cmds.emplace_back(cmd);
	}
}

void CmdCompound::apply(GameInspector* inspector) {
	for (int t = 0; t < cmds.size(); ++t) {
		if_checked(cmds[t]) { cmds[t]->apply(inspector); }
	}
}

void CmdCompound::redo(GameInspector* inspector) {
	for (int t = 0; t < cmds.size(); ++t) {
		if_checked(cmds[t]) { cmds[t]->redo(inspector); }
	}
}

void CmdCompound::undo(GameInspector* inspector) {
	for (int t = int(cmds.size()) - 1; t >= 0; --t) {
		if_checked(cmds[t]) { cmds[t]->undo(inspector); }
	}
}

//--------------------------------------------------------------------
// CmdActorGrouping
//--------------------------------------------------------------------
void CmdActorGrouping::setup(GameWorld& world, ObjectId parent, std::set<ObjectId> objectsToParentUnder) {
	m_parentActorId = parent;

	// Gather the list of all objects that are going to be parented.
	// As a special case if an object and its parent are selected,
	// do not-reparent the child in order to maintain that hierarchy.
	for (ObjectId potentialNewChildId : objectsToParentUnder) {
		if (world.getActorById(potentialNewChildId) != nullptr) {
			ObjectId previousParent = world.getParentId(potentialNewChildId);
			if (objectsToParentUnder.count(previousParent) == 0) {
				m_newChildrenAndTheirOldParents[potentialNewChildId] = previousParent;
			}
		} else {
			sgeAssert(false && "Found an unexisting object while grouping!");
		}
	}
}

void CmdActorGrouping::apply(GameInspector* inspector) {
	GameWorld& world = *inspector->getWorld();

	for (auto pair : m_newChildrenAndTheirOldParents) {
		// HACK(kind of): checking for circular references isn't implemented yet
		// so for now we just unparent everything.
		world.setParentOf(pair.first, ObjectId());
		world.setParentOf(pair.first, m_parentActorId);
	}
}

void CmdActorGrouping::redo(GameInspector* inspector) {
	apply(inspector);
}

void CmdActorGrouping::undo(GameInspector* inspector) {
	GameWorld& world = *inspector->getWorld();

	for (auto pair : m_newChildrenAndTheirOldParents) {
		// HACK(kind of): checking for circular references isn't implemented yet
		// so for now we just unparent everything.
		world.setParentOf(pair.first, ObjectId());
		world.setParentOf(pair.first, pair.second);
	}
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void CmdObjectDeletion::setupDeletion(GameWorld& world, vector_set<ObjectId> objectIdsToBeDeleted) {
	m_deletedObjectIds = std::move(objectIdsToBeDeleted);

	// Store the original hierarchy, we will need it for undo
	for (ObjectId objIdToDel : m_deletedObjectIds) {
		ParentAndChilds& p = m_originalHierarchy[objIdToDel];
		p.parent = world.getParentId(objIdToDel);
		p.children = world.getChildensOfAsList(objIdToDel);
	}

	GameWorld deletedObjectsInAPrefab;
	world.createPrefab(deletedObjectsInAPrefab, true, &m_deletedObjectIds);
	m_prefabWorldJson = serializeGameWorld(&deletedObjectsInAPrefab);
}

void CmdObjectDeletion::apply(GameInspector* inspector) {
	GameWorld* world = inspector->getWorld();

	for (ObjectId id : m_deletedObjectIds) {
		world->objectDelete(id);
	}
}

void CmdObjectDeletion::redo(GameInspector* inspector) {
	apply(inspector);
}

void CmdObjectDeletion::undo(GameInspector* inspector) {
	GameWorld* world = inspector->getWorld();
	world->instantiatePrefabFromJsonString(m_prefabWorldJson.c_str(), false, false);

	// Restore the hierarchy of the deleted objects.
	for (auto& pair : m_originalHierarchy) {
		ObjectId currActor = pair.first;
		world->setParentOf(currActor, pair.second.parent, true);

		for (ObjectId childId : pair.second.children) {
			world->setParentOf(childId, currActor, true);
		}
	}
}

//--------------------------------------------------------------------
// CmdExistingObjectCreation
//--------------------------------------------------------------------
void CmdExistingObjectCreation::setup(GameWorld& world, vector_set<ObjectId> targetObjects) {
	m_targetObjectIds = std::move(targetObjects);

	// Store the original hierarchy, we will need it for undo
	for (ObjectId obj : m_targetObjectIds) {
		ParentAndChilds& p = m_originalHierarchy[obj];
		p.parent = world.getParentId(obj);
		p.children = world.getChildensOfAsList(obj);
	}

	GameWorld prefabWorld;
	world.createPrefab(prefabWorld, true, &m_targetObjectIds);
	m_prefabWorldJson = serializeGameWorld(&prefabWorld);
}

void CmdExistingObjectCreation::apply(GameInspector* UNUSED(inspector)) {
	sgeAssert(false && "CmdExistingObjectCreation::apply should never get called, the objects are expected to be already created!");
}

void CmdExistingObjectCreation::redo(GameInspector* inspector) {
	GameWorld* world = inspector->getWorld();
	world->instantiatePrefabFromJsonString(m_prefabWorldJson.c_str(), false, false);

	// Restore the hierarchy of the deleted objects.
	for (auto& pair : m_originalHierarchy) {
		ObjectId currActor = pair.first;
		world->setParentOf(currActor, pair.second.parent, true);

		for (ObjectId childId : pair.second.children) {
			world->setParentOf(childId, currActor, true);
		}
	}
}

void CmdExistingObjectCreation::undo(GameInspector* inspector) {
	GameWorld* world = inspector->getWorld();

	for (ObjectId id : m_targetObjectIds) {
		world->objectDelete(id);
	}
}

//--------------------------------------------------------------------
// CmdObjectCreation
//--------------------------------------------------------------------
void CmdObjectCreation::setup(TypeId objectType) {
	m_objectType = objectType;
	m_createdObjectId = ObjectId();
}

void CmdObjectCreation::apply(GameInspector* inspector) {
	GameObject* const go = inspector->getWorld()->allocObject(m_objectType, m_createdObjectId);
	if (go != nullptr) {
		m_createdObjectId = go->getId();
	} else {
		sgeAssert(go != nullptr && "CmdObjectCreation failed");
	}
}

void CmdObjectCreation::redo(GameInspector* inspector) {
	apply(inspector);
}

void CmdObjectCreation::undo(GameInspector* inspector) {
	sgeAssert(m_createdObjectId.isNull() == false);
	inspector->getWorld()->objectDelete(m_createdObjectId);
}

//--------------------------------------------------------------------
// CmdDuplicateSpecial
//--------------------------------------------------------------------
void CmdDuplicateSpecial::apply(GameInspector* inspector) {
	GameWorld* const world = inspector->getWorld();

	// Duplicate the objects and build the ObjectId relationship remapping.
	std::unordered_map<ObjectId, ObjectId> destOf; // For each sourceObject this stores the id of the duplicate.
	std::unordered_map<ObjectId, ObjectId> srcOf;  // For each destObiect this stores the id of the source.
	std::vector<GameObject*> createdGameObjects;

	createdGameObjects.reserve(m_sourceGameObjectsIds.size());
	m_createdGameObjectsIds.reserve(m_sourceGameObjectsIds.size());

	for (ObjectId const srcActorId : m_sourceGameObjectsIds) {
		GameObject* const srcActor = world->getObjectById(srcActorId);
		if (!srcActor) {
			sgeAssert(false);
			continue;
		}

		// Caution:
		// We must call onDuplocationComplete() for the created actor!
		GameObject* const destGameObject = duplicateGameObjectIncompleate(srcActor);
		if_checked(destGameObject) {
			createdGameObjects.push_back(destGameObject);
			m_createdGameObjectsIds.insert(destGameObject->getId());
			destOf[srcActor->getId()] = destGameObject->getId();
			srcOf[destGameObject->getId()] = srcActor->getId();
		}
	}

	const std::function<void(GameObject*, MemberChain)> replaceObjectIds = [&](GameObject* dest, MemberChain chain) -> void {
		const TypeDesc* const typeDesc = chain.getType();
		if (!typeDesc) {
			return;
		}

		// When we reach this point replace the object id.
		// This is our exit point of the recursion.
		if (typeDesc->typeId == sgeTypeId(ObjectId)) {
			ObjectId& idToReplace = *(ObjectId*)chain.follow(dest);
			auto itr = destOf.find(idToReplace);

			// If the Id is in the duplicated soruces, replace it.
			if (itr != destOf.end()) {
				idToReplace = itr->second;
			}
		}

		// Check if this is a struct.
		for (const MemberDesc& mfd : typeDesc->members) {
			MemberChain toMemberChain = chain;
			toMemberChain.add(&mfd);
			replaceObjectIds(dest, toMemberChain);
		}

		// Check if this is a std::vector
		if (typeDesc->stdVectorUnderlayingType.isValid()) {
			int numElements = int(typeDesc->stdVectorSize(chain.follow(dest)));
			for (int t = 0; t < numElements; ++t) {
				MemberChain toMemberChain = chain;
				toMemberChain.knots.back().arrayIdx = t;
				replaceObjectIds(dest, toMemberChain);
			}
		}
	};

	// Now substitute the object IDs in the created actors.
	for (GameObject* const destGameObject : createdGameObjects) {
		const TypeDesc* const typeDesc = typeLib().find(destGameObject->getType());
		if (!typeDesc) {
			sgeAssert(false);
			continue;
		}

		for (const MemberDesc& mfd : typeDesc->members) {
			if (mfd.isEditable()) {
				MemberChain chain;
				chain.add(&mfd);
				replaceObjectIds(destGameObject, chain);
			}
		}
	}

	// Resolve the object hierarchy.
	for (GameObject* const destGameObject : createdGameObjects) {
		Actor* const destActor = destGameObject->getActor();
		if (!destActor) {
			continue;
		}

		ObjectId parentOfSourceId = world->getParentId(srcOf[destActor->getId()]);
		if (parentOfSourceId.isNull()) {
			continue;
		}

		// Check if the original parent object was duplicated.
		// If so, place the duplicate under it.
		auto itrParentDuplicate = destOf.find(parentOfSourceId);
		if (itrParentDuplicate != destOf.end()) {
			bool succeeded = world->setParentOf(destActor->getId(), itrParentDuplicate->second);
			if (succeeded == false) {
				std::string errorMsg = string_format("Cannot re-parent!", destActor->getDisplayNameCStr());
				getEngineGlobal()->showNotification(std::move(errorMsg));
			}
		}
	}


	// Finalize the duplication process by calling onDuplocationComplete()
	for (GameObject* const go : createdGameObjects) {
		go->onDuplocationComplete();
	}

	vector_set<ObjectId> createdActorsIds;
	for (GameObject* obj : createdGameObjects) {
		createdActorsIds.insert(obj->getId());
	}

	m_cmdExistingCreationHelper.setup(*world, createdActorsIds);
}

void CmdDuplicateSpecial::redo(GameInspector* inspector) {
	m_cmdExistingCreationHelper.redo(inspector);
}

void CmdDuplicateSpecial::undo(GameInspector* inspector) {
	m_cmdExistingCreationHelper.undo(inspector);
}

void CmdDuplicateSpecial::getText(std::string& text) {
	text = "CmdDuplicateSmart";
}


} // namespace sge
