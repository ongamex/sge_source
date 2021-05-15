#include "RigidBodyEditorConfig.h"
#include "Actor.h"
#include "IconsForkAwesome/IconsForkAwesome.h"
#include "sge_core/SGEImGui.h"
#include "sge_engine/windows/PropertyEditorWindow.h"
#include "traits/TraitModel.h"
#include "traits/TraitRigidBody.h"
#include "typelibHelper.h"

namespace sge {
// clang-format off
DefineTypeId(RigidBodyPropertiesConfigurator,    21'02'28'0006);
DefineTypeId(RigidBodyConfigurator::ShapeSource, 21'02'28'0007);
DefineTypeId(RigidBodyConfigurator,              21'02'28'0008);

ReflBlock()
{
	ReflAddType(RigidBodyPropertiesConfigurator)
		ReflMember(RigidBodyConfigurator, mass)
		ReflMember(RigidBodyPropertiesConfigurator, friction)
		ReflMember(RigidBodyPropertiesConfigurator, rollingFriction)
		ReflMember(RigidBodyPropertiesConfigurator, spinningFriction)
		ReflMember(RigidBodyPropertiesConfigurator, bounciness)
		ReflMember(RigidBodyPropertiesConfigurator, noMoveX).setPrettyName("No X Movement")
		ReflMember(RigidBodyPropertiesConfigurator, noMoveY).setPrettyName("No Y Movement")
		ReflMember(RigidBodyPropertiesConfigurator, noMoveZ).setPrettyName("No Z Movement")
		ReflMember(RigidBodyPropertiesConfigurator, noRotationX).setPrettyName("No X Rotation")
		ReflMember(RigidBodyPropertiesConfigurator, noRotationY).setPrettyName("No Y Rotation")
		ReflMember(RigidBodyPropertiesConfigurator, noRotationZ).setPrettyName("No Z Rotation")
		ReflMember(RigidBodyPropertiesConfigurator, movementDamping)
		ReflMember(RigidBodyPropertiesConfigurator, rotationDamping)
		ReflMember(RigidBodyPropertiesConfigurator, specifyGravity)
		ReflMember(RigidBodyPropertiesConfigurator, gravity)
	;

	ReflAddType(RigidBodyConfigurator::ShapeSource)
		ReflEnumVal(RigidBodyConfigurator::shapeSource_fromTraitModel, "From TraitModel")
		ReflEnumVal(RigidBodyConfigurator::shapeSource_manuallySpecify, "Manually Specify Shapes")
	;

	ReflAddType(RigidBodyConfigurator)
		ReflInherits(RigidBodyConfigurator, RigidBodyPropertiesConfigurator)
		ReflMember(RigidBodyConfigurator, shapeSource)
		ReflMember(RigidBodyConfigurator, assetPropery)
		ReflMember(RigidBodyConfigurator, collisionShapes)
	;	
}
// clang-format on

//---------------------------------------------------------------------
// RigidBodyConfigurator
//---------------------------------------------------------------------
void RigidBodyPropertiesConfigurator::applyProperties(Actor& actor) const {
	TraitRigidBody* const traitRb = getTrait<TraitRigidBody>(&actor);

	if (traitRb && traitRb->getRigidBody()) {
		applyProperties(*traitRb->getRigidBody());
	}
}

void RigidBodyPropertiesConfigurator::applyProperties(RigidBody& rb) const {
	rb.setFriction(friction);
	rb.setRollingFriction(rollingFriction);
	rb.setSpinningFriction(spinningFriction);
	rb.setBounciness(bounciness);
	rb.setCanMove(!noMoveX, !noMoveY, !noMoveZ);
	rb.setCanRotate(!noRotationX, !noRotationY, !noRotationZ);
	rb.setDamping(movementDamping, rotationDamping);
	if (specifyGravity) {
		rb.setGravity(gravity);
	}
}

void RigidBodyPropertiesConfigurator::extractPropsFromRigidBody(const RigidBody& rb) {
	mass = rb.getMass();
	friction = rb.getFriction();
	rollingFriction = rb.getRollingFriction();
	spinningFriction = rb.getSpinningFriction();
	bounciness = rb.getBounciness();

	rb.getCanMove(noMoveX, noMoveY, noMoveZ);
	noMoveX = !noMoveX;
	noMoveY = !noMoveY;
	noMoveZ = !noMoveZ;

	rb.getCanRotate(noRotationX, noRotationY, noRotationZ);
	noRotationX = !noRotationX;
	noRotationY = !noRotationY;
	noRotationZ = !noRotationZ;

	rb.getDamping(movementDamping, rotationDamping);

	gravity = rb.getGravity();
}

//---------------------------------------------------------------------
// RigidBodyConfigurator
//---------------------------------------------------------------------
bool RigidBodyConfigurator::apply(Actor& actor, bool addToWorldNow) const {
	TraitRigidBody* const traitRb = getTrait<TraitRigidBody>(&actor);

	if (traitRb == nullptr) {
		return false;
	}

	const transf3d transformOfActor = actor.getTransform();

	switch (shapeSource) {
		case shapeSource_fromTraitModel: {
			TraitModel* const traitModel = getTrait<TraitModel>(&actor);
			if (traitModel) {
				AssetModel* const assetModel = traitModel->getAssetProperty().getAssetModel();
				if (assetModel) {
					traitRb->destroyRigidBody();
					traitRb->createBasedOnModel(assetModel->staticEval, mass, false, false);
				}
			}
		} break;
		case shapeSource_manuallySpecify: {
			traitRb->destroyRigidBody();
			traitRb->getRigidBody()->create(&actor, collisionShapes.data(), int(collisionShapes.size()), mass, false);
		} break;
		default: {
			sgeAssert(false && "Not Implemented ShapeSource");
		} break;
	}

	traitRb->getRigidBody()->setTransformAndScaling(transformOfActor, true);

	// Apply the properties.
	applyProperties(*traitRb->getRigidBody());

	if (addToWorldNow) {
		traitRb->addToWorld();
	}

	return true;
}

//---------------------------------------------------------------------
// User Interface
//---------------------------------------------------------------------
void edit_CollisionShapeDesc(GameInspector& inspector, GameObject* gameObject, MemberChain chain) {
	CollsionShapeDesc& sdesc = *reinterpret_cast<CollsionShapeDesc*>(chain.follow(gameObject));

	const ImGuiEx::IDGuard idGuard(&sdesc);

	auto doMemberUIFn = [&](const MemberDesc* const md) -> void {
		if (md) {
			chain.add(md);
			ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain);
			chain.pop();
		}
	};

	doMemberUIFn(sgeFindMember(CollsionShapeDesc, offset));
	doMemberUIFn(sgeFindMember(CollsionShapeDesc, type));

	switch (sdesc.type) {
		case CollsionShapeDesc::type_box: {
			doMemberUIFn((sgeFindMember(CollsionShapeDesc, boxHalfDiagonal)));
		} break;
		case CollsionShapeDesc::type_sphere: {
			doMemberUIFn((sgeFindMember(CollsionShapeDesc, sphereRadius)));
		} break;
		case CollsionShapeDesc::type_capsule: {
			doMemberUIFn((sgeFindMember(CollsionShapeDesc, capsuleHeight)));
			doMemberUIFn((sgeFindMember(CollsionShapeDesc, capsuleRadius)));
		} break;
		case CollsionShapeDesc::type_cylinder: {
			doMemberUIFn((sgeFindMember(CollsionShapeDesc, cylinderHalfDiagonal)));
		} break;
		case CollsionShapeDesc::type_cone: {
			doMemberUIFn((sgeFindMember(CollsionShapeDesc, coneHeight)));
			doMemberUIFn((sgeFindMember(CollsionShapeDesc, coneRadius)));
		} break;
		default: {
			ImGui::TextEx("Not Implemented");
		}
	}
}

void edit_RigidBodyPropertiesConfigurator(GameInspector& inspector, GameObject* gameObject, MemberChain chain) {
	RigidBodyPropertiesConfigurator& rbpc = *reinterpret_cast<RigidBodyPropertiesConfigurator*>(chain.follow(gameObject));

	auto doMemberUIFn = [&](const MemberDesc* const md) -> void {
		if (md) {
			chain.add(md);
			ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain);
			chain.pop();
		}
	};

	const ImGuiEx::IDGuard idGuard(&rbpc);
	ImGuiEx::BeginGroupPanel("Rigid Body Properties Config");

	if (rbpc.dontShowDynamicProperties == false) {
		doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, mass));
	}

	doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, friction));
	doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, rollingFriction));
	doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, spinningFriction));
	doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, bounciness));

	if (rbpc.dontShowDynamicProperties == false) {
		doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, noMoveX));
		doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, noMoveY));
		doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, noMoveZ));
		doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, noRotationX));
		doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, noRotationY));
		doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, noRotationZ));
		doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, movementDamping));
		doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, rotationDamping));

		doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, specifyGravity));
		if (rbpc.specifyGravity) {
			doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, gravity));
		}
	}

	if (ImGui::Button(ICON_FK_CHECK " Apply")) {
		Actor* const actor = gameObject->getActor();
		if (actor) {
			rbpc.applyProperties(*actor);
		}
	}

	ImGuiEx::EndGroupPanel();
}

SGE_ENGINE_API void edit_RigidBodyConfigurator(GameInspector& inspector, GameObject* gameObject, MemberChain chain) {
	RigidBodyConfigurator& rbec = *reinterpret_cast<RigidBodyConfigurator*>(chain.follow(gameObject));

	const ImGuiEx::IDGuard idGuard(&rbec);

	ImGuiEx::BeginGroupPanel("Rigid Body Config");

	auto doMemberUIFn = [&](const MemberDesc* const md) -> void {
		if (md) {
			chain.add(md);
			ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain);
			chain.pop();
		}
	};

	if (rbec.dontShowDynamicProperties == false) {
		doMemberUIFn(sgeFindMember(RigidBodyConfigurator, mass));
	}

	doMemberUIFn(sgeFindMember(RigidBodyConfigurator, friction));
	doMemberUIFn(sgeFindMember(RigidBodyConfigurator, rollingFriction));
	doMemberUIFn(sgeFindMember(RigidBodyConfigurator, spinningFriction));
	doMemberUIFn(sgeFindMember(RigidBodyConfigurator, bounciness));

	if (rbec.dontShowDynamicProperties == false) {
		doMemberUIFn(sgeFindMember(RigidBodyConfigurator, noMoveX));
		doMemberUIFn(sgeFindMember(RigidBodyConfigurator, noMoveY));
		doMemberUIFn(sgeFindMember(RigidBodyConfigurator, noMoveZ));
		doMemberUIFn(sgeFindMember(RigidBodyConfigurator, noRotationX));
		doMemberUIFn(sgeFindMember(RigidBodyConfigurator, noRotationY));
		doMemberUIFn(sgeFindMember(RigidBodyConfigurator, noRotationZ));
		doMemberUIFn(sgeFindMember(RigidBodyConfigurator, movementDamping));
		doMemberUIFn(sgeFindMember(RigidBodyConfigurator, rotationDamping));

		doMemberUIFn(sgeFindMember(RigidBodyConfigurator, specifyGravity));
		if (rbec.specifyGravity) {
			doMemberUIFn(sgeFindMember(RigidBodyConfigurator, gravity));
		}
	}


	doMemberUIFn(sgeFindMember(RigidBodyConfigurator, shapeSource));

	if (rbec.shapeSource == RigidBodyConfigurator::shapeSource_fromTraitModel) {
		if (gameObject->findTraitByFamily(sgeTypeId(TraitModel)) == nullptr) {
			ImGui::TextEx("The current objects needs to have TraitModel for this option to work");
		}
	} else if (rbec.shapeSource == RigidBodyConfigurator::shapeSource_manuallySpecify) {
		ImGui::Text("Manually specify the shapes that will form the rigid body below!");
		doMemberUIFn(sgeFindMember(RigidBodyConfigurator, collisionShapes));
	}

	if (ImGui::Button(ICON_FK_CHECK " Apply")) {
		Actor* const actor = gameObject->getActor();
		if (actor) {
			rbec.apply(*actor, true);
		}
	}

	ImGuiEx::EndGroupPanel();
}



} // namespace sge
