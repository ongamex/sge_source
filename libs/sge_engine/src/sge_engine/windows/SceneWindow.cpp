#include "IconsForkAwesome/IconsForkAwesome.h"
#include "sge_core/AssetLibrary.h"
#include "sge_core/ICore.h"
#include "sge_core/QuickDraw.h"
#include "sge_core/SGEImGui.h"
#include "sge_engine/Camera.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GameDrawer.h"
#include "sge_engine/GameInspector.h"
#include "sge_utils/math/color.h"
#include "sge_utils/utils/Wildcard.h"

#include "SceneWindow.h"
#include "sge_engine/EngineGlobal.h"

namespace sge {

void SceneWindow::update(SGEContext* const sgecon, const InputState& isOriginal) {
	if (m_gameDrawer == nullptr) {
		return;
	}

	GameWorld* const world = m_gameDrawer->getWorld();
	GameInspector* const inspector = world->getInspector();

	InputState is = isOriginal;
	bool wasWindowShown = false;
	if (ImGui::Begin(m_windowName.c_str(), nullptr)) {
		wasWindowShown = true;
		// Make sure that the frame target has the currect size.
		m_canvasPos = fromImGui(ImGui::GetCursorScreenPos());
		m_canvasSize = fromImGui(ImGui::GetContentRegionAvail());

		// Keep the canvas some minimal size, for the sole purpouse of not handling situations
		// where the the window has 0 width or height in the rendering.
		m_canvasSize.x = maxOf(m_canvasSize.x, 64.f);
		m_canvasSize.y = maxOf(m_canvasSize.y, 64.f);

		is = computeInputStateInDomainSpace(isOriginal);

		is.setDomainFromPosAndSize(m_canvasPos, m_canvasSize);

		is.m_wasActiveWhilePolling =
		    ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem |
		                           ImGuiHoveredFlags_RootWindow | ImGuiHoveredFlags_ChildWindows);

		const bool isFrameTargetCorrect =
		    m_frameTarget.IsResourceValid() && m_frameTarget->getWidth() == m_canvasSize.x && m_frameTarget->getHeight() == m_canvasSize.y;

		if (isFrameTargetCorrect == false) {
			if (!m_frameTarget.HasResource()) {
				m_frameTarget = sgecon->getDevice()->requestResource<FrameTarget>();
			}
			m_frameTarget->create2D(int(m_canvasSize.x), int(m_canvasSize.y));
		}

		sgeAssert(m_frameTarget.IsResourceValid());
		sgecon->clearDepth(m_frameTarget, 1.f);
		float const bgColor[] = {0.f, 0.f, 0.f, 1.f};
		sgecon->clearColor(m_frameTarget, -1, bgColor);

		// Update the view aspect ratio.
		world->userProjectionSettings.canvasSize = vec2i(int(m_canvasSize.x), int(m_canvasSize.y));
		world->userProjectionSettings.aspectRatio = (float)m_canvasSize.x / (float)m_canvasSize.y;

		// Intilize the game draw settings.
		ICamera* const gameCamera = world->getRenderCamera();

		GameDrawSets drawSets;
		drawSets.rdest = RenderDestination(sgecon, m_frameTarget);
		drawSets.quickDraw = &getCore()->getQuickDraw();
		drawSets.drawCamera = gameCamera;
		drawSets.gameCamera = gameCamera;
		drawSets.gameDrawer = m_gameDrawer;


		// Draw.
		m_gameDrawer->prepareForNewFrame();
		m_gameDrawer->updateShadowMaps(drawSets);

		//  TODO: Caution:
		// This was reset in the update shadow maps.
		drawSets.rdest = RenderDestination(sgecon, m_frameTarget);
		drawSets.quickDraw = &getCore()->getQuickDraw();
		drawSets.drawCamera = gameCamera;
		drawSets.gameCamera = gameCamera;

		if (world->gridShouldDraw) {
			drawSets.quickDraw->drawWired_Clear();
			drawSets.quickDraw->drawWiredAdd_Grid(vec3f(0.f), vec3f::getAxis(0, world->gridSegmentsSpacing),
			                                      vec3f::getAxis(2, world->gridSegmentsSpacing), world->gridNumSegments.x,
			                                      world->gridNumSegments.y, 0xFF888888);
			drawSets.quickDraw->drawWired_Execute(drawSets.rdest, drawSets.drawCamera->getProjView(),
			                                      getCore()->getGraphicsResources().BS_backToFrontAlpha);
		}

		m_gameDrawer->drawWorld(drawSets, world->m_useEditorCamera ? drawReason_editing : drawReason_gameplay);

		if (world->isInEditMode()) {
			drawOverlay(drawSets);
			updateToolsAndOverlay(is, drawSets);
		}

		// Keyboard shortcuts.
		if (ImGui::IsWindowFocused()) {
			if (is.IsKeyPressed(Key::Key_F3)) {
				inspector->m_disableAutoStepping = !inspector->m_disableAutoStepping;
			}

			world->m_useEditorCamera = !getEngineGlobal()->getEngineAllowingRelativeCursor();

			if (is.IsKeyReleased(Key::Key_F1)) {
				world->toggleEditMode();
			}

			if (is.IsKeyPressed(Key::Key_F) || (is.IsKeyDown(Key::Key_Space))) {
				// TODO: Fix selection.
				inspector->focusOnSelection();
			}

			if (is.isKeyCombo(Key::Key_LCtrl, Key::Key_G)) {
				if (inspector->getSelection().size() > 1) {
					std::set<ObjectId> potentialChildren;

					for (int t = 1; t < inspector->getSelection().size(); ++t) {
						const SelectedItem& sel = inspector->getSelection()[t];
						if (sel.editMode == editMode_actors) {
							potentialChildren.insert(sel.objectId);
						}
					}
					const ObjectId parentId = inspector->getSelection()[0].objectId;

					CmdActorGrouping* cmd = new CmdActorGrouping();
					cmd->setup(*world, parentId, potentialChildren);

					inspector->appendCommand(cmd, true);
				}
			}

			if (is.isKeyCombo(Key::Key_LCtrl, Key::Key_U)) {
				// TODO: add a command for this.
				for (const SelectedItem& sel : inspector->getSelection()) {
					if (sel.editMode == editMode_actors) {
						world->setParentOf(sel.objectId, ObjectId());
					}
				}

				getEngineGlobal()->showNotification("All selected objects are unparanted!");
			}

			if (is.isKeyCombo(Key::Key_LCtrl, Key::Key_D)) {
				inspector->duplicateSelection();
			}
		}

		// Display the rendered image to the ImGui window.
		// ImDrawList* draw_list = ImGui::GetWindowDrawList();
		if (kIsTexcoordStyleD3D) {
			ImGui::Image(m_frameTarget->getRenderTarget(0), ImVec2(m_canvasSize.x, m_canvasSize.y));
		} else {
			ImGui::Image(m_frameTarget->getRenderTarget(0), ImVec2(m_canvasSize.x, m_canvasSize.y), ImVec2(0, 1), ImVec2(1, 0));
		}
	}
	ImGui::End();

