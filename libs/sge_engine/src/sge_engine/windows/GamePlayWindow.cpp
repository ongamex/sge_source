#include "sge_core/ICore.h"
#include "sge_core/QuickDraw.h"
#include "sge_core/SGEImGui.h"
#include "sge_engine/Camera.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GameInspector.h"
#include "sge_engine/GameSerialization.h"
#include "sge_utils/utils/Wildcard.h"

#include "sge_core/GameUI/UIContext.h"

#include "GamePlayWindow.h"

namespace sge {

GamePlayWindow::GamePlayWindow(std::string windowName, const char* const worldJsonString)
    : m_windowName(std::move(windowName))
    , m_isOpened(true) {
	m_gameDrawer.reset(getEngineGlobal()->getActivePlugin()->allocateGameDrawer());

	m_sceneInstance.newScene();
	m_gameDrawer->initialize(&m_sceneInstance.getWorld());

	m_sceneInstance.loadWorldFromJson(worldJsonString, false, "");

	m_sceneInstance.getWorld().m_useEditorCamera = false;
	m_sceneInstance.getWorld().isEdited = false;
}

void GamePlayWindow::update(SGEContext* const sgecon, const InputState& isOriginal) {
	m_timer.tick();

	if (isClosed()) {
		return;
	}

	GameWorld* const world = m_gameDrawer->getWorld();
	GameDrawSets drawSets;

	InputState is = isOriginal;
	bool wasWindowShown = false;
	if (ImGui::Begin(m_windowName.c_str(), &m_isOpened)) {
		wasWindowShown = true;
		// Make sure that the frame target has the currect size.
		vec2f canvasPos = fromImGui(ImGui::GetCursorScreenPos());
		vec2f canvasSize = fromImGui(ImGui::GetContentRegionAvail());

		// Keep the canvas some minimal size, for the sole purpouse of not handling situations
		// where the the window has 0 width or height in the rendering.
		canvasSize.x = maxOf(canvasSize.x, 64.f);
		canvasSize.y = maxOf(canvasSize.y, 64.f);

		// Compute the input state according ot the position of this window.
		is.m_cursorDomain = is.m_cursorClient - canvasPos;
		is.m_cursorDomainSize = canvasSize;

		is.m_wasActiveWhilePolling =
		    ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem |
		                           ImGuiHoveredFlags_RootWindow | ImGuiHoveredFlags_ChildWindows);

		const bool isFrameTargetCorrect =
		    m_frameTarget.IsResourceValid() && m_frameTarget->getWidth() == canvasSize.x && m_frameTarget->getHeight() == canvasSize.y;

		if (isFrameTargetCorrect == false) {
			if (!m_frameTarget.HasResource()) {
				m_frameTarget = sgecon->getDevice()->requestResource<FrameTarget>();
			}
			m_frameTarget->create2D(int(canvasSize.x), int(canvasSize.y));
		}

		sgeAssert(m_frameTarget.IsResourceValid());
		sgecon->clearDepth(m_frameTarget, 1.f);
		float const bgColor[] = {0.f, 0.f, 0.f, 1.f};
		sgecon->clearColor(m_frameTarget, -1, bgColor);

		// Update the view aspect ratio.
		world->userProjectionSettings.canvasSize = vec2i(int(canvasSize.x), int(canvasSize.y));
		world->userProjectionSettings.aspectRatio = (float)canvasSize.x / (float)canvasSize.y;

		// Update the game world.
		m_sceneInstance.update(m_timer.diff_seconds(), is);
		sgeAssert(world->isInEditMode() == false);

		// Intilize the game draw settings.
		ICamera* const gameCamera = world->getRenderCamera();

		drawSets.rdest = RenderDestination(sgecon, m_frameTarget);
		drawSets.quickDraw = &getCore()->getQuickDraw();
		drawSets.drawCamera = gameCamera;
		drawSets.gameCamera = gameCamera;

		getCore()->getQuickDraw().drawWired_Clear();
		getCore()->getQuickDraw().drawWiredAdd_Grid(vec3f(0.f), vec3f::getAxis(0) * 1.f, vec3f::getAxis(2) * 1.f, 10, 10, 0x33FFFFFF);
		getCore()->getQuickDraw().drawWired_Execute(drawSets.rdest, drawSets.drawCamera->getProjView(),
		                                            getCore()->getGraphicsResources().BS_backToFrontAlpha);

		// Draw the game world.
		m_gameDrawer->prepareForNewFrame();
		m_gameDrawer->updateShadowMaps(drawSets);
		m_gameDrawer->drawWorld(drawSets, world->m_useEditorCamera ? drawReason_editing : drawReason_gameplay);

		const ImVec2 imGuiCursorPreGameplayFrame = ImGui::GetCursorPos();

		// Display the rendered image to the ImGui window.
		if (kIsTexcoordStyleD3D)
			ImGui::Image(m_frameTarget->getRenderTarget(0), ImVec2(canvasSize.x, canvasSize.y));
		else
			ImGui::Image(m_frameTarget->getRenderTarget(0), ImVec2(canvasSize.x, canvasSize.y), ImVec2(0, 1), ImVec2(1, 0));

		// [SGE_EDITOR_MOUSE_CAPTURE_HOTKEY]
		// Draw a message telling the user that he can capture/uncapture mouse with F2.
		ImGui::SetCursorPos(imGuiCursorPreGameplayFrame);
		ImGui::Text("Press F2 to toggle mouse capture.");
	}
	ImGui::End();
}

} // namespace sge
