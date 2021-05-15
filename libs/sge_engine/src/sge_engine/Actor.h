#pragma once

#include "sge_utils/math/Box.h"
#include "sge_utils/math/transform.h"

#include "sge_engine/GameObject.h"

namespace sge {
struct Trait;
struct GameInspector;
struct InspectorCmd;
struct SelectedItem;

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
struct SGE_ENGINE_API Actor : public GameObject {
	// Things that are common for every actor.
	Actor() = default;
	virtual ~Actor() = default;

	const transf3d& getTransform() const { return m_logicTransform; }
	const mat4f& getTransformMtx() const;

	/// Shorthand for getting the position of the actor in world space.
	const vec3f& getPosition() const { return m_logicTransform.p; }
	const quatf& getOrientation() const { return m_logicTransform.r; }

	/// Shorthand for retrieving the direction of an axis in world space.
	const vec3f getDirX() const { return getTransformMtx().c0.xyz(); }
	const vec3f getDirY() const { return getTransformMtx().c1.xyz(); }
	const vec3f getDirZ() const { return getTransformMtx().c2.xyz(); }

	//// Physics engine can't modify the scaling of the object. So this is a faster alternative for that case.
	// void setTransformFromPhysicsInternal(const vec3f& p, const quatf& r);

	/*virtual */ void setTransform(const transf3d& transform, bool killVelocity = true);
	void setLocalTransform(const transf3d& localTransform, bool killVelocity = true);

	void setPosition(const vec3f& p, bool killVelocity = true);
	void setOrientation(const quatf& r, bool killVelocity = true);
	// Called after the physics simulation has ended and before update().

	void setTransformEx(const transf3d& transform, bool killVelocity, bool recomputeBinding, bool shouldChangeRigidBodyTransform);

	// Returns the aabb in object space. The box may be empty if not applicable.
	// This is not intended for physics or any game logic.
	// This should be used for the editor and the rendering.
	virtual AABox3f getBBoxOS() const = 0;

	virtual int getNumItemsInMode(EditMode const mode) const {
		if (mode == editMode_actors)
			return 1;
		return 0;
	}

	virtual bool getItemTransform(transf3d& result, EditMode const mode, int UNUSED(itemIndex)) {
		if (mode == editMode_actors) {
			result = getTransform();
			return true;
		}

		// You probably want to implement this?
		sgeAssert(false);
		result = getTransform();

		return false;
	}

	// TODO: Should perform some checks when we are in actor mode>
	virtual void setItemTransform(EditMode const mode, int UNUSED(itemIndex), const transf3d& tr) {
		if (mode == editMode_actors) {
			setTransformEx(tr, true, true, true);
			return;
		}

		// You probably want to implement this?
		sgeAssert(false);
	}

	virtual InspectorCmd*
	    generateDeleteItemCmd(GameInspector* inspector, const SelectedItem* items, int numItems, bool ifActorModeShouldDeleteActorsUnder);

	virtual InspectorCmd* generateItemSetTransformCmd(
	    GameInspector* inspector, EditMode const mode, int itemIndex, const transf3d& initalTrasform, const transf3d& newTransform);

  public:
	transf3d m_logicTransform = transf3d::getIdentity();
	transf3d m_bindingToParentTransform = transf3d::getIdentity();
	bool m_bindingIgnoreRotation = false;
	mutable mat4f m_trasformAsMtx;
	mutable bool m_isTrasformAsMtxValid = false;
};

} // namespace sge