	if (inspector->m_toolInAction) {
		if (ImGui::Begin("Tool Settings")) {
			inspector->m_toolInAction->onUI(inspector);
		}
		ImGui::End();
	}
}

void SceneWindow::updateRightClickMenu(bool canOpen) {
	if (m_gameDrawer == nullptr) {
		return;
	}

	GameWorld* const world = m_gameDrawer->getWorld();
	GameInspector* const inspector = world->getInspector();

	const char* const kRightClickMenuName = "SGE Inspector Right Click Menu Name";

	if ((ImGui::IsKeyPressed(ImGuiKey_Tab, false) || ImGui::IsMouseClicked(1)) && canOpen) {
		ImGui::OpenPopup(kRightClickMenuName);
	}

	ImGui::SetNextWindowPos(ImGui::GetMousePos(), ImGuiCond_Appearing);
	;
	if (ImGui::BeginPopup(kRightClickMenuName)) {
		static bool wasMenuShowPrevFrame = false;

		static std::vector<std::string> levelsList;

		// If the menu has just been opened and tab is down (the manu can be opened by pressing tab)
		// Then automatically open the create menu.
		if (ImGui::IsWindowAppearing() && ImGui::IsKeyPressed(ImGuiKey_Tab, false)) {
			ImGui::OpenPopup("Create");
		}

		if (ImGui::BeginMenu("Create")) {
			if (wasMenuShowPrevFrame == false /* || ImGui::IsWindowAppearing() && ImGui::IsKeyPressed(ImGuiKey_Tab)*/) {
				ImGui::SetKeyboardFocusHere();
			}

			static ImGuiTextFilter createActorFilter;

			createActorFilter.Draw(ICON_FK_SEARCH);

			if (ImGui::IsItemClicked(2)) {
				ImGui::ClearActiveID(); // Hack: (if we do not make this call ImGui::InputText will set it's cached value.
				createActorFilter.Clear();
			}

			// Find everything that inherits GameObject and add a create function for it.
			for (const TypeId& typeId : typeLib().m_gameObjectTypes) {
				const TypeDesc* td = typeLib().find(typeId);

				if (td == nullptr) {
					continue;
				}

				if (createActorFilter.PassFilter(td->name)) {
					Texture* const iconTexture =
					    getEngineGlobal()->getEngineAssets().getIconForObjectType(td->typeId)->asTextureView()->tex.GetPtr();

					ImGui::Image(iconTexture, ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()));
					ImGui::SameLine();

					if (ImGui::MenuItem(td->name)) {
						CmdObjectCreation* cmd = new CmdObjectCreation;
						cmd->setup(td->typeId);
						inspector->appendCommand(cmd, true);
					}
				}
			}

			wasMenuShowPrevFrame = true;
			ImGui::EndMenu();
		} else {
			wasMenuShowPrevFrame = false;
		}

		if (ImGui::BeginMenu("Edit Mode")) {
			if (ImGui::MenuItem("Actor Mode", nullptr, inspector->editMode == editMode_actors, inspector->editMode != editMode_actors))
				inspector->editMode = editMode_actors;
			if (ImGui::MenuItem("Point Mode", nullptr, inspector->editMode == editMode_points, inspector->editMode != editMode_points))
				inspector->editMode = editMode_points;

			ImGui::EndMenu();
		}

		ImGui::Separator();

		if (ImGui::MenuItem(ICON_FK_UNDO " Undo")) {
			inspector->undoCommand();
		}

		if (ImGui::MenuItem(ICON_FK_GAVEL " Redo")) {
			inspector->redoCommand();
		}

		if (ImGui::MenuItem(ICON_FK_TRASH " Delete")) {
			inspector->deleteSelection(false);
		}

		if (ImGui::MenuItem(ICON_FK_FILES_O " Duplicate")) {
			inspector->duplicateSelection();
		}

		ImGui::Separator();


		ImGui::Checkbox("Physics Debug Draw", &inspector->m_physicsDebugDrawEnabled);

		ImGui::Separator();

		ImGui::EndPopup();
	}
}

