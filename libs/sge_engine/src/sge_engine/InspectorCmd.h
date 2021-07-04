#pragma once

#include "sge_engine/Actor.h"
#include "sge_engine/GameObject.h"
#include "sge_engine_api.h"
#include "sge_utils/math/transform.h"
#include "sge_utils/utils/vector_set.h"
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace sge {

struct GameInspector;

//--------------------------------------------------------------------
// InspectorCmd
//--------------------------------------------------------------------
struct SGE_ENGINE_API InspectorCmd {
	InspectorCmd() = default;
	virtual ~InspectorCmd() = default;

	virtual void apply(GameInspector* inspector) = 0;
	virtual void redo(GameInspector* inspector) = 0;
	virtual void undo(GameInspector* inspector) = 0;

	virtual void getText(std::string& text) { text = "<command>"; }
};

// A set of commands.
struct SGE_ENGINE_API CmdCompound : public InspectorCmd {
	CmdCompound() = default;
	CmdCompound(const CmdCompound&) = delete;
	CmdCompound& operator=(const CmdCompound&) = delete;

	void addCommand(InspectorCmd* const cmd);
	void apply(GameInspector* inspector) final;
	void redo(GameInspector* inspector) final;
	void undo(GameInspector* inspector) final;

	void getText(std::string& text) final { text = cmdText; }

	std::string cmdText = "<Unnamed Compound Command>";
	std::vector<std::unique_ptr<InspectorCmd>> cmds;
};

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
struct SGE_ENGINE_API CmdMemberChange : public InspectorCmd {
	static void setActorLogicTransform(CmdMemberChange* cmd, GameInspector* inspector, void* dest, void* src);
	static void setActorLocalTransform(CmdMemberChange* cmd, GameInspector* inspector, void* dest, void* src);

	CmdMemberChange() = default;
	~CmdMemberChange();

	void setup(ObjectId const objId,
	           const MemberChain& chain,
	           const void* originalValue,
	           const void* newValue,
	           void (*customCopyFn)(CmdMemberChange* cmd, GameInspector* inspector, void* dest, void* src));

	void setup(GameObject* actor,
	           const MemberChain& chain,
	           const void* originalValue,
	           const void* newValue,
	           void (*customCopyFn)(CmdMemberChange* cmd, GameInspector* inspector, void* dest, void* src)) {
		if_checked(actor) { setup(actor->getId(), chain, originalValue, newValue, customCopyFn); }
	}

	void setupLogicTransformChange(Actor& actor, const transf3d& initalTransform, const transf3d& newTransform) {
		setup(&actor, MemberChain(typeLib().findMember(&Actor::m_logicTransform)), &initalTransform, &newTransform,
		      &setActorLogicTransform);
	}

	void setupLogicTransformChange(Actor& actor, const transf3d& newTransform) {
		transf3d originalValue = actor.getTransform();
		setupLogicTransformChange(actor, originalValue, newTransform);
	}

	void apply(GameInspector* inspector) final;
	void redo(GameInspector* inspector) final { apply(inspector); }
	void undo(GameInspector* inspector) final;
	void getText(std::string& text) final;

  public:
	ObjectId m_objectId;
	MemberChain m_chainToDynamicProp; // A path starting from the actor that will lead us to the member that we are going to change.
	void (*m_customCopyFn)(CmdMemberChange* cmd,
	                       GameInspector* inspector,
	                       void* dest,
	                       void* src) = nullptr; // if specified a custom copy function

	std::unique_ptr<char[]> m_orginaldata;
	std::unique_ptr<char[]> m_newData;
};

//--------------------------------------------------------------------
// CmdDynamicProperyChanged
//--------------------------------------------------------------------
struct SGE_ENGINE_API CmdDynamicProperyChanged : public InspectorCmd {
	CmdDynamicProperyChanged() = default;
	~CmdDynamicProperyChanged();

	void setup(GameObject* obj, const MemberChain& chain, std::string dynamicPropName, const void* originalValue, const void* newValue);

	void apply(GameInspector* inspector) final;
	void redo(GameInspector* inspector) final { apply(inspector); }
	void undo(GameInspector* inspector) final;
	void getText(std::string& text) final;

  public:

	ObjectId m_objectId;
	MemberChain m_chainToDynamicProp; // A path starting from the actor that will lead us the dynamic properties.
	std::string m_dynamicPropName;
	TypeId m_propertyTypeId;
	

	std::unique_ptr<char[]> m_orginaldata;
	std::unique_ptr<char[]> m_newData;
};

//--------------------------------------------------------------------
// CmdActorGrouping
//--------------------------------------------------------------------
struct SGE_ENGINE_API CmdActorGrouping : public InspectorCmd {
	CmdActorGrouping() = default;

	void setup(GameWorld& world, ObjectId parent, std::set<ObjectId> objectsToParentUnder);

	void apply(GameInspector* inspector) final;
	void redo(GameInspector* inspector) final;
	void undo(GameInspector* inspector) final;

	void getText(std::string& text) final { text = "Grouping Objects"; }

	ObjectId m_parentActorId;
	std::map<ObjectId, ObjectId> m_newChildrenAndTheirOldParents;
};

//--------------------------------------------------------------------
// CmdObjectDeletion
//--------------------------------------------------------------------
struct SGE_ENGINE_API CmdObjectDeletion : public InspectorCmd {
	void setupDeletion(GameWorld& world, vector_set<ObjectId> objectIdsToBeDeleted);

	void apply(GameInspector* inspector) final;
	void redo(GameInspector* inspector) final;
	void undo(GameInspector* inspector) final;

	void getText(std::string& text) final { text = "CmdObjectDeletion"; }

	struct ParentAndChilds {
		ObjectId parent;
		vector_set<ObjectId> children;
	};

  private:
	vector_set<ObjectId> m_deletedObjectIds;
	std::unordered_map<ObjectId, ParentAndChilds> m_originalHierarchy;
	std::string m_prefabWorldJson;
};

//--------------------------------------------------------------------
// CmdExistingObjectCreation
//--------------------------------------------------------------------
struct SGE_ENGINE_API CmdExistingObjectCreation : public InspectorCmd {
	void setup(GameWorld& world, vector_set<ObjectId> targetObjects);

	void apply(GameInspector* inspector) final;
	void redo(GameInspector* inspector) final;
	void undo(GameInspector* inspector) final;

	void getText(std::string& text) final { text = "CmdObjectDeletion"; }

	struct ParentAndChilds {
		ObjectId parent;
		vector_set<ObjectId> children;
	};

  private:
	vector_set<ObjectId> m_targetObjectIds;
	std::unordered_map<ObjectId, ParentAndChilds> m_originalHierarchy;
	std::string m_prefabWorldJson;
};


//--------------------------------------------------------------------
// CmdObjectCreation
//--------------------------------------------------------------------
struct SGE_ENGINE_API CmdObjectCreation : public InspectorCmd {
	void setup(TypeId objectType);

	void apply(GameInspector* inspector) final;
	void redo(GameInspector* inspector) final;
	void undo(GameInspector* inspector) final;

	void getText(std::string& text) final { text = "CmdObjectCreation"; }

	ObjectId getCreatedObjectId() const { return m_createdObjectId; }

  private:
	TypeId m_objectType;
	ObjectId m_createdObjectId;
};

//--------------------------------------------------------------------
// CmdDuplicateSpecial
//--------------------------------------------------------------------
struct SGE_ENGINE_API CmdDuplicateSpecial : public InspectorCmd {
	vector_set<ObjectId> m_sourceGameObjectsIds;
	vector_set<ObjectId> m_createdGameObjectsIds;


	void setup(vector_set<ObjectId> objectToDuplicate) { m_sourceGameObjectsIds = std::move(objectToDuplicate); }

	void apply(GameInspector* inspector) final;
	void redo(GameInspector* inspector) final;
	void undo(GameInspector* inspector) final;
	void getText(std::string& text) final;

  private:
	CmdExistingObjectCreation m_cmdExistingCreationHelper;
};



} // namespace sge
