#define _CRT_SECURE_NO_WARNINGS

#include "GameInspector.h"
#include "GameWorld.h"
#include "IconsForkAwesome/IconsForkAwesome.h"
#include "sge_core/ICore.h"
#include "sge_core/SGEImGui.h"
#include "sge_core/shaders/modeldraw.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/ui/ImGuiDragDrop.h"
#include "sge_utils/utils/strings.h"

#include "UIAssetPicker.h"

// imgui.h needs to be already included.
#define IM_VEC2_CLASS_EXTRA
#include <imgui/imconfig.h>

namespace sge {

bool assetPicker(
    const char* label, std::string& assetPath, AssetLibrary* const assetLibrary, const AssetType assetTypes[], const int numAssetTypes) {
	ImGuiEx::IDGuard idGuard(label);
	ImGuiEx::Label(label);

	if (ImGui::Button(ICON_FK_SHOPPING_CART)) {
		ImGui::OpenPopup("Asset Picker");
	}

	ImGui::SameLine();

	float inputTextWidth = ImGui::GetContentRegionAvailWidth();

	bool wasAssetPicked = false;
	ImGui::PushItemWidth(inputTextWidth);
	if (ImGuiEx::InputText("##Asset Picker Text", assetPath, ImGuiInputTextFlags_EnterReturnsTrue)) {
		wasAssetPicked = true;
	}

	if (ImGui::BeginDragDropTarget()) {
		if (Optional<std::string> droppedAssetPath = DragDropPayloadAsset::accept()) {
			assetPath = *droppedAssetPath;
			wasAssetPicked = true;
		}

		ImGui::EndDragDropTarget();
	}

	ImGui::PopItemWidth();

	if (ImGui::BeginPopup("Asset Picker")) {
		static ImGuiTextFilter filter;

		filter.Draw();
		if (ImGui::IsItemClicked(2)) {
			ImGui::ClearActiveID(); // Hack: (if we do not make this call ImGui::InputText will set it's cached value.
			filter.Clear();
		}

		int textureItemsInLine = 0;

		for (int iType = 0; iType < numAssetTypes; ++iType) {
			const AssetType assetType = assetTypes[iType];
			if (ImGui::BeginMenu(assetType_getName(assetType))) {
				for (auto& itr : assetLibrary->getAllAssets(assetType)) {
					if (!filter.PassFilter(itr.first.c_str())) {
						continue;
					}

					std::shared_ptr<Asset>& asset = itr.second;
					if (assetType == AssetType::Texture2D) {
						if (isAssetLoadFailed(asset) == false) {
							if (!isAssetLoaded(asset)) {
								if (ImGui::Button(itr.first.c_str(), ImVec2(48, 48))) {
									getCore()->getAssetLib()->getAsset(AssetType::Texture2D, asset->getPath().c_str(), true);
								}
							} else if (isAssetLoaded(asset)) {
								if (ImGui::ImageButton(asset->asTextureView()->tex.GetPtr(), ImVec2(48, 48))) {
									assetPath = itr.first;
									wasAssetPicked = true;
								}
							}

							if (ImGui::IsItemHovered()) {
								ImGui::BeginTooltip();
								ImGui::Text(itr.first.c_str());
								ImGui::EndTooltip();
							}

							textureItemsInLine++;

							if (textureItemsInLine == 8)
								textureItemsInLine = 0;
							else
								ImGui::SameLine();
						}
					} else if (assetType == AssetType::Model) {
						if (isAssetLoaded(asset)) {
							ImGui::Text(ICON_FK_CHECK);
						} else {
							ImGui::Text(ICON_FK_RECYCLE);
						}

						ImGui::SameLine();

						bool selected = itr.first == assetPath;
						if (ImGui::Selectable(itr.first.c_str(), &selected, ImGuiSelectableFlags_DontClosePopups)) {
							if (!isAssetLoaded(asset)) {
								getCore()->getAssetLib()->getAsset(AssetType::Model, asset->getPath().c_str(), true);
							} else {
								assetPath = itr.first;
								wasAssetPicked = true;
							}
						}

						if (ImGui::IsItemHovered() && isAssetLoaded(asset)) {
							ImGui::BeginTooltip();

							const int textureSize = 256;
							static float passedTime = 0.f;
							static GpuHandle<FrameTarget> frameTarget;

							passedTime += ImGui::GetIO().DeltaTime;

							if (!frameTarget) {
								frameTarget = getCore()->getDevice()->requestResource<FrameTarget>();
								frameTarget->create2D(textureSize, textureSize);
							}

							AssetModel* model = asset->asModel();

							getCore()->getDevice()->getContext()->clearColor(frameTarget, 0, vec4f(0.f).data);
							getCore()->getDevice()->getContext()->clearDepth(frameTarget, 1.f);

							const vec3f camPos =
							    mat_mul_pos(mat4f::getRotationY(passedTime * sge2Pi * 0.25f),
							                model->staticEval.aabox.halfDiagonal() * 1.66f + model->staticEval.aabox.center());
							const mat4f proj = mat4f::getPerspectiveFovRH(deg2rad(90.f), 1.f, 0.01f, 10000.f, kIsTexcoordStyleD3D);
							const mat4f lookAt = mat4f::getLookAtRH(camPos, vec3f(0.f), vec3f(0.f, kIsTexcoordStyleD3D ? 1.f : -1.f, 0.f));

							RenderDestination rdest(getCore()->getDevice()->getContext(), frameTarget);
							getCore()->getModelDraw().draw(rdest, camPos, -camPos.normalized0(), proj * lookAt, mat4f::getIdentity(),
							                               GeneralDrawMod(), model->staticEval, InstanceDrawMods());

							ImGui::Image(frameTarget->getRenderTarget(0), ImVec2(textureSize, textureSize));
							ImGui::EndTooltip();
						}

					} else {
						// Generic for all asset types.
						if (itr.first == assetPath) {
							ImGui::TextUnformatted(itr.first.c_str());
						} else {
							bool selected = false;
							if (ImGui::Selectable(itr.first.c_str(), &selected)) {
								assetPath = itr.first;
								wasAssetPicked = true;
							}
						}
					}
				}

				ImGui::EndMenu();
			}
		}
		ImGui::EndPopup();
	}

	return wasAssetPicked;
}

SGE_ENGINE_API bool assetPicker(
    const char* label, std::shared_ptr<Asset>& asset, AssetLibrary* const assetLibrary, const AssetType assetTypes[], const int numAssetTypes) {
	std::string tempPath = isAssetLoaded(asset) ? asset->getPath() : "";

	if (assetPicker(label, tempPath, assetLibrary, assetTypes, numAssetTypes)) {
		asset = assetLibrary->getAsset(tempPath.c_str(), true);
		return true;
	}

	return false;
}

bool actorPicker(
    const char* label, GameWorld& world, ObjectId& ioValue, std::function<bool(const GameObject&)> filter, bool pickPrimarySelection) {
	GameInspector* inspector = world.getInspector();

	if (!inspector) {
		return false;
	}

	if (ImGui::Button(ICON_FK_EYEDROPPER)) {
		ObjectId newPick;
		if (pickPrimarySelection) {
			newPick = inspector->getPrimarySelection();
		} else {
			newPick = inspector->getSecondarySelection();
		}

		if (newPick.isNull() == false) {
			if (filter) {
				GameObject* obj = world.getObjectById(ioValue);
				if (obj) {
					if (filter(*obj)) {
						ioValue = newPick;
					} else {
						getEngineGlobal()->showNotification("This object cannot be picked!");
					}
				}
			} else {
				ioValue = newPick;
			}


			return true;
		} else {
			if (pickPrimarySelection) {
				getEngineGlobal()->showNotification("Select an object to be picked!");
			} else {
				getEngineGlobal()->showNotification("Select a secondary object to be picked!");
			}
		}
	}

	if (pickPrimarySelection) {
		ImGuiEx::TextTooltip("Select an object and click this button to pick it!");
	} else {
		ImGuiEx::TextTooltip("Select a secondary object and click this button to pick it!");
	}
	ImGui::SameLine();

	const GameObject* const initalObject = world.getObjectById(ioValue);
	char currentActorName[256] = {'\0'};
	if (initalObject) {
		sge_strcpy(currentActorName, initalObject->getDisplayNameCStr());
	}

	if (ImGui::InputText(label, currentActorName, SGE_ARRSZ(currentActorName), ImGuiInputTextFlags_EnterReturnsTrue)) {
		const GameObject* const newlyAssignedObject = world.getObjectByName(currentActorName);
		if (newlyAssignedObject) {
			ioValue = newlyAssignedObject->getId();
		} else {
			ioValue = ObjectId();
		}
		return true;
	}

	if (ImGui::BeginDragDropTarget()) {
		bool accepted = false;
		if (Optional<ObjectId> droppedObjid = DragDropPayloadActor::acceptSingle()) {
			ioValue = *droppedObjid;
			accepted = true;
		}

		ImGui::EndDragDropTarget();
		if (accepted) {
			return true;
		}
	}

	return false;
}

SGE_ENGINE_API bool gameObjectTypePicker(const char* label, TypeId& ioValue, const TypeId needsToInherit) {
	ImGuiEx::IDGuard idGuard(label);
	ImGuiEx::Label(label);

	if (ImGui::Button(ICON_FK_SHOPPING_CART)) {
		ImGui::OpenPopup("Type Picker Popup");
	}

	ImGui::SameLine();

	float inputTextWidth = ImGui::GetContentRegionAvailWidth();

	bool wasAssetPicked = false;
	ImGui::PushItemWidth(inputTextWidth);

	const TypeDesc* initialType = typeLib().find(ioValue);

	static std::string currentTypeName;
	currentTypeName = initialType ? initialType->name : "<None>";
	if (ImGuiEx::InputText("##Asset Picker Text", currentTypeName, ImGuiInputTextFlags_EnterReturnsTrue)) {
		const TypeDesc* pickedType = typeLib().findByName(currentTypeName.c_str());
		if (pickedType != nullptr) {
			ioValue = pickedType->typeId;
			wasAssetPicked = true;
		} else {
			SGE_DEBUG_ERR("The speicifed type cannot be found!");
		}
	}

	ImGui::PopItemWidth();

	if (ImGui::BeginPopup("Type Picker Popup")) {
		static ImGuiTextFilter filter;
		filter.Draw();
		if (ImGui::IsItemClicked(2)) {
			ImGui::ClearActiveID(); // Hack: (if we do not make this call ImGui::InputText will set it's cached value.
			filter.Clear();
		}

		for (TypeId typeId : typeLib().m_gameObjectTypes) {
			const TypeDesc* potentialType = typeLib().find(typeId);
			if (potentialType == nullptr) {
				continue;
			}

			if (!needsToInherit.isNull() && !potentialType->doesInherits(needsToInherit)) {
				continue;
			}


			if (!filter.PassFilter(potentialType->name)) {
				continue;
			}

			Texture* const iconTexture =
			    getEngineGlobal()->getEngineAssets().getIconForObjectType(potentialType->typeId)->asTextureView()->tex.GetPtr();

			ImGui::Image(iconTexture, ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize()));
			ImGui::SameLine();

			if (ImGui::Selectable(potentialType->name)) {
				ioValue = potentialType->typeId;
				wasAssetPicked = true;
			}
		}
		ImGui::EndPopup();
	}


	return wasAssetPicked;
}


} // namespace sge
