
#include "OutlinerWindow.h"
#include "IconsForkAwesome/IconsForkAwesome.h"
#include "sge_core/AssetLibrary.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GameInspector.h"
#include "sge_utils/utils/ScopeGuard.h"
#include "sge_utils/utils/strings.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui_internal.h"

#include "sge_core/SGEImGui.h"
#include "sge_engine/ui/ImGuiDragDrop.h"

namespace sge {

void OutlinerWindow::update(SGEContext* const UNUSED(sgecon), const InputState& UNUSED(is)) {
	const ImVec4 kPrimarySelectionColor(0.f, 1.f, 0.f, 1.f);

	if (isClosed()) {
		return;
	}

	if (ImGui::Begin(m_windowName.c_str(), &m_isOpened, ImGuiWindowFlags_MenuBar)) {
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("Display")) {
				if (ImGui::MenuItem("Object ids")) {
					m_displayObjectIds = !m_displayObjectIds;
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		ImGui::PushID("sge-id-to-avoid-collision");
		ScopeGuard sg([] { ImGui::PopID(); });

		ImGuiEx::Label(ICON_FK_BINOCULARS " Filter");
		nodeNamesFilter.Draw("##Filter");
		if (ImGui::IsItemClicked(2)) {
			ImGui::ClearActiveID(); // Hack: (if we do not make this call ImGui::InputText will set it's cached value.
			nodeNamesFilter.Clear();
		}

		GameWorld* const world = m_inspector.getWorld();

		ObjectId dragAndDropTargetedActor;
		std::set<ObjectId> droppedActorsOnTargetActor;

		ImGui::BeginChild("SceneObjectsTreeWindow");

		ImRect dropTargetRectForWindow;
		dropTargetRectForWindow.Min = ImGui::GetWindowPos() + ImGui::GetWindowContentRegionMin();
		dropTargetRectForWindow.Max = ImGui::GetWindowPos() + ImGui::GetWindowContentRegionMax();

		std::function<void(GameObject*, bool)> addChildObjects = [&](const GameObject* currentEntity, bool shouldIgnoreFiler) -> void {
			if (currentEntity == nullptr) {
				sgeAssert(false);
				return;
			}

			ImGuiTreeNodeFlags treeNodeFlags =
			    ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen;

			bool shouldShowChildren = true;
			bool passesFilter = shouldIgnoreFiler || nodeNamesFilter.PassFilter(currentEntity->getDisplayNameCStr());
			const vector_set<ObjectId>* pAllChildObjects = world->getChildensOf(currentEntity->getId());

			if (!pAllChildObjects || pAllChildObjects->empty()) {
				treeNodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
			}

			if (passesFilter) {
				bool isCurrrentNodePrimarySelection = false;
				bool isCurrentNodeSelected = m_inspector.isSelected(currentEntity->getId(), &isCurrrentNodePrimarySelection);
				if (isCurrentNodeSelected) {
					treeNodeFlags |= ImGuiTreeNodeFlags_Selected;
				}

				// Add the GUI elements itself.
				Texture* const iconTexture =
				    getEngineGlobal()->getEngineAssets().getIconForObjectType(currentEntity->getType())->asTextureView()->tex.GetPtr();

				ImGui::Image(iconTexture, ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize()));
				ImGui::SameLine();

				void* const treeNodeId = (void*)size_t(currentEntity->getId().id + 1); // Avoid having id 0 in the outliner

				if (isCurrrentNodePrimarySelection) {
					ImGui::PushStyleColor(ImGuiCol_Text, kPrimarySelectionColor);
				}

				bool isTreeNodeOpen = false;
				if (m_displayObjectIds) {
					std::string treeNodeName = string_format("%s [%d]", currentEntity->getDisplayNameCStr(), currentEntity->getId().id);
					isTreeNodeOpen = ImGui::TreeNodeEx(treeNodeId, treeNodeFlags, treeNodeName.c_str());
				} else {
					isTreeNodeOpen = ImGui::TreeNodeEx(treeNodeId, treeNodeFlags, currentEntity->getDisplayNameCStr());
				}

				shouldShowChildren = isTreeNodeOpen;

				if (isCurrrentNodePrimarySelection) {
					ImGui::PopStyleColor(1);
				}

				if (ImGui::IsMouseReleased(0) && ImGui::IsItemHovered(ImGuiHoveredFlags_None)) {
					if (ImGui::GetIO().KeyCtrl) {
						m_inspector.deselect(currentEntity->getId());
					} else if (ImGui::GetIO().KeyShift) {
						bool shouldSelectAsPrimary = m_inspector.isSelected(currentEntity->getId());
						m_inspector.select(currentEntity->getId(), shouldSelectAsPrimary);
					} else {
						m_inspector.deselectAll();
						m_inspector.select(currentEntity->getId());
					}
				}

				// Drag-and-drop support used for parenting/unparenting objects.
				// When the users start draging create a list of all selected nodes plus the one
				// that was used to initiate the dragging. When dropped these object are going to get parented to
				// something depending on where the user dropped them.
				if (ImGui::BeginDragDropSource()) {
					DragDropPayloadActor::setPayload(currentEntity->getId());

					ImGui::Text(currentEntity->getDisplayNameCStr());
					ImGui::EndDragDropSource();
				}

				// Handle dropping actors over another actor to parent it.
				// Do not do the parenting here as we are currently traversing the hierarchy and it will mess up the algorithm.
				// Save the data and do it once we've done iterating.
				if (ImGui::BeginDragDropTarget()) {
					if (Optional<std::set<ObjectId>> dropedIds = DragDropPayloadActor::accept()) {
						dragAndDropTargetedActor = currentEntity->getId();
						droppedActorsOnTargetActor = *dropedIds;
					}

					ImGui::EndDragDropTarget();
				}

				if (isCurrrentNodePrimarySelection) {
					ImGui::SameLine();
					ImGui::TextColored(kPrimarySelectionColor, "[Primery Selection]");
				}
			} else {
				shouldShowChildren = true;
			}

			// The the tree node is opened (by clicking the arrow) add the GUI for children objects in the hierarchy.
			if (shouldShowChildren) {
				if (pAllChildObjects && pAllChildObjects->empty() == false) {
					for (ObjectId childId : *pAllChildObjects) {
						GameObject* child = world->getObjectById(childId);
						addChildObjects(child, passesFilter);
					}

					if (passesFilter)
						ImGui::TreePop();
				}
			}
		};

		// Recursivley display tree nodes which represent the actors that are playing in the scene.
		m_inspector.getWorld()->iterateOverPlayingObjects(
		    [&](GameObject* object) -> bool {
			    if (world->getParentId(object->m_id).isNull()) {
				    addChildObjects(object, false);
			    }
			    return true;
		    },
		    false);

		// Drop over empty space in the outliner window means that the user wants to un-parent the selected objects.
		// Setting the window as a drag-and-drop target isn't currently supported in ImGui.
		// This is workaround was propoused by the author of the library.
		if (ImGui::BeginDragDropTargetCustom(dropTargetRectForWindow, 1234)) {
			if (Optional<std::set<ObjectId>> dropedIds = DragDropPayloadActor::accept()) {
				CmdActorGrouping* const cmd = new CmdActorGrouping;
				cmd->setup(*m_inspector.getWorld(), ObjectId(), dropedIds.get());
				m_inspector.appendCommand(cmd, true);
			}


			ImGui::EndDragDropTarget();
		}

		// Drop over a ImGui::TreeNode means that the user wants to parent the dragged actors under
		// the drop-target actor.
		if (!droppedActorsOnTargetActor.empty() && droppedActorsOnTargetActor.count(dragAndDropTargetedActor) == 0) {
			CmdActorGrouping* const cmd = new CmdActorGrouping;
			cmd->setup(*m_inspector.getWorld(), dragAndDropTargetedActor, droppedActorsOnTargetActor);
			m_inspector.appendCommand(cmd, true);
		}


		ImGui::EndChild();
	}
	ImGui::End();
}
} // namespace sge
