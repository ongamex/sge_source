#include "TraitModel.h"
#include "IconsForkAwesome/IconsForkAwesome.h"
#include "sge_engine/GameDrawer/RenderItems/TraitModelRenderItem.h"
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
#include "sge_utils/utils/strings.h"
#include "sge_utils/utils/vector_set.h"

namespace sge {

struct MDiffuseMaterial; // User for creating material overrides.

// clang-format off
DefineTypeId(TraitModel, 20'03'01'0004);
DefineTypeId(TraitModel::PerModelSettings, 21'07'11'0002);
DefineTypeId(std::vector<TraitModel::PerModelSettings>, 21'07'11'0003);
DefineTypeId(TraitModel::MaterialOverride, 20'10'15'0001);
DefineTypeId(std::vector<TraitModel::MaterialOverride>, 20'10'15'0002);

ReflBlock() {
	ReflAddType(TraitModel::MaterialOverride)
		ReflMember(TraitModel::MaterialOverride, materialName)
		ReflMember(TraitModel::MaterialOverride, materialObjId).setPrettyName("Material Object")
	;
	ReflAddType(std::vector<TraitModel::MaterialOverride>);

	ReflAddType(TraitModel::PerModelSettings)
		ReflMember(TraitModel::PerModelSettings, isRenderable)
		ReflMember(TraitModel::PerModelSettings, alphaMultiplier).uiRange(0.f, 1.f, 0.01f)
		ReflMember(TraitModel::PerModelSettings, m_assetProperty)
		ReflMember(TraitModel::PerModelSettings, m_materialOverrides)
		ReflMember(TraitModel::PerModelSettings, useSkeleton)
		ReflMember(TraitModel::PerModelSettings, rootSkeletonId)
	;
	ReflAddType(std::vector<TraitModel::PerModelSettings>);

	ReflAddType(TraitModel)
		ReflMember(TraitModel, isRenderable)
		ReflMember(TraitModel, m_models)
	;
}
// clang-format on

void TraitModel::PerModelSettings::setModel(const char* assetPath, bool updateNow) {
	m_assetProperty.setTargetAsset(assetPath);
	if (updateNow) {
		updateAssetProperty();
	}
}

void TraitModel::PerModelSettings::setModel(std::shared_ptr<Asset>& asset, bool updateNow) {
	m_assetProperty.setAsset(asset);
	m_evalModel = NullOptional();
	if (updateNow) {
		updateAssetProperty();
	}
}

//-----------------------------------------------------------------
// TraitModel::PerModelSettings
//-----------------------------------------------------------------
AABox3f TraitModel::PerModelSettings::getBBoxOS() const {
	// If the attached asset is a model use it to compute the bounding box.
	const AssetModel* const assetModel = m_assetProperty.getAssetModel();
	if (assetModel && assetModel->staticEval.isInitialized()) {
		AABox3f bbox = assetModel->staticEval.aabox.getTransformed(m_additionalTransform);
		return bbox;
	}

	return AABox3f();
}

void TraitModel::PerModelSettings::computeNodeToBoneIds(TraitModel& ownerTraitModel) {
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

	GameWorld* world = ownerTraitModel.getWorld();
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

void TraitModel::PerModelSettings::computeSkeleton(TraitModel& ownerTraitModel, std::vector<mat4f>& boneOverrides) {
	AssetModel* assetmodel = m_assetProperty.getAssetModel();
	if (assetmodel == nullptr) {
		return;
	}

	boneOverrides.resize(assetmodel->model.numNodes());

	if (useSkeleton == false) {
		return;
	}

	computeNodeToBoneIds(ownerTraitModel);
	Actor* root = ownerTraitModel.getWorld()->getActorById(rootSkeletonId);
	if (root == nullptr) {
		return;
	}

	mat4f rootInv = root ? root->getTransformMtx().inverse() : mat4f::getIdentity();

	for (auto pair : nodeToBoneId) {
		mat4f& boneGobalTForm = boneOverrides[pair.first];

		Actor* const boneActor = ownerTraitModel.getWorld()->getActorById(pair.second);
		if (boneActor) {
			boneGobalTForm = rootInv * boneActor->getTransformMtx();
		} else {
			sgeAssert(false && "Expected alive object");
			boneGobalTForm = mat4f::getIdentity();
		}
	}
}

void TraitModel::setModel(const char* assetPath, bool updateNow) {
	m_models.resize(1);

	m_models[0].m_evalModel = NullOptional();
	m_models[0].m_assetProperty.setTargetAsset(assetPath);
	if (updateNow) {
		postUpdate();
	}
}

void TraitModel::setModel(std::shared_ptr<Asset>& asset, bool updateNow) {
	m_models.resize(1);

	m_models[0].m_assetProperty.setAsset(asset);
	m_models[0].m_evalModel = NullOptional();
	if (updateNow) {
		postUpdate();
	}
}

void TraitModel::addModel(const char* assetPath, bool updateNow) {
	m_models.push_back(PerModelSettings());
	m_models.back().setModel(assetPath, updateNow);
}

void TraitModel::addModel(std::shared_ptr<Asset>& asset, bool updateNow) {
	m_models.push_back(PerModelSettings());
	m_models.back().setModel(asset, updateNow);
}

//-----------------------------------------------------------------
// TraitModel
//-----------------------------------------------------------------
AABox3f TraitModel::getBBoxOS() const {
	AABox3f bbox;

	for (const PerModelSettings& mdl : m_models) {
		bbox.expand(mdl.getBBoxOS());
	}

	return bbox;
}

void TraitModel::getRenderItems(std::vector<TraitModelRenderItem>& renderItems) {
	for (int iModel = 0; iModel < int(m_models.size()); iModel++) {
		PerModelSettings& modelSets = m_models[iModel];
		if (!modelSets.isRenderable) {
			continue;
		}

		const AssetModel* const assetModel = modelSets.m_assetProperty.getAssetModel();
		mat4f node2world = getActor()->getTransformMtx();

		const EvaluatedModel* evalModel = nullptr;
		if (modelSets.m_evalModel.hasValue()) {
			evalModel = &modelSets.m_evalModel.get();
		} else if (assetModel) {
			evalModel = &assetModel->staticEval;
		}

		if (evalModel) {
			int numNodes = evalModel->getNumEvalNodes();

			for (int iNode = 0; iNode < numNodes; ++iNode) {
				TraitModelRenderItem renderItem;

				const EvaluatedNode& evalNode = evalModel->getEvalNode(iNode);
				const ModelNode* node = evalModel->m_model->nodeAt(iNode);
				int numAttachments = int(node->meshAttachments.size());
				for (int iAttach = 0; iAttach < numAttachments; ++iAttach) {
					const EvaluatedMaterial& mtl = evalModel->getEvalMaterial(node->meshAttachments[iAttach].attachedMaterialIndex);
					Material material;
					
					material.alphaMultiplier = modelSets.alphaMultiplier * mtl.alphaMultiplier;

					material.diffuseColor = mtl.diffuseColor;
					material.metalness = mtl.metallic;
					material.roughness = mtl.roughness;

					material.diffuseTexture = isAssetLoaded(mtl.diffuseTexture) && mtl.diffuseTexture->asTextureView()
					                              ? mtl.diffuseTexture->asTextureView()->tex.GetPtr()
					                              : nullptr;

					material.texNormalMap = isAssetLoaded(mtl.texNormalMap) && mtl.texNormalMap->asTextureView()
					                            ? mtl.texNormalMap->asTextureView()->tex.GetPtr()
					                            : nullptr;

					material.texMetalness = isAssetLoaded(mtl.texMetallic) && mtl.texMetallic->asTextureView()
					                            ? mtl.texMetallic->asTextureView()->tex.GetPtr()
					                            : nullptr;

					material.texRoughness = isAssetLoaded(mtl.texRoughness) && mtl.texRoughness->asTextureView()
					                            ? mtl.texRoughness->asTextureView()->tex.GetPtr()
					                            : nullptr;


					renderItem.mtl = material;
					renderItem.zSortingPositionWs = mat_mul_pos(node2world, evalNode.aabbGlobalSpace.center());
					renderItem.traitModel = this;
					renderItem.evalModel = evalModel;
					renderItem.iModel = iModel;
					renderItem.iEvalNode = iNode;
					renderItem.iEvalNodeMechAttachmentIndex = iAttach;
					renderItem.needsAlphaSorting = getActor()->m_forceAlphaZSort || mtl.needsAlphaSorting || material.alphaMultiplier < 0.999f;

					renderItems.push_back(renderItem);
				}
			}
		}
	}
}

void TraitModel::invalidateCachedAssets() {
	for (PerModelSettings& modelSets : m_models) {
		modelSets.invalidateCachedAssets();
	}
}

bool TraitModel::updateAssetProperties() {
	bool hasChange = false;
	for (PerModelSettings& model : m_models) {
		hasChange |= model.updateAssetProperty();
	}

	return hasChange;
}

/// TraitModel Attribute Editor.
void editTraitModel(GameInspector& inspector, GameObject* actor, MemberChain chain) {
	TraitModel& traitModel = *(TraitModel*)chain.follow(actor);

	if (ImGui::CollapsingHeader(ICON_FK_CUBE " Model Trait", ImGuiTreeNodeFlags_DefaultOpen)) {
		int deleteModelIndex = -1;

		// Per model User Interface.
		for (int iModel = 0; iModel < traitModel.m_models.size(); ++iModel) {
			std::string label = string_format("Model %d", iModel);

			chain.add(sgeFindMember(TraitModel, m_models), iModel);
			TraitModel::PerModelSettings& modelSets = traitModel.m_models[iModel];

			const ImGuiEx::IDGuard idGuardImGui(&modelSets);
			ImGuiEx::BeginGroupPanel(label.c_str());
			if (ImGui::CollapsingHeader(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
				chain.add(sgeFindMember(TraitModel::PerModelSettings, isRenderable));
				ProperyEditorUIGen::doMemberUI(inspector, actor, chain);
				chain.pop();

				chain.add(sgeFindMember(TraitModel::PerModelSettings, alphaMultiplier));
				ProperyEditorUIGen::doMemberUI(inspector, actor, chain);
				chain.pop();

				chain.add(sgeFindMember(TraitModel::PerModelSettings, m_assetProperty));
				ProperyEditorUIGen::doMemberUI(inspector, actor, chain);
				chain.pop();

				// Material and Skeleton overrides interface.
				if (AssetModel* loadedAsset = modelSets.m_assetProperty.getAssetModel()) {
					for (ModelMaterial* mtl : loadedAsset->model.getMatrials()) {
						// ImGui::Text(loadedAsset->model.m_materials[t]->name.c_str());
						// Check if override for this material already exists.
						auto itrExisting = find_if(modelSets.m_materialOverrides, [&](const TraitModel::MaterialOverride& ovr) -> bool {
							return ovr.materialName == mtl->name;
						});

						if (itrExisting == std::end(modelSets.m_materialOverrides)) {
							TraitModel::MaterialOverride ovr;
							ovr.materialName = mtl->name;
							modelSets.m_materialOverrides.emplace_back(ovr);
						}
					}

					// Now do the UI for each available slot (keep in mind that we are keeping old slots (from previous models still
					// available).
					if (ImGui::CollapsingHeader(ICON_FK_PAINT_BRUSH " Material Override")) {
						for (int iMtl : rng_int(loadedAsset->model.numMaterials())) {
							const ImGuiEx::IDGuard guard(iMtl);

							const ModelMaterial* mtl = loadedAsset->model.materialAt(iMtl);

							// ImGui::Text(loadedAsset->model.m_materials[t]->name.c_str());
							// Check if override for this material already exists.
							auto itrExisting = find_if(modelSets.m_materialOverrides, [&](const TraitModel::MaterialOverride& ovr) -> bool {
								return ovr.materialName == mtl->name;
							});

							if (itrExisting != std::end(modelSets.m_materialOverrides)) {
								ImGuiEx::BeginGroupPanel(itrExisting->materialName.c_str(), ImVec2(-1.f, 0.f));
								{
									const int slotIndex = int(itrExisting - modelSets.m_materialOverrides.begin());
									chain.add(sgeFindMember(TraitModel::PerModelSettings, m_materialOverrides), slotIndex);
									chain.add(sgeFindMember(TraitModel::MaterialOverride, materialObjId));
									ProperyEditorUIGen::doMemberUI(inspector, actor, chain);

									if (modelSets.m_materialOverrides[slotIndex].materialObjId.isNull()) {
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

									if (GameObject* const materialObject =
									        inspector.getWorld()->getObjectById(itrExisting->materialObjId)) {
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
					}

					// Skeletion UI
					if (ImGui::CollapsingHeader(ICON_FK_WRENCH " Skeleton (WIP)")) {
						if (ImGui::Button("Extract Skeleton")) {
							AssetModel* modelAsset = modelSets.m_assetProperty.getAssetModel();
							GameWorld* world = inspector.getWorld();
							ALocator* allBonesParent = world->m_allocator<ALocator>();
							allBonesParent->setTransform(traitModel.getActor()->getTransform());

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


						chain.add(sgeFindMember(TraitModel::PerModelSettings, useSkeleton));
						ProperyEditorUIGen::doMemberUI(inspector, actor, chain);
						chain.pop();

						chain.add(sgeFindMember(TraitModel::PerModelSettings, rootSkeletonId));
						ProperyEditorUIGen::doMemberUI(inspector, actor, chain);
						chain.pop();
					}
				}

				// A button deleting the current model.
				if (traitModel.isFixedModelsSize == false) {
					if (ImGui::Button(ICON_FK_TRASH " Remove Model")) {
						deleteModelIndex = iModel;
					}
				}

				chain.pop(); // Pops chain.add(sgeFindMember(TraitModel, m_models), iModel);
			}

			ImGuiEx::EndGroupPanel();
		}

		// Handle adding/removing models.
		if (traitModel.isFixedModelsSize == false) {
			if (deleteModelIndex >= 0) {
				std::vector<TraitModel::PerModelSettings> oldModels = traitModel.m_models;
				std::vector<TraitModel::PerModelSettings> newModels = traitModel.m_models;
				newModels.erase(newModels.begin() + deleteModelIndex);

				chain.add(sgeFindMember(TraitModel, m_models));

				CmdMemberChange* const cmdDelElem = new CmdMemberChange();
				cmdDelElem->setup(actor, chain, &oldModels, &newModels, nullptr);
				cmdDelElem->apply(&inspector);
				inspector.appendCommand(cmdDelElem, false);

				chain.pop();
			}
		}

		if (ImGui::Button(ICON_FK_PLUS " Add Model")) {
			std::vector<TraitModel::PerModelSettings> oldModels = traitModel.m_models;
			std::vector<TraitModel::PerModelSettings> newModels = traitModel.m_models;
			newModels.push_back(TraitModel::PerModelSettings());

			chain.add(sgeFindMember(TraitModel, m_models));

			CmdMemberChange* const cmdNewModelElem = new CmdMemberChange();
			cmdNewModelElem->setup(actor, chain, &oldModels, &newModels, nullptr);
			cmdNewModelElem->apply(&inspector);
			inspector.appendCommand(cmdNewModelElem, false);

			chain.pop();
		}
	}
}


SgePluginOnLoad() {
	getEngineGlobal()->addPropertyEditorIUGeneratorForType(sgeTypeId(TraitModel), editTraitModel);
}

} // namespace sge
