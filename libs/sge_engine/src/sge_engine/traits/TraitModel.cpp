#include "TraitModel.h"
#include "IconsForkAwesome/IconsForkAwesome.h"
#include "sge_core/SGEImGui.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GameInspector.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/TypeRegister.h"
#include "sge_engine/actors/ALocator.h"
#include "sge_engine/materials/Material.h"
#include "sge_engine/windows/PropertyEditorWindow.h"
#include "sge_utils/loop.h"
#include "sge_utils/stl_algorithm_ex.h"
#include "sge_utils/utils/vector_set.h"

namespace sge {

struct MDiffuseMaterial;

//-------------------------------------------------------------------
// TraitModel
//-------------------------------------------------------------------
// clang-format off
DefineTypeId(TraitModel, 20'03'01'0004);
DefineTypeId(TraitModel::MaterialOverride, 20'10'15'0001);
DefineTypeId(std::vector<TraitModel::MaterialOverride>, 20'10'15'0002);

ReflBlock() {
	ReflAddType(TraitModel::MaterialOverride)
		ReflMember(TraitModel::MaterialOverride, materialName)
		ReflMember(TraitModel::MaterialOverride, materialObjId).setPrettyName("Material Object")
	;

	ReflAddType(std::vector<TraitModel::MaterialOverride>);

	ReflAddType(TraitModel)
		ReflMember(TraitModel, m_assetProperty)
		ReflMember(TraitModel, m_materialOverrides)
		ReflMember(TraitModel, useSkeleton)
		ReflMember(TraitModel, rootSkeletonId)
	;

}
// clang-format on

AABox3f TraitModel::getBBoxOS() const {
	// If the attached asset is a model use it to compute the bounding box.
	const AssetModel* const assetModel = getAssetProperty().getAssetModel();
	if (assetModel && assetModel->staticEval.isInitialized()) {
		AABox3f bbox = assetModel->staticEval.aabox.getTransformed(m_additionalTransform);
		return bbox;
	}

	return AABox3f();
}

void TraitModel::computeNodeToBoneIds() {
	if (nodeToBoneId.empty() == false) {
		return;
	}

	nodeToBoneId.clear();

	if (useSkeleton == false) {
		return;
	}

	AssetModel* assetmodel = m_assetProperty.getAssetModel();
	if (assetmodel == nullptr) {
		return;
	}

	GameWorld* world = getWorld();
	vector_set<ObjectId> boneActorIds;
	world->getAllChildren(boneActorIds, rootSkeletonId);
	boneActorIds.insert(rootSkeletonId);

	vector_set<Actor*> boneActors;
	for (ObjectId boneId : boneActorIds) {
		Actor* bone = world->getActorById(boneId);
		if (bone == nullptr) {
			nodeToBoneId.clear();
			sgeAssert("Could not find a bone for the actor!");
			return;
		}

		boneActors.insert(bone);
	}

	for (const int iNode : rng_int(assetmodel->model.numNodes())) {
		const ModelNode* node = assetmodel->model.nodeAt(iNode);
		for (Actor* boneActor : boneActors) {
			if (node->name == boneActor->getDisplayName()) {
				nodeToBoneId[iNode] = boneActor->getId();
				break;
			}
		}
	}
}

void TraitModel::computeSkeleton(std::vector<mat4f>& boneOverrides) {
	AssetModel* assetmodel = m_assetProperty.getAssetModel();
	if (assetmodel == nullptr) {
		return;
	}

	boneOverrides.resize(assetmodel->model.numNodes());

	if (useSkeleton == false) {
		return;
	}

	computeNodeToBoneIds();
	Actor* root = getWorld()->getActorById(rootSkeletonId);
	if (root == nullptr) {
		return;
	}

	mat4f rootInv = root ? root->getTransformMtx().inverse() : mat4f::getIdentity();

	for (auto pair : nodeToBoneId) {
		mat4f& boneGobalTForm = boneOverrides[pair.first];

		Actor* const boneActor = getWorld()->getActorById(pair.second);
		if (boneActor) {
			boneGobalTForm = rootInv * boneActor->getTransformMtx();
		} else {
			sgeAssert(false && "Expected alive object");
			boneGobalTForm = mat4f::getIdentity();
		}
	}
}

void TraitModel::getRenderItems(std::vector<IRenderItem*>& renderItems) {
	m_tempRenderItems.clear();

	const AssetModel* const assetModel = getAssetProperty().getAssetModel();
	mat4f node2world = getActor()->getTransformMtx();

	const EvaluatedModel* evalModel = m_evalModel.hasValue() ? &m_evalModel.get() : &assetModel->staticEval;

	if (evalModel) {
		int numNodes = evalModel->getNumEvalNodes();

		for (int iNode = 0; iNode < numNodes; ++iNode) {
			TraitModelRenderItem renderItem;

			const EvaluatedNode& evalNode = evalModel->getEvalNode(iNode);
			for (int iAttach = 0; iAttach < evalModel->m_model->nodeAt(iNode)->meshAttachments.size(); ++iAttach) {
				renderItem.bboxWs = evalNode.aabbGlobalSpace.getTransformed(node2world);
				renderItem.traitModel = this;
				renderItem.evalModel = evalModel;
				renderItem.iEvalNode = iNode;
				renderItem.iEvalNodeMechAttachmentIndex = iAttach;
				renderItem.needsAlphaSorting = getActor()->m_forceAlphaZSort;

				m_tempRenderItems.push_back(renderItem);
			}
		}
	}

	for (TraitModelRenderItem& ri : m_tempRenderItems) {
		renderItems.push_back(&ri);
	}
}

/// TraitModel Attribute Editor.
void editTraitStaticModel(GameInspector& inspector, GameObject* actor, MemberChain chain) {
	TraitModel& traitStaticModel = *(TraitModel*)chain.follow(actor);

	ImGuiEx::BeginGroupPanel("Model Trait");

	chain.add(sgeFindMember(TraitModel, m_assetProperty));
	ProperyEditorUIGen::doMemberUI(inspector, actor, chain);
	chain.pop();

	if (AssetModel* loadedAsset = traitStaticModel.m_assetProperty.getAssetModel()) {
		for (ModelMaterial* mtl : loadedAsset->model.getMatrials()) {
			// ImGui::Text(loadedAsset->model.m_materials[t]->name.c_str());
			// Check if override for this material already exists.
			auto itrExisting = find_if(traitStaticModel.m_materialOverrides,
			                           [&](const TraitModel::MaterialOverride& ovr) -> bool { return ovr.materialName == mtl->name; });

			if (itrExisting == std::end(traitStaticModel.m_materialOverrides)) {
				TraitModel::MaterialOverride ovr;
				ovr.materialName = mtl->name;
				traitStaticModel.m_materialOverrides.emplace_back(ovr);
			}
		}

		// Now do the UI for each available slot (keep in mind that we are keeping old slots (from previous models still available).

		for (int iMtl : rng_int(loadedAsset->model.numMaterials())) {
			const ImGuiEx::IDGuard guard(iMtl);

			const ModelMaterial* mtl = loadedAsset->model.materialAt(iMtl);

			// ImGui::Text(loadedAsset->model.m_materials[t]->name.c_str());
			// Check if override for this material already exists.
			auto itrExisting = find_if(traitStaticModel.m_materialOverrides,
			                           [&](const TraitModel::MaterialOverride& ovr) -> bool { return ovr.materialName == mtl->name; });

			if (itrExisting != std::end(traitStaticModel.m_materialOverrides)) {
				ImGuiEx::BeginGroupPanel(itrExisting->materialName.c_str(), ImVec2(-1.f, 0.f));
				{
					const int slotIndex = int(itrExisting - traitStaticModel.m_materialOverrides.begin());
					chain.add(sgeFindMember(TraitModel, m_materialOverrides), slotIndex);
					chain.add(sgeFindMember(TraitModel::MaterialOverride, materialObjId));
					ProperyEditorUIGen::doMemberUI(inspector, actor, chain);

					if (traitStaticModel.m_materialOverrides[slotIndex].materialObjId.isNull()) {
						if (ImGui::Button("Create New Material")) {
							CmdObjectCreation* cmdMtlCreate = new CmdObjectCreation();
							cmdMtlCreate->setup(sgeTypeId(MDiffuseMaterial));
							cmdMtlCreate->apply(&inspector);

							CmdMemberChange* const cmdAssignMaterial = new CmdMemberChange();
							const ObjectId originalValue = ObjectId();
							const ObjectId createdMaterialId = cmdMtlCreate->getCreatedObjectId();
							cmdAssignMaterial->setup(actor, chain, &originalValue, &createdMaterialId, nullptr);
							cmdAssignMaterial->apply(&inspector);

							CmdCompound* cmdCompound = new CmdCompound();
							cmdCompound->addCommand(cmdMtlCreate);
							cmdCompound->addCommand(cmdAssignMaterial);
							inspector.appendCommand(cmdCompound, false);
						}
					}

					chain.pop();
					chain.pop();

					if (GameObject* const materialObject = inspector.getWorld()->getObjectById(itrExisting->materialObjId)) {
						ImGui::PushID(materialObject);
						ImGuiEx::BeginGroupPanel("", ImVec2(-1.f, 0.f));
						if (ImGui::CollapsingHeader("Material")) {
							ProperyEditorUIGen::doGameObjectUI(inspector, materialObject);
						}
						ImGuiEx::EndGroupPanel();
						ImGui::PopID();
					}
				}
				ImGuiEx::EndGroupPanel();
			} else {
				sgeAssert(false && "The slot should have been created above.");
			}
		}

		if (ImGui::Button("Extract Skeleton")) {
			AssetModel* modelAsset = traitStaticModel.m_assetProperty.getAssetModel();
			GameWorld* world = inspector.getWorld();
			ALocator* allBonesParent = world->m_allocator<ALocator>();
			allBonesParent->setTransform(traitStaticModel.getActor()->getTransform());

			struct NodeRemapEl {
				transf3d localTransform;
				ABone* boneActor = nullptr;
			};
			vector_map<int, NodeRemapEl> nodeRemap;

			transf3d n2w = actor->getActor()->getTransform();

			for (int iNode : rng_int(modelAsset->model.numNodes())) {
				const ModelNode* node = modelAsset->model.nodeAt(iNode);

				const float boneLengthAuto = 0.1f;
				const float boneLength = node->limbLength > 0.f ? node->limbLength : boneLengthAuto;

				nodeRemap[iNode].localTransform = node->staticLocalTransform;
				nodeRemap[iNode].boneActor = inspector.getWorld()->m_allocator<ABone>(ObjectId(), node->name.c_str());
				nodeRemap[iNode].boneActor->boneLength = boneLength;
			}

			std::function<void(int nodeIndex, NodeRemapEl* parent)> traverseGlobalTransform;
			traverseGlobalTransform = [&](int nodeIndex, NodeRemapEl* parent) -> void {
				const ModelNode* node = modelAsset->model.nodeAt(nodeIndex);

				NodeRemapEl* const remapNode = nodeRemap.find_element(nodeIndex);
				if (parent) {
					remapNode->boneActor->setTransform(parent->boneActor->getTransform() * remapNode->localTransform);
					inspector.getWorld()->setParentOf(remapNode->boneActor->getId(), parent->boneActor->getId());
				} else {
					remapNode->localTransform = allBonesParent->getTransform() * remapNode->localTransform;
					remapNode->boneActor->setTransform(remapNode->localTransform);
					world->setParentOf(remapNode->boneActor->getId(), allBonesParent->getId());
				}

				for (const int childNode : node->childNodes) {
					traverseGlobalTransform(childNode, remapNode);
				}
			};


			traverseGlobalTransform(modelAsset->model.getRootNodeIndex(), nullptr);
		}

		chain.add(sgeFindMember(TraitModel, useSkeleton));
		ProperyEditorUIGen::doMemberUI(inspector, actor, chain);
		chain.pop();

		chain.add(sgeFindMember(TraitModel, rootSkeletonId));
		ProperyEditorUIGen::doMemberUI(inspector, actor, chain);
		chain.pop();
	}

	ImGuiEx::EndGroupPanel();
}

SgePluginOnLoad() {
	getEngineGlobal()->addPropertyEditorIUGeneratorForType(sgeTypeId(TraitModel), editTraitStaticModel);
}

} // namespace sge