bool SceneWindow::updateToolsAndOverlay(const InputState& is, const GameDrawSets& drawSets) {
	if (m_gameDrawer == nullptr) {
		return false;
	}

	GameWorld* const world = m_gameDrawer->getWorld();
	GameInspector* const inspector = world->getInspector();

	bool userInteractedWithCamera = false;
	if (world->getRenderCamera() == &world->m_editorCamera) {
		userInteractedWithCamera = world->m_editorCamera.update(is, drawSets.rdest.viewport.ratioWbyH());
	}

	bool const canInteractWithViewport = !userInteractedWithCamera && is.wasActiveWhilePolling();

	if (canInteractWithViewport) {
		if (is.IsKeyReleased(Key::Key_Q)) {
			inspector->setTool(&inspector->m_selectionTool);
		}
		if (is.IsKeyReleased(Key::Key_W)) {
			inspector->m_transformTool.m_mode = Gizmo3D::Mode_Translation;
			inspector->setTool(&inspector->m_transformTool);
		}
		if (is.IsKeyReleased(Key::Key_E)) {
			inspector->m_transformTool.m_mode = Gizmo3D::Mode_Rotation;
			inspector->setTool(&inspector->m_transformTool);
		}
		if (is.isKeyCombo(Key::Key_LShift, Key::Key_R)) {
			inspector->m_transformTool.m_mode = Gizmo3D::Mode_ScaleVolume;
			inspector->setTool(&inspector->m_transformTool);
		} else if (is.IsKeyReleased(Key::Key_R)) {
			inspector->m_transformTool.m_mode = Gizmo3D::Mode_Scaling;
			inspector->setTool(&inspector->m_transformTool);
		}

		if (is.isKeyCombo(Key::Key_LShift, Key::Key_T)) {
			vector_set<ObjectId> newObjects;
			inspector->duplicateSelection(&newObjects);
			if (inspector->m_selection.size() > 0) {
				inspector->m_plantingTool.setup(newObjects, *inspector->getWorld());
				inspector->setTool(&inspector->m_plantingTool);
			} else {
				getEngineGlobal()->showNotification("Make a selection first before using the planting tool!");
			}
		} else if (is.IsKeyReleased(Key::Key_T)) {
			vector_set<ObjectId> allSelectedObjects;
			inspector->getAllSelectedObjects(allSelectedObjects);
			if (allSelectedObjects.empty() == false) {
				inspector->m_plantingTool.setup(allSelectedObjects, *inspector->getWorld());
				inspector->setTool(&inspector->m_plantingTool);
			} else {
				getEngineGlobal()->showNotification("Make a selection first before using the planting tool!");
			}
		}
	}

	// Update the viewport tool.
	if (!inspector->m_toolInAction) {
		inspector->m_toolInAction = &inspector->m_selectionTool;
	} else if (inspector->m_toolInAction) {
		InspectorToolResult res = inspector->m_toolInAction->updateTool(inspector, canInteractWithViewport, is, drawSets);

		if (canInteractWithViewport) {
			if (res.reapply) {
				inspector->setTool(inspector->m_toolInAction);
			} else if (res.propagateInput) {
				inspector->setTool(&inspector->m_selectionTool);
			}
		}

		if (res.isDone) {
			inspector->setTool(&inspector->m_selectionTool);
		}
	}

	updateRightClickMenu(canInteractWithViewport);

	return canInteractWithViewport;
}

