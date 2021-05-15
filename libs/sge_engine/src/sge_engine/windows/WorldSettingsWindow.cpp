#include "WorldSettingsWindow.h"
#include "IconsForkAwesome/IconsForkAwesome.h"
#include "sge_core/SGEImGui.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GameInspector.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/UIAssetPicker.h"
#include "sge_engine/traits/TraitCamera.h"
#include "sge_utils/sge_utils.h"
#include "sge_utils/utils/strings.h"

namespace sge {
void WorldSettingsWindow::update(SGEContext* const UNUSED(sgecon), const InputState& UNUSED(is)) {
	if (isClosed()) {
		return;
	}

	if (ImGui::Begin(m_windowName.c_str(), &m_isOpened)) {
		GameWorld* world = m_inspector.getWorld();

		if (ImGui::CollapsingHeader(ICON_FK_CODE "  World Scripts")) {
			ImGui::Text("Add World Scripts objects to be executed.");
			std::string label;
			int indexToDelete = -1;
			for (int t = 0; t < int(world->m_scriptObjects.size()); ++t) {
				ImGui::PushID(t);

				string_format(label, "Script %d", t);
				actorPicker(label.c_str(), *world, world->m_scriptObjects[t], nullptr, true);

				if (ImGui::Button(ICON_FK_TRASH)) {
					indexToDelete = t;
				}

				ImGui::PopID();
			}

			if (indexToDelete >= 0) {
				world->m_scriptObjects.erase(world->m_scriptObjects.begin() + indexToDelete);
			}

			if (ImGui::Button(ICON_FK_PLUS " Add")) {
				world->m_scriptObjects.push_back(ObjectId());
			}
		}

		if (ImGui::CollapsingHeader(ICON_FK_LIGHTBULB_O " Scene Default Lighting")) {
			ImGui::ColorPicker3("Ambient Light", m_inspector.getWorld()->m_ambientLight.data);
			ImGui::ColorPicker3("Rim Light", m_inspector.getWorld()->m_rimLight.data);
			ImGui::DragFloat("Rim Width Cosine", &m_inspector.getWorld()->m_rimCosineWidth, 0.01f, 0.f, 1.f);
		}

		if (ImGui::CollapsingHeader(ICON_FK_SUN " Sky")) {
			ImGui::ColorPicker3("Sky Top Color", m_inspector.getWorld()->m_skyColorTop.data);
			ImGui::ColorPicker3("Sky Bottom Color", m_inspector.getWorld()->m_skyColorBottom.data);
		}

		if (ImGui::CollapsingHeader(ICON_FK_CAMERA " Gameplay")) {
			ImGui::InputInt(ICON_FK_CAMERA " Gameplay Camera", &m_inspector.getWorld()->m_cameraPovider.id);

			ImGui::SameLine();
			if (ImGui::Button(ICON_FK_SHOPPING_CART)) {
				ImGui::OpenPopup("Game Camera Picker");
			}

			if (ImGui::BeginPopup("Game Camera Picker")) {
				m_inspector.getWorld()->iterateOverPlayingObjects(
				    [&](const GameObject* object) -> bool {
					    if (getTrait<TraitCamera>(object)) {
						    bool selected = false;
						    if (ImGui::Selectable(object->getDisplayNameCStr(), &selected)) {
							    m_inspector.getWorld()->m_cameraPovider = object->getId();
						    }
					    }

					    return true;
				    },
				    false);

				ImGui::EndPopup();
			}
		}

		if (ImGui::CollapsingHeader(ICON_FK_CUBES " Physics")) {
			ImGuiEx::Label("Number of Physics Steps per Frame");
			ImGui::DragInt("##SPFPhysics", &world->m_physicsSimNumSubSteps, 0.1f, 1, 100, "%d", ImGuiSliderFlags_AlwaysClamp);

			ImGuiEx::Label("Default Gravity");
			if (ImGui::DragFloat3("##gravityDrag", world->m_defaultGravity.data)) {
				world->setDefaultGravity(world->m_defaultGravity);
			}
		}

		if (ImGui::CollapsingHeader(ICON_FK_FILTER " Grid")) {
			ImGuiEx::Label("Show Grid");
			ImGui::Checkbox("##ShowGirdCB", &world->gridShouldDraw);

			ImGuiEx::Label("Mark Every:");
			ImGui::DragFloat("##MarkEvery", &world->gridSegmentsSpacing);

			ImGuiEx::Label("Gird Segements:");
			ImGui::DragInt2("##Grid Segments", world->gridNumSegments.data);
		}

		if (ImGui::CollapsingHeader(ICON_FK_BUG " Debugging")) {
			ImGui::InputInt("MS Delay", &m_inspector.getWorld()->debug.forceSleepMs, 1, 10);
		}

		if (ImGui::CollapsingHeader(ICON_FK_MUSIC " Audio")) {
			ImGuiEx::Label("Master Volume:");
			ImGui::DragFloat("##MasterVolume", &world->m_masterVolume, 0.01f, 0.0f, 1.0f, "%.2f");
		}
	}
	ImGui::End();
}
} // namespace sge
