#include "AStaticObstacle.h"
#include "sge_core/ICore.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/RigidBodyFromModel.h"

namespace sge {

//--------------------------------------------------------------------
// AStaticObstacle
//--------------------------------------------------------------------
DefineTypeId(CollisionShapeSource, 20'03'02'0005);
ReflBlock() {
	ReflAddType(CollisionShapeSource) ReflEnumVal((int)CollisionShapeSource::FromBoundingBox, "FromBoundingBox")
	    ReflEnumVal((int)CollisionShapeSource::FromConcaveHulls, "FromConcaveHulls")
	        ReflEnumVal((int)CollisionShapeSource::FromConvexHulls, "FromConvexHulls");
}

DefineTypeId(AStaticObstacle, 20'03'02'0006);
ReflBlock() {
	ReflAddActor(AStaticObstacle) ReflMember(AStaticObstacle, m_traitModel);
}

AABox3f AStaticObstacle::getBBoxOS() const {
	return m_traitModel.getBBoxOS();
}

void AStaticObstacle::create() {
	registerTrait(m_traitRB);
	registerTrait(m_traitModel);
	//m_traitModel.getAssetProperty().setTargetAsset("assets/editor/models/roundedCube.mdl");
}

void AStaticObstacle::postUpdate(const GameUpdateSets& UNUSED(updateSets)) {
	if (m_traitModel.postUpdate()) {
		if (m_traitRB.getRigidBody()->isValid()) {
			this->getWorld()->physicsWorld.removePhysicsObject(*m_traitRB.getRigidBody());
			m_traitRB.getRigidBody()->destroy();
		}

		const AssetModel* const assetModel = m_traitModel.getAssetProperty().getAssetModel();
		if (assetModel && assetModel->staticEval.isInitialized()) {
			// Create the new rigid body if we succesfully created a shape.
			std::vector<CollsionShapeDesc> shapeDescs;
			const bool hasShape = initializeCollisionShapeBasedOnModel(shapeDescs, assetModel->staticEval);
			if (hasShape) {
				m_traitRB.getRigidBody()->create((Actor*)this, shapeDescs.data(), int(shapeDescs.size()), 0.f, false);

				// CAUTION: this looks a bit hacky but it is used to set the physics scaling.
				// TODO: Check if it would be better if we explicitly set it here.
				setTransform(getTransform(), true);
				getWorld()->physicsWorld.addPhysicsObject(*m_traitRB.getRigidBody());
			} else {
				SGE_DEBUG_ERR("Static obstacle failed to create rigid body, the shape isn't valid!\n");
			}
		}
	}
}

} // namespace sge