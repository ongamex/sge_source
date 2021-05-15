#include "sge_core/AssetLibrary.h"
#include "sge_core/ICore.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GameInspector.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "ActorCreateWindow.h"

namespace sge {
ActorCreateWindow::ActorCreateWindow(std::string windowName, GameInspector& inspector)
    : m_windowName(std::move(windowName))
    , m_inspector(inspector) {
}

void ActorCreateWindow::update(SGEContext* const UNUSED(sgecon), const InputState& UNUSED(is)) {
	const ImVec4 kPrimarySelectionColor(0.f, 1.f, 0.f, 1.f);

	if (isClosed()) {
		return;
	}

	if (ImGui::Begin(m_windowName.c_str(), &m_isOpened)) {
		const float kItemWidth = 32.f;
		const ImVec2 kWidgetSize = ImVec2(kItemWidth + 40.f, kItemWidth + 30.f);

		nodeNamesFilter.Draw();
		// Clear the filter if the filter text was clicked with the middle mouse button.
		if (ImGui::IsItemClicked(2)) {
			// HACK: there is apperently a bug in ImGui if we remove edit_state->ClearText();
			// the UI will automatically reapply its saved state into the input of the filter.
			// By exploring the ImGui code this workaround should work.
			ImGuiContext& g = *GImGui;
			ImGuiInputTextState* edit_state = &g.InputTextState;
			edit_state->ClearText();
			nodeNamesFilter.Clear();
		}

		{
			ImGui::BeginChild("Actor Create Buttons Window");

			// Find everything that inherits Actor and add a create function for it.
			int numItemsShown = 0;
			const float windowWidth = ImGui::GetContentRegionAvail().x;
			const int itemsPerRow = maxOf(int((windowWidth) / (kWidgetSize.x + ImGui::GetStyle().ItemSpacing.x * 2.f)), 1);
			for (const TypeId typeId : typeLib().m_gameObjectTypes) {
				const TypeDesc* typeDesc = typeLib().find(typeId);

				if (typeDesc == nullptr) {
					continue;
				}

				if (nodeNamesFilter.PassFilter(typeDesc->name) == false) {
					continue;
				}

				Texture* const icon =
				    getEngineGlobal()->getEngineAssets().getIconForObjectType(typeDesc->typeId)->asTextureView()->tex.GetPtr();

				ImGui::BeginChildFrame(ImHashStr(typeDesc->name), kWidgetSize, ImGuiWindowFlags_NoBackground);

				float indent = (ImGui::GetContentRegionAvailWidth() - kItemWidth) * 0.5f;
				ImGui::Indent(indent);
				ImGui::ImageButton(icon, ImVec2(32, 32), ImVec2(0.f, 0.f), ImVec2(1.f, 1.f), 0);
				bool isImageButtonPressed = ImGui::IsItemClicked(0);
				ImGui::Unindent(indent);

				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip(typeDesc->name);
				}

				// If the text is too long, do not center it below the icon, as it is hard to read the name of the type.
				const float typeNameTextWidth = ImGui::CalcTextSize(typeDesc->name).x;
				if (typeNameTextWidth >= kWidgetSize.x) {
					indent = 0.f;
				} else {
					indent = (ImGui::GetContentRegionAvailWidth() - ImGui::CalcTextSize(typeDesc->name).x) * 0.5f;
				}

				if (indent != 0.f) {
					ImGui::Indent(indent);
				}
				ImGui::Text(typeDesc->name);
				if (indent != 0.f) {
					ImGui::Unindent(indent);
				}

				if (isImageButtonPressed) {
					CmdObjectCreation* cmd = new CmdObjectCreation;
					cmd->setup(typeDesc->typeId);
					m_inspector.appendCommand(cmd, true);

					m_inspector.deselectAll();
					m_inspector.select(cmd->getCreatedObjectId());

					m_inspector.m_plantingTool.setup(m_inspector.getWorld()->getActorById(cmd->getCreatedObjectId()));
					m_inspector.setTool(&m_inspector.m_plantingTool);

					ImGui::FocusWindow(nullptr);
				}

				ImGui::EndChildFrame();

				numItemsShown++;
				if (numItemsShown == itemsPerRow) {
					numItemsShown = 0;
				} else {
					ImGui::SameLine();
				}
			}

			ImGui::EndChild();
		}
	}
	ImGui::End();
}

} // namespace sge
