#include <functional>

#include "sge_core/AssetLibrary.h"
#include "sge_utils/math/transform.h"
#include "sge_utils/utils/range_loop.h"

#include "EvaluatedModel.h"
#include "Model.h"

namespace sge {

void EvaluatedModel::initialize(AssetLibrary* const assetLibrary, Model* model) {
	sgeAssert(assetLibrary != nullptr);
	sgeAssert(model != nullptr);

	*this = EvaluatedModel();
	m_assetLibrary = assetLibrary;
	m_model = model;
}

int EvaluatedModel::addAnimationDonor(const std::shared_ptr<Asset>& donorAsset) {
	if (isAssetLoaded(donorAsset, AssetType::Model) == false || !isInitialized()) {
		return -1;
	}

	for (int iDonor = 0; iDonor < m_donors.size(); ++iDonor) {
		if (donorAsset == m_donors[iDonor].donorModel) {
			return iDonor;
		}
	}

	AnimationDonor animDonor;

	animDonor.donorModel = donorAsset;
	animDonor.originalNodeId_to_donorNodeId.resize(m_model->numNodes(), -1);

	// Build the node-to-node remapping, Keep in mind that some nodes might not be present in the donor.
	// in that case -1 should be written for that node index.
	for (const int iOrigNode : range_int(m_model->numNodes())) {
		const std::string& originalNodeName = m_model->nodeAt(iOrigNode)->name;
		animDonor.originalNodeId_to_donorNodeId[iOrigNode] =
		    animDonor.donorModel->asModel()->model.findFistNodeIndexWithName(originalNodeName);
	}

	m_donors.emplace_back(std::move(animDonor));

	return int(m_donors.size()) - 1;
}

bool EvaluatedModel::evaluateFromMoments(const EvalMomentSets evalMoments[], int numMoments) {
	if (numMoments != 0 && evalMoments != nullptr) {
		evaluateFromMomentsInternal(evalMoments, numMoments);
	} else {
		EvalMomentSets staticMoment;
		evaluateFromMomentsInternal(&staticMoment, 1);
	}
	evaluateMaterials();
	evaluateSkinning();

	return true;
}

bool EvaluatedModel::evaluateFromNodesGlobalTransform(const std::vector<mat4f>& boneGlobalTrasnformOverrides) {
	evaluateNodesFromExternalBones(boneGlobalTrasnformOverrides);
	evaluateMaterials();
	evaluateSkinning();
	return true;
}

bool EvaluatedModel::evaluateNodes_common() {
	aabox.setEmpty();
	m_evaluatedNodes.resize(m_model->numNodes());

	// Initialize the attachments to the node.

	for (int iNode : range_int(m_model->numNodes())) {
		EvaluatedNode& evalNode = m_evaluatedNodes[iNode];
		// [EVAL_MESH_NODE_DEFAULT_ZERO]
		evalNode.evalLocalTransform = mat4f::getZero();
		evalNode.evalGlobalTransform = mat4f::getZero();

		// Obtain the inital bounding box by getting the unevaluated attached meshes bounding boxes.
		for (const MeshAttachment& meshAttachment : m_model->nodeAt(iNode)->meshAttachments) {
			const ModelMesh* mesh = m_model->meshAt(meshAttachment.attachedMeshIndex);
			if (mesh) {
				evalNode.aabbGlobalSpace.expand(mesh->aabox);
			}
		}
	}

	return true;
}

bool EvaluatedModel::evaluateFromMomentsInternal(const EvalMomentSets evalMoments[], int numMoments) {
	evaluateNodes_common();

	// Evaluates the nodes. They may be effecte by multiple models (stealing animations and blending them)
	for (int const iMoment : range_int(numMoments)) {
		const EvalMomentSets& moment = evalMoments[iMoment];

		// Find the animation donor.
		const AnimationDonor* donor = nullptr;
		if (moment.donorIndex >= 0) {
			if (moment.donorIndex < int(m_donors.size())) {
				donor = &m_donors[moment.donorIndex];
			} else {
				sgeAssert(false && "Animation donor with the specified index could not be found!");
				continue;
			}
		}

		const Model& donorModel = (donor != nullptr) ? donor->donorModel->asModel()->model : *m_model;
		const ModelAnimation* const donorAnimation = donorModel.animationAt(moment.animationIndex);

		const float evalTime = moment.time;

		for (int iOrigNode = 0; iOrigNode < m_model->numNodes(); ++iOrigNode) {
			// Use the node form the specified Model in the node, if such node doesn't exists, fallback to the originalNode.
			const int donorNodeIndex = (donor != nullptr) ? donor->originalNodeId_to_donorNodeId[iOrigNode] : iOrigNode;

			// Find the node that is equvalent to the node in @m_model and evaluate its transform.
			// If no such node was found use the default transformation from @m_model.
			transf3d nodeLocalTransform;
			if (donorNodeIndex >= 0) {
				nodeLocalTransform = donorModel.nodeAt(donorNodeIndex)->staticLocalTransform;
				if (donorAnimation != nullptr) {
					donorAnimation->evaluateForNode(nodeLocalTransform, donorNodeIndex, evalTime);
				}
			} else {
				nodeLocalTransform = m_model->nodeAt(iOrigNode)->staticLocalTransform;
			}

			// Caution: [EVAL_MESH_NODE_DEFAULT_ZERO]
			// It is assumed that all transforms in evalNode are initialized to zero!
			m_evaluatedNodes[iOrigNode].evalLocalTransform += nodeLocalTransform.toMatrix() * moment.weight;
		}
	}

	// Evaluate the node global transform by traversing the node hierarchy using the local transform computed above.
	// Evaluate attached meshes to the evaluated nodes.
	std::function<void(int, mat4f)> traverseGlobalTransform;
	traverseGlobalTransform = [&](int iNode, const mat4f& parentTransfrom) -> void {
		EvaluatedNode& evalNode = m_evaluatedNodes[iNode];
		evalNode.evalGlobalTransform = parentTransfrom * evalNode.evalLocalTransform;
		evalNode.aabbGlobalSpace = evalNode.aabbGlobalSpace.getTransformed(evalNode.evalGlobalTransform);

		for (const int childNodeIndex : m_model->nodeAt(iNode)->childNodes) {
			traverseGlobalTransform(childNodeIndex, evalNode.evalGlobalTransform);
		}

		if (evalNode.aabbGlobalSpace.IsEmpty() == false) {
			aabox.expand(evalNode.aabbGlobalSpace);
		}
	};

	traverseGlobalTransform(m_model->getRootNodeIndex(), mat4f::getIdentity());
	return true;
}

bool EvaluatedModel::evaluateNodesFromExternalBones(const std::vector<mat4f>& boneGlobalTrasnformOverrides) {
	evaluateNodes_common();

	if (boneGlobalTrasnformOverrides.size() != m_model->numNodes()) {
		sgeAssert(false && "It seems that the provided amount of node transforms isn't the one that we expect");
		return false;
	}

	for (int iNode = 0; iNode < m_model->numNodes(); ++iNode) {
		EvaluatedNode& evalNode = m_evaluatedNodes[iNode];

		// evalNode.evalLocalTransform is not computed as it isn't needed by anything at the moment.
		evalNode.evalGlobalTransform = boneGlobalTrasnformOverrides[iNode];
		evalNode.aabbGlobalSpace = evalNode.aabbGlobalSpace.getTransformed(evalNode.evalGlobalTransform);
		aabox.expand(evalNode.aabbGlobalSpace.getTransformed(evalNode.evalGlobalTransform));
	}

	return true;
}

bool EvaluatedModel::evaluateMaterials() {
	if (areMaterialsAlreadyEvaluated) {
		return false;
	}

	areMaterialsAlreadyEvaluated = true;
	m_evaluatedMaterials.resize(m_model->numMaterials());

	std::string texPath;

	for (int iMaterial = 0; iMaterial < m_model->numMaterials(); ++iMaterial) {
		EvaluatedMaterial& evalMtl = m_evaluatedMaterials[iMaterial];
		ModelMaterial* rawMaterial = m_model->materialAt(iMaterial);

		evalMtl.diffuseColor = rawMaterial->diffuseColor;
		evalMtl.roughness = rawMaterial->roughness;
		evalMtl.metallic = rawMaterial->metallic;

		// Check if there is a diffuse texture attached here.
		if (rawMaterial->diffuseTextureName.empty() == false) {
			texPath = m_model->getModelLoadSetting().assetDir + rawMaterial->diffuseTextureName;
			evalMtl.diffuseTexture = m_assetLibrary->getAsset(AssetType::Texture2D, texPath.c_str(), true);
		}

		// Normal map.
		if (rawMaterial->normalTextureName.empty() == false) {
			texPath = m_model->getModelLoadSetting().assetDir + rawMaterial->normalTextureName;
			evalMtl.texNormalMap = m_assetLibrary->getAsset(AssetType::Texture2D, texPath.c_str(), true);
		}

		// Metallic map.
		if (rawMaterial->metallicTextureName.empty() == false) {
			texPath = m_model->getModelLoadSetting().assetDir + rawMaterial->metallicTextureName;
			evalMtl.texMetallic = m_assetLibrary->getAsset(AssetType::Texture2D, texPath.c_str(), true);
		}

		// Roughness map.
		if (rawMaterial->roughnessTextureName.empty() == false) {
			texPath = m_model->getModelLoadSetting().assetDir + rawMaterial->roughnessTextureName;
			evalMtl.texRoughness = m_assetLibrary->getAsset(AssetType::Texture2D, texPath.c_str(), true);
		}
	}

	return true;
}

bool EvaluatedModel::evaluateSkinning() {
	SGEContext* const context = m_assetLibrary->getDevice()->getContext();

	m_evaluatedMeshes.resize(m_model->numMeshes());

	// Evaluate the meshes.
	for (int iMesh = 0; iMesh < m_model->numMeshes(); ++iMesh) {
		EvaluatedMesh& evalMesh = m_evaluatedMeshes[iMesh];
		const ModelMesh& rawMesh = *m_model->meshAt(iMesh);

		if (rawMesh.bones.empty() == false) {
			// Compute the tansform of the bone, it combines the binding offset matrix of the bone and
			// the evaluated position of the node that represents the bone in the scene.
			std::vector<mat4f> bonesTransformTexData(rawMesh.bones.size());
			for (int iBone = 0; iBone < rawMesh.bones.size(); ++iBone) {
				const ModelMeshBone& bone = rawMesh.bones[iBone];

				const mat4f boneTransformWithOffsetModelObjectSpace =
				    m_evaluatedNodes[bone.nodeIdx].evalGlobalTransform * bone.offsetMatrix;

				bonesTransformTexData[iBone] = boneTransformWithOffsetModelObjectSpace;
			}

			// Compute the bones skinning matrix.
			// TODO: this texture should be created by a shader if needed and maybe reused between draw calls.
			{
				int neededTexWidth = 4;
				int neededTexHeight = int(rawMesh.bones.size());

				TextureData data = TextureData(bonesTransformTexData.data(), sizeof(vec4f) * 4);

				const bool doesBigEnoughTextureExists = evalMesh.skinningBoneTransfsTex.HasResource() &&
				                                        evalMesh.skinningBoneTransfsTex->getDesc().texture2D.height >= neededTexHeight;

				if (doesBigEnoughTextureExists == false) {
					TextureDesc td;
					td.textureType = UniformType::Texture2D;
					td.usage = TextureUsage::DynamicResource;
					td.format = TextureFormat::R32G32B32A32_FLOAT;
					td.texture2D.arraySize = 1;
					td.texture2D = Texture2DDesc(neededTexWidth, neededTexHeight);

					SamplerDesc sd;
					sd.filter = TextureFilter::Min_Mag_Mip_Point;

					evalMesh.skinningBoneTransfsTex = context->getDevice()->requestResource<Texture>();
					evalMesh.skinningBoneTransfsTex->create(td, &data, sd);
				} else {
					context->updateTextureData(evalMesh.skinningBoneTransfsTex.GetPtr(), data);
				}
			}
		}

		sgeAssert(!rawMesh.bones.empty() == evalMesh.skinningBoneTransfsTex.HasResource());

		// Finally fill the geometry structure.
		evalMesh.geometry = Geometry(rawMesh.vertexBuffer.GetPtr(), rawMesh.indexBuffer.GetPtr(), evalMesh.skinningBoneTransfsTex.GetPtr(),
		                             rawMesh.vertexDeclIndex, rawMesh.vbVertexColorOffsetBytes >= 0, rawMesh.vbUVOffsetBytes >= 0,
		                             rawMesh.vbNormalOffsetBytes >= 0, rawMesh.hasUsableTangetSpace, rawMesh.primitiveTopology,
		                             rawMesh.vbByteOffset, rawMesh.ibByteOffset, rawMesh.stride, rawMesh.ibFmt, rawMesh.numElements);
	}

	return true;
}

const ModelAnimation* EvaluatedModel::findAnimation(const int idxDonor, const int animIndex) const {
	if (idxDonor == -1) {
		return m_model->animationAt(animIndex);
	}

	if (idxDonor >= 0 && idxDonor < m_donors.size()) {
		return m_donors[idxDonor].donorModel->asModel()->model.animationAt(animIndex);
	}

	return nullptr;
}

} // namespace sge
