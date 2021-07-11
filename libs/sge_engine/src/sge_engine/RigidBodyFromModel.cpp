#include "RigidBodyFromModel.h"
#include "sge_core/AssetLibrary.h"
#include "sge_core/ICore.h"
#include "sge_core/model/EvaluatedModel.h"
#include "sge_engine/Physics.h"
#include "sge_engine/traits/TraitModel.h"


namespace sge {

bool addCollisionShapeBasedOnModel(std::vector<CollsionShapeDesc>& shapeDescs, const EvaluatedModel& evaluatedMode) {
	const Model* const model = evaluatedMode.m_model;
	if (model == nullptr) {
		return false;
	}

	// Attempt to use the convex hulls first.
	if (model->m_convexHulls.size() > 0) {
		for (const ModelCollisionMesh& cvxHull : model->m_convexHulls) {
			shapeDescs.emplace_back(CollsionShapeDesc::createConvexPoly(cvxHull.vertices, cvxHull.indices));
		}
	}
	// Then the concave.
	else if (model->m_concaveHulls.size() > 0) {
		for (const ModelCollisionMesh& cvxHull : model->m_concaveHulls) {
			shapeDescs.emplace_back(CollsionShapeDesc::createTriMesh(cvxHull.vertices, cvxHull.indices));
		}
	}
	// Then the collision shapes (boxes, capsules, cylinders ect.)
	else {
		for (const Model_CollisionShapeBox& box : model->m_collisionBoxes) {
			shapeDescs.emplace_back(CollsionShapeDesc::createBox(box.halfDiagonal, box.transform));
		}

		for (const Model_CollisionShapeCapsule& capsule : model->m_collisionCapsules) {
			shapeDescs.emplace_back(CollsionShapeDesc::createCapsule(capsule.radius, capsule.halfHeight * 2.f, capsule.transform));
		}

		for (const Model_CollisionShapeCylinder& cylinder : model->m_collisionCylinders) {
			shapeDescs.emplace_back(CollsionShapeDesc::createCylinder(cylinder.halfDiagonal, cylinder.transform));
		}

		for (const Model_CollisionShapeSphere& sphere : model->m_collisionSpheres) {
			shapeDescs.emplace_back(CollsionShapeDesc::createSphere(sphere.radius, sphere.transform));
		}

		if (shapeDescs.empty()) {
			// Fallback to the bounding box of the whole 3D model.
			AABox3f modelBBox = evaluatedMode.aabox;

			// For example if we have a single plane for obsticle,
			// the bounding box by some axis could be 0, in order not to break the physics
			// add some size in that axis.
			vec3f boxSize = modelBBox.size();
			if (boxSize.x < 1e-2f)
				modelBBox.max.x += 0.01f, modelBBox.min.x -= 1e-2f;
			if (boxSize.y < 1e-2f)
				modelBBox.max.y += 0.01f, modelBBox.min.y -= 1e-2f;
			if (boxSize.z < 1e-2f)
				modelBBox.max.z += 0.01f, modelBBox.min.z -= 1e-2f;

			if (modelBBox.IsEmpty() == false) {
				shapeDescs.emplace_back(CollsionShapeDesc::createBox(modelBBox));
			}
		}
	}

	return true;
}

bool addCollisionShapeBasedOnModel(std::vector<CollsionShapeDesc>& shapeDescs, const char* modelAssetPath) {
	std::shared_ptr<Asset> modelAsset = getCore()->getAssetLib()->getAsset(AssetType::Model, modelAssetPath, true);
	if (!isAssetLoaded(modelAsset)) {
		return false;
	}

	return addCollisionShapeBasedOnModel(shapeDescs, modelAsset->asModel()->staticEval);
}

bool addCollisionShapeBasedOnTraitModel(std::vector<CollsionShapeDesc>& shapeDescs, TraitModel& traitModel) {
	bool hadShapes = false;
	for (TraitModel::PerModelSettings& mdlSets : traitModel.m_models) {
		AssetModel* const assetModel = mdlSets.m_assetProperty.getAssetModel();
		if (assetModel) {
			hadShapes |= addCollisionShapeBasedOnModel(shapeDescs, assetModel->staticEval);
		}
	}

	return hadShapes;
}

} // namespace sge