void SceneWindow::drawOverlay(const GameDrawSets& drawSets) {
	if (m_gameDrawer == nullptr) {
		return;
	}

	GameWorld* const world = m_gameDrawer->getWorld();
	GameInspector* const inspector = world->getInspector();
	SGEContext* const sgecon = drawSets.rdest.sgecon;

	// HACK:
	if (!sgecon) {
		return;
	}

	// Draw the selected objects.
	for (int t = 0; t < inspector->m_selection.size(); ++t) {
		Actor* const actor = inspector->m_world->getActorById(inspector->m_selection[t].objectId);
		if (actor != nullptr) {
			drawSets.gameDrawer->drawActor(drawSets, inspector->editMode, actor, inspector->m_selection[t].index,
			                               t == 0 ? drawReason_visualizeSelectionPrimary : drawReason_visualizeSelection);
		}
	}

	// Draw the selection rect.
	sgecon->clearDepth(drawSets.rdest.frameTarget, 1.f);

	if (inspector->m_toolInAction) {
		inspector->m_toolInAction->drawOverlay(drawSets);
	}
}

InputState SceneWindow::computeInputStateInDomainSpace(const InputState& isOriginal) const {
	// Compute the input state according ot the position of this window.
	InputState is = isOriginal;
	is.setDomainFromPosAndSize(m_canvasPos, m_canvasSize);

	return is;
}


} // namespace sge
