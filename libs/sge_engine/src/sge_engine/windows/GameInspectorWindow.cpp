#include "GameInspectorWindow.h"
#include "IconsForkAwesome/IconsForkAwesome.h"
#include "sge_core/SGEImGui.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GameInspector.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/traits/TraitCamera.h"
#include "sge_utils/sge_utils.h"
#include "sge_utils/utils/strings.h"

namespace sge {
void GameInspectorWindow::update(SGEContext* const UNUSED(sgecon), const InputState& UNUSED(is)) {
	if (isClosed()) {
		return;
	}

	if (ImGui::Begin(m_windowName.c_str(), &m_isOpened)) {
		if (m_inspector.getWorld()->m_workingFilePath.empty()) {
			ImGui::TextUnformatted("File not saved...");
		} else {
			ImGui::TextUnformatted(m_inspector.getWorld()->m_workingFilePath.c_str());
		}

		GameWorld* world = m_inspector.getWorld();

		if (ImGui::Button("Undo")) {
			m_inspector.undoCommand();
		}

		ImGui::SameLine();

		if (ImGui::Button("Redo")) {
			m_inspector.redoCommand();
		}

		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();

		if (ImGui::Button("Delete") && m_inspector.hasSelection()) {
			m_inspector.deleteSelection(false);
		}

		if (ImGui::Button("Delete Hierarchy") && m_inspector.hasSelection()) {
			m_inspector.deleteSelection(true);
		}

		if (ImGui::CollapsingHeader(ICON_FK_CODE " Game World Scripts")) {
			std::string label;
			std::string currentObjectName;
			int indexToDelete = -1;
			for (int t = 0; t < int(world->m_scriptObjects.size()); ++t) {
				GameObject* const go = world->getObjectById(world->m_scriptObjects[t]);

				if (go) {
					currentObjectName = go->getDisplayName();
				} else {
					currentObjectName = "<not-assigned-object>";
				}

				ImGui::PushID(t);
				string_format(label, "Object %d", t);
				ImGuiEx::Label(label.c_str(), false);

				if (ImGuiEx::InputText("##ObjectName", currentObjectName)) {
					GameObject* newObj = world->getObjectByName(currentObjectName.c_str());
					if (newObj) {
						world->m_scriptObjects[t] = newObj->getId();
					}
				}

				if (ImGui::Button(ICON_FK_EYEDROPPER)) {
					auto& selection = m_inspector.getSelection();
					if (selection.size() >= 1) {
						GameObject* newObj = world->getObjectById(selection[0].objectId);
						if (newObj) {
							world->m_scriptObjects[t] = newObj->getId();
						}
					}
				}

				ImGui::SameLine();
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

		if (ImGui::CollapsingHeader("Lighting")) {
			ImGui::ColorPicker3("Ambient Light", m_inspector.getWorld()->m_ambientLight.data);
			ImGui::ColorPicker3("Rim Light", m_inspector.getWorld()->m_rimLight.data);
			ImGui::DragFloat("Rim Width Cosine", &m_inspector.getWorld()->m_rimCosineWidth, 0.01f, 0.f, 1.f);
		}

		if (ImGui::CollapsingHeader("Sky")) {
			ImGui::ColorPicker3("Sky Top Color", m_inspector.getWorld()->m_skyColorTop.data);
			ImGui::ColorPicker3("Sky Bottom Color", m_inspector.getWorld()->m_skyColorBottom.data);
		}

		if (ImGui::CollapsingHeader("Game Camera")) {
			ImGui::InputInt("Playing Camera", &m_inspector.getWorld()->m_cameraPovider.id);
			ImGui::SameLine();
			if (ImGui::Button("Pick")) {
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

		// History.
		if (ImGui::CollapsingHeader("History")) {
			auto commandsNamesGetter = [](void* data, int idx, const char** text) -> bool {
				GameInspector* const inspector = (GameInspector*)data;

				static std::string commandText;

				int const backwardsIdx = int(inspector->m_commandHistory.size()) - idx - 1;

				if (backwardsIdx >= 0 && backwardsIdx < int(inspector->m_commandHistory.size())) {
					inspector->m_commandHistory[backwardsIdx]->getText(commandText);
					*text = commandText.c_str();
					return false;
				}

				return true;
			};

			// Display a list of all commands.
			int curr = 0;
			ImGui::ListBox("Cmd History", &curr, commandsNamesGetter, &m_inspector, m_inspector.m_lastExecutedCommandIdx + 1, 5);
		}

		// Stepping.
		ImGui::Separator();
		{
			ImGui::Checkbox("No Auto Step", &m_inspector.m_disableAutoStepping);

			m_inspector.m_stepOnce = false;
			if (ImGui::Button("Step Once")) {
				m_inspector.m_disableAutoStepping = true;
				m_inspector.m_stepOnce = true;
			}

			ImGui::Text("Steps taken %d", m_inspector.m_world->totalStepsTaken);
		}

		// Hierarchical relationships.
		if (ImGui::CollapsingHeader("Hierarchy")) {
			static int child;
			static int parent;
			ImGui::InputInt("child", &child);
			ImGui::InputInt("parent", &parent);

			if (ImGui::Button("Set Parent!")) {
				m_inspector.getWorld()->setParentOf(ObjectId(child), ObjectId(parent));
			}
		}

		ImGui::InputInt("MS Delay", &m_inspector.getWorld()->debug.forceSleepMs, 1, 10);
	}
	ImGui::End();
}
} // namespace sge
