#include <filesystem>

#include "ActorCreateWindow.h"
#include "AssetsWindow.h"
#include "CreditsWindow.h"
#include "EditorWindow.h"
#include "GameInspectorWindow.h"
#include "GamePlayWindow.h"
#include "HelpWindow.h"
#include "InfoWindow.h"
#include "LogWindow.h"
#include "ModelPreviewWindow.h"
#include "OutlinerWindow.h"
#include "PrefabWindow.h"
#include "ProjectSettingsWindow.h"
#include "PropertyEditorWindow.h"
#include "SceneWindow.h"
#include "WorldSettingsWindow.h"
#include "sge_core/ICore.h"
#include "sge_core/QuickDraw.h"
#include "sge_core/SGEImGui.h"
#include "sge_core/application/application.h"
#include "sge_core/application/input.h"
#include "sge_core/ui/MultiCurve2DEditor.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GameDrawer/GameDrawer.h"
#include "sge_engine/GameSerialization.h"
#include "sge_engine/actors/ACRSpline.h"
#include "sge_engine/actors/ALight.h"
#include "sge_engine/actors/ALine.h"
#include "sge_utils/stl_algorithm_ex.h"
#include "sge_utils/tiny/FileOpenDialog.h"
#include "sge_utils/utils/FileStream.h"
#include "sge_utils/utils/Path.h"
#include "sge_utils/utils/json.h"
#include "sge_utils/utils/strings.h"

#include "IconsForkAwesome/IconsForkAwesome.h"

namespace sge {

void EditorWindow::Assets::load() {
	AssetLibrary* assetLib = getCore()->getAssetLib();

	m_assetPlayIcon = assetLib->getAsset(AssetType::Texture2D, "assets/editor/textures/icons/play.png", true);
	m_assetForkPlayIcon = assetLib->getAsset(AssetType::Texture2D, "assets/editor/textures/icons/forkplay.png", true);
	m_assetPauseIcon = assetLib->getAsset(AssetType::Texture2D, "assets/editor/textures/icons/pause.png", true);
	m_assetOpenIcon = assetLib->getAsset(AssetType::Texture2D, "assets/editor/textures/icons/open.png", true);
	m_assetSaveIcon = assetLib->getAsset(AssetType::Texture2D, "assets/editor/textures/icons/save.png", true);
	m_assetRefreshIcon = assetLib->getAsset(AssetType::Texture2D, "assets/editor/textures/icons/refresh.png", true);
	m_assetRebuildIcon = assetLib->getAsset(AssetType::Texture2D, "assets/editor/textures/icons/rebuild.png", true);
	m_assetPickingIcon = assetLib->getAsset(AssetType::Texture2D, "assets/editor/textures/icons/pick.png", true);
	m_assetTranslationIcon = assetLib->getAsset(AssetType::Texture2D, "assets/editor/textures/icons/translation.png", true);
	m_assetRotationIcon = assetLib->getAsset(AssetType::Texture2D, "assets/editor/textures/icons/rotation.png", true);
	m_assetScalingIcon = assetLib->getAsset(AssetType::Texture2D, "assets/editor/textures/icons/scale.png", true);
	m_assetVolumeScaleIcon = assetLib->getAsset(AssetType::Texture2D, "assets/editor/textures/icons/volumeScale.png", true);
	m_assetSnapToGridOffIcon = assetLib->getAsset(AssetType::Texture2D, "assets/editor/textures/icons/snapToGridOff.png", true);
	m_assetSnapToGridOnIcon = assetLib->getAsset(AssetType::Texture2D, "assets/editor/textures/icons/snapToGridOn.png", true);

	m_showGameUIOnIcon = assetLib->getAsset(AssetType::Texture2D, "assets/editor/textures/icons/showGameUIOn.png", true);
	m_showGameUIOffIcon = assetLib->getAsset(AssetType::Texture2D, "assets/editor/textures/icons/showGameUIOff.png", true);

	m_orthoIcon = assetLib->getAsset(AssetType::Texture2D, "assets/editor/textures/icons/ortho.png", true);
	m_xIcon = assetLib->getAsset(AssetType::Texture2D, "assets/editor/textures/icons/x.png", true);
	m_yIcon = assetLib->getAsset(AssetType::Texture2D, "assets/editor/textures/icons/y.png", true);
	m_zIcon = assetLib->getAsset(AssetType::Texture2D, "assets/editor/textures/icons/z.png", true);
}

void EditorWindow::onGamePluginPreUnload() {
	m_gameDrawer.reset(nullptr);
	m_sceneWindow->setGameDrawer(nullptr);
	m_sceneInstance.newScene(true);
}

void EditorWindow::onGamePluginChanged() {
	m_gameDrawer.reset(getEngineGlobal()->getActivePlugin()->allocateGameDrawer());
	m_gameDrawer->initialize(&m_sceneInstance.getWorld());
	m_sceneWindow->setGameDrawer(m_gameDrawer.get());
}

EditorWindow::EditorWindow(WindowBase& nativeWindow, std::string windowName)
    : m_nativeWindow(nativeWindow)
    , m_windowName(std::move(windowName)) {
	m_gameDrawer.reset(getEngineGlobal()->getActivePlugin()->allocateGameDrawer());

	getEngineGlobal()->subscribeOnPluginChange([this]() -> void { onGamePluginChanged(); }).abandon();

	sgeAssert(m_gameDrawer != nullptr);

	newScene();

	getEngineGlobal()->addWindow(new AssetsWindow(ICON_FK_FILE " Assets", m_sceneInstance.getInspector()));
	getEngineGlobal()->addWindow(new OutlinerWindow(ICON_FK_SEARCH " Outliner", m_sceneInstance.getInspector()));
	getEngineGlobal()->addWindow(new WorldSettingsWindow(ICON_FK_COGS " World Settings", m_sceneInstance.getInspector()));
	getEngineGlobal()->addWindow(new PropertyEditorWindow(ICON_FK_COG " Property Editor", m_sceneInstance.getInspector()));
	getEngineGlobal()->addWindow(new PrefabWindow("Prefabs", m_sceneInstance.getInspector()));
	m_sceneWindow = new SceneWindow(ICON_FK_GLOBE " Scene", m_gameDrawer.get());
	getEngineGlobal()->addWindow(m_sceneWindow);
	getEngineGlobal()->addWindow(new ActorCreateWindow("Actor Create", m_sceneInstance.getInspector()));

	getEngineGlobal()->addWindow(new LogWindow(ICON_FK_LIST " Log"));

	m_assets.load();

	loadEditorSettings();
}

void EditorWindow::loadEditorSettings() {
	FileReadStream frs("editorSettings.json");
	if (frs.isOpened()) {
		m_rescentOpenedSceneFiles.clear();

		JsonParser jp;
		jp.parse(&frs);

		const JsonValue* jRoot = jp.getRoot();
		const JsonValue* jReascentFiles = jRoot->getMember("reascent_files");
		if (jReascentFiles) {
			for (int t = 0; t < jReascentFiles->arrSize(); ++t) {
				m_rescentOpenedSceneFiles.emplace_back(jReascentFiles->arrAt(t)->GetString());
			}
		}
	}
}

void EditorWindow::saveEditorSettings() {
	JsonValueBuffer jvb;
	JsonValue* jRoot = jvb(JID_MAP);

	JsonValue* jReascentFiles = jvb(JID_ARRAY);

	for (const std::string filename : m_rescentOpenedSceneFiles) {
		jReascentFiles->arrPush(jvb(filename));
	}

	jRoot->setMember("reascent_files", jReascentFiles);

	JsonWriter jw;
	jw.WriteInFile("editorSettings.json", jRoot, true);
}

void EditorWindow::addReasecentScene(const char* const filename) {
	if (filename == nullptr) {
		return;
	}

	// Update the list of reascently used files.
	auto itr = std::find(m_rescentOpenedSceneFiles.begin(), m_rescentOpenedSceneFiles.end(), filename);
	if (itr == m_rescentOpenedSceneFiles.end()) {
		// If the file isn't already in the list, push it to the front.
		push_front(m_rescentOpenedSceneFiles, std::string(filename));
	} else {
		// if the file is already in the list move it to the front.
		std::string x = std::move(*itr);
		m_rescentOpenedSceneFiles.erase(itr);
		m_rescentOpenedSceneFiles.insert(m_rescentOpenedSceneFiles.begin(), std::move(x));
	}
	saveEditorSettings();
}

void EditorWindow::openAssetImport(const std::string& filename) {
	AssetsWindow* wnd = getEngineGlobal()->findFirstWindowOfType<AssetsWindow>();
	if (wnd) {
		ImGui::SetWindowFocus(wnd->getWindowName());
	} else {
		wnd = new AssetsWindow(ICON_FK_FILE " Assets", m_sceneInstance.getInspector());
		getEngineGlobal()->addWindow(wnd);
	}

	wnd->openAssetImport(filename);
}

EditorWindow::~EditorWindow() {
	sgeAssert(false);
}

void EditorWindow::newScene(bool forceKeepSameInspector) {
	m_sceneInstance.newScene(forceKeepSameInspector);
	if (m_gameDrawer) {
		m_gameDrawer->initialize(&m_sceneInstance.getWorld());
	}

	// m_inspector.m_world->userProjectionSettings.aspectRatio = 1.f; // Just some default to be overriden.

	for (auto& wnd : getEngineGlobal()->getAllWindows()) {
		wnd->onNewWorld();
	}

	m_nativeWindow.setWindowTitle("SGEEngine Editor - Untitled Scene*");
}

void EditorWindow::loadWorldFromFile(const char* const filename, const char* overrideWorkingFilename, bool forceKeepSameInspector) {
	std::vector<char> fileContents;
	if (FileReadStream::readFile(filename, fileContents)) {
		fileContents.push_back('\0');
		loadWorldFromJson(fileContents.data(), true, overrideWorkingFilename != nullptr ? overrideWorkingFilename : filename,
		                  forceKeepSameInspector);

		addReasecentScene(filename);

		return;
	}

	sgeAssert(false);
}

void EditorWindow::loadWorldFromJson(const char* const json,
                                     bool disableAutoSepping,
                                     const char* const workingFileName,
                                     bool forceKeepSameInspector) {
	m_sceneInstance.loadWorldFromJson(json, disableAutoSepping, workingFileName, forceKeepSameInspector);

	for (auto& wnd : getEngineGlobal()->getAllWindows()) {
		wnd->onNewWorld();
	}

	std::string windowTitle = string_format("SGEEngine Editor - %s", workingFileName);
	m_nativeWindow.setWindowTitle(windowTitle.c_str());
}

void EditorWindow::saveWorldToFile(bool forceAskForFilename) {
	std::string filename;
	if (!forceAskForFilename && m_sceneInstance.getWorld().m_workingFilePath.empty() == false) {
		filename = m_sceneInstance.getWorld().m_workingFilePath;
	} else {
		// In order for the File Save dialog to point to the default assets/levels directory
		// the directory must exist.
		std::filesystem::create_directories("./assets/levels");
		filename = FileSaveDialog("Save GameWorld to file...", "*.lvl\0*.lvl\0", "lvl", "./assets/levels");
		if (filename.empty() == false) {
			if (extractFileExtension(filename.c_str()).empty()) {
				filename += ".lvl";
			}

			// Check if the file exists if so, ask if we want to overwrite.
			FileReadStream frsCheckExist(filename.c_str());
			if (frsCheckExist.isOpened()) {
				if (DialogYesNo("Save Game World", "Are you sure you want to OVERWRITE the existing level?") == false) {
					filename.clear();
				}
			}
		}
	}

	saveWorldToSpecificFile(filename.c_str());
}

void EditorWindow::saveWorldToSpecificFile(const char* filename) {
	if (filename == nullptr) {
		sgeAssert(false && "saveWorldToSpecificFile expects a filename");
		return;
	}

	bool succeeded = m_sceneInstance.saveWorldToFile(filename);

	if (succeeded) {
		getEngineGlobal()->showNotification(string_format("SUCCEEDED saving '%s'", filename));
		SGE_DEBUG_LOG("[SAVE_LEVEL] Saving game level succeeded. File is %s\n", filename);
		m_sceneInstance.getWorld().m_workingFilePath = filename;

		addReasecentScene(filename);
	} else {
		getEngineGlobal()->showNotification(string_format("FAILED saving level '%s'", filename));
		SGE_DEBUG_WAR("[SAVE_LEVEL] Saving game level failed or canceled!\n");
	}
}


void EditorWindow::update(SGEContext* const sgecon, const InputState& is) {
	m_timer.tick();

	const ImGuiWindowFlags mainWndFlags =
	    ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar;

	ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
	ImGui::SetNextWindowSize(ImVec2(float(m_nativeWindow.GetClientWidth()), float(m_nativeWindow.GetClientHeight())));
	ImGui::Begin(m_windowName.c_str(), nullptr, mainWndFlags);

	// [SGE_EDITOR_MOUSE_CAPTURE_HOTKEY]
	// while editing some games might need the cursor to be relative
	// (hidden and not moving while getting move events) for games file first pesron shooters.
	// In order to test them we need to allow the game to capture the cursor, however
	// when done testing we need to be able to remove that capture and have a normal mouse cursor
	// behaviour to continue editing the game. By pressing F2 we toggle if the game is able or not
	// to capture the cursor.
	if (is.IsKeyPressed(Key::Key_F2)) {
		getEngineGlobal()->setEngineAllowingRelativeCursor(!getEngineGlobal()->getEngineAllowingRelativeCursor());
	}

	if (loadLevelFile.empty() == false) {
		this->loadWorldFromFile(loadLevelFile.c_str());
		loadLevelFile.clear();

		GamePlayWindow* gameplayWindow = getEngineGlobal()->findFirstWindowOfType<GamePlayWindow>();

		if (gameplayWindow == nullptr) {
			JsonValueBuffer jvb;
			JsonValue* jWorld = serializeGameWorld(&m_sceneInstance.getWorld(), jvb);
			JsonWriter jw;
			WriteStdStringStream stringStream;
			jw.write(&stringStream, jWorld);

			gameplayWindow = new GamePlayWindow("Game Play", stringStream.serializedString.c_str());
			getEngineGlobal()->addWindow(gameplayWindow);
		}
	}

	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu(ICON_FK_FILE " File")) {
			if (ImGui::MenuItem(ICON_FK_FILE " New Scene")) {
				if (DialogYesNo("Open GameWorld", "Are you sure? All unsaved data WILL BE LOST?")) {
					newScene();
				}
			}

			if (ImGui::MenuItem(ICON_FK_FOLDER_OPEN " Open Scene")) {
				if (DialogYesNo("Open GameWorld", "Are you sure? All unsaved data WILL BE LOST?")) {
					// In order for the file save dialong to be opned in the default assets/levels directory
					// the directory must exist.
					std::filesystem::create_directories("./assets/levels");
					const std::string filename = FileOpenDialog("Load GameWorld form file...", true, "*.lvl\0*.lvl\0", "./assets/levels");

					if (!filename.empty()) {
						loadWorldFromFile(filename.c_str());
					}
				}
			}

			if (ImGui::BeginMenu(ICON_FK_FOLDER_OPEN " Open Scene...")) {
				static std::vector<std::string> levelsList;

				if (ImGui::IsWindowAppearing()) {
					levelsList.clear();
					// Fild all filels in dir. and store them in levelsList
					if (std::filesystem::is_directory("./assets/levels")) {
						for (const auto& entry : std::filesystem::directory_iterator("./assets/levels")) {
							levelsList.emplace_back(entry.path().string());
						}
					}
				}

				if (levelsList.empty()) {
					ImGui::Text(ICON_FK_EXCLAMATION_TRIANGLE " No files in assets/levels.");
				} else {
					for (const auto& levelFile : levelsList) {
						if (ImGui::MenuItem(levelFile.c_str())) {
							loadWorldFromFile(levelFile.c_str());
						}
					}
				}
				ImGui::EndMenu();
			}

			if (ImGui::MenuItem(ICON_FK_FLOPPY_O " Save")) {
				saveWorldToFile(false);
			}

			if (ImGui::MenuItem(ICON_FK_FLOPPY_O " Save As...")) {
				saveWorldToFile(true);
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(ICON_FK_WINDOW_RESTORE " Window")) {
			if (ImGui::MenuItem(ICON_FK_FAX " Start-up Dialogue")) {
				m_isWelcomeWindowOpened = true;
			}

			ImGui::Separator();

			if (ImGui::MenuItem(ICON_FK_FILE " Assets")) {
				AssetsWindow* wnd = getEngineGlobal()->findFirstWindowOfType<AssetsWindow>();
				if (wnd) {
					ImGui::SetWindowFocus(wnd->getWindowName());
				} else {
					getEngineGlobal()->addWindow(new AssetsWindow(ICON_FK_FILE " Assets", m_sceneInstance.getInspector()));
				}
			}

			if (ImGui::MenuItem(ICON_FK_SEARCH " Outliner")) {
				OutlinerWindow* wnd = getEngineGlobal()->findFirstWindowOfType<OutlinerWindow>();
				if (wnd) {
					ImGui::SetWindowFocus(wnd->getWindowName());
				} else {
					getEngineGlobal()->addWindow(new OutlinerWindow(ICON_FK_SEARCH " Outliner", m_sceneInstance.getInspector()));
				}
			}

			if (ImGui::MenuItem(ICON_FK_COGS " World Settings")) {
				WorldSettingsWindow* wnd = getEngineGlobal()->findFirstWindowOfType<WorldSettingsWindow>();
				if (wnd) {
					ImGui::SetWindowFocus(wnd->getWindowName());
				} else {
					getEngineGlobal()->addWindow(new WorldSettingsWindow(ICON_FK_COGS " World Settings", m_sceneInstance.getInspector()));
				}
			}

			if (ImGui::MenuItem(ICON_FK_COGS " Property Editor")) {
				PropertyEditorWindow* wnd = getEngineGlobal()->findFirstWindowOfType<PropertyEditorWindow>();
				if (wnd) {
					ImGui::SetWindowFocus(wnd->getWindowName());
				} else {
					getEngineGlobal()->addWindow(new PropertyEditorWindow("Propery Editor", m_sceneInstance.getInspector()));
				}
			}

			if (ImGui::MenuItem("Actor Create")) {
				ActorCreateWindow* wnd = getEngineGlobal()->findFirstWindowOfType<ActorCreateWindow>();
				if (wnd) {
					ImGui::SetWindowFocus(wnd->getWindowName());
				} else {
					getEngineGlobal()->addWindow(new ActorCreateWindow("Actor Create", m_sceneInstance.getInspector()));
				}
			}

			if (ImGui::MenuItem("Prefabs")) {
				PrefabWindow* wnd = getEngineGlobal()->findFirstWindowOfType<PrefabWindow>();
				if (wnd) {
					ImGui::SetWindowFocus(wnd->getWindowName());
				} else {
					getEngineGlobal()->addWindow(new PrefabWindow("Prefabs", m_sceneInstance.getInspector()));
				}
			}

			if (ImGui::MenuItem("Game Play")) {
				GamePlayWindow* gameplayWindow = getEngineGlobal()->findFirstWindowOfType<GamePlayWindow>();

				if (gameplayWindow == nullptr) {
					JsonValueBuffer jvb;
					JsonValue* jWorld = serializeGameWorld(&m_sceneInstance.getWorld(), jvb);
					JsonWriter jw;
					WriteStdStringStream stringStream;
					jw.write(&stringStream, jWorld);

					gameplayWindow = new GamePlayWindow("Game Play", stringStream.serializedString.c_str());
					getEngineGlobal()->addWindow(gameplayWindow);
				}
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(ICON_FK_BUG " Debug")) {
			if (ImGui::MenuItem(ICON_FK_LIST " Log")) {
				LogWindow* wnd = getEngineGlobal()->findFirstWindowOfType<LogWindow>();
				if (wnd) {
					ImGui::SetWindowFocus(wnd->getWindowName());
				} else {
					getEngineGlobal()->addWindow(new LogWindow(ICON_FK_LIST " Log"));
				}
			}

			if (ImGui::MenuItem("Model Preview")) {
				getEngineGlobal()->addWindow(new ModelPreviewWindow("Model Preview"));
			}

			if (ImGui::MenuItem("Info")) {
				getEngineGlobal()->addWindow(new InfoWindow("Info"));
			}

			if (ImGui::MenuItem("Game Inspector Window")) {
				getEngineGlobal()->addWindow(new InfoWindow("Info"));
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(ICON_FK_BOOK " Help")) {
			if (ImGui::MenuItem(ICON_FK_BOOK " Keyboard Shortucts and Docs")) {
				HelpWindow* wnd = getEngineGlobal()->findFirstWindowOfType<HelpWindow>();
				if (wnd) {
					ImGui::SetWindowFocus(wnd->getWindowName());
				} else {
					getEngineGlobal()->addWindow(new HelpWindow(ICON_FK_BOOK " Keyboard Shortucts and Docs"));
				}
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Credits")) {
				CreditsWindow* wnd = getEngineGlobal()->findFirstWindowOfType<CreditsWindow>();
				if (wnd) {
					ImGui::SetWindowFocus(wnd->getWindowName());
				} else {
					getEngineGlobal()->addWindow(new CreditsWindow("Credits"));
				}
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(ICON_FK_DOWNLOAD " Run & Export")) {
			ProjectSettingsWindow* wnd = getEngineGlobal()->findFirstWindowOfType<ProjectSettingsWindow>();
			if (wnd) {
				ImGui::SetWindowFocus(wnd->getWindowName());
			} else {
				getEngineGlobal()->addWindow(new ProjectSettingsWindow(ICON_FK_PUZZLE_PIECE " Run & Export"));
			}

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	// Show the notifications in a small window at the bottom right corner.
	if (getEngineGlobal()->getNotificationCount() != 0) {
		const int notificationWndFlags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar |
		                                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing;

		if (ImGui::Begin("SGENotificationWindow", nullptr, notificationWndFlags)) {
			for (int t = 0; t < getEngineGlobal()->getNotificationCount(); ++t) {
				ImGui::TextUnformatted(getEngineGlobal()->getNotificationText(t));
			}

			const ImGuiIO& io = ImGui::GetIO();
			ImVec2 const windowSize = ImGui::GetWindowSize();
			ImVec2 pos;
			pos.x = io.DisplaySize.x - windowSize.x - io.DisplaySize.x * 0.01f;
			pos.y = io.DisplaySize.y - windowSize.y - io.DisplaySize.y * 0.01f;

			ImGui::SetWindowPos(pos);
		}
		ImGui::End();
	}

	// Update the game world.
	InputState isSceneWindowDomain = m_sceneWindow->computeInputStateInDomainSpace(is);
	m_sceneInstance.update(m_timer.diff_seconds(), isSceneWindowDomain);

	// Rendering.
	sgecon->clearDepth(getCore()->getDevice()->getWindowFrameTarget(), 1.f);

	if (ImGui::BeginChild("Toolbar", ImVec2(0, 48), true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
		if (m_assets.m_assetPlayIcon && m_sceneInstance.getInspector().m_disableAutoStepping) {
			if (ImGui::ImageButton(m_assets.m_assetPlayIcon->asTextureView()->tex.GetPtr(), ImVec2(24, 24)))
				m_sceneInstance.getInspector().m_disableAutoStepping = false;
		} else if (m_assets.m_assetPauseIcon && !m_sceneInstance.getInspector().m_disableAutoStepping) {
			if (ImGui::ImageButton(m_assets.m_assetPauseIcon->asTextureView()->tex.GetPtr(), ImVec2(24, 24)))
				m_sceneInstance.getInspector().m_disableAutoStepping = true;
		}


		ImGui::SameLine();

		if (m_assets.m_assetPickingIcon && ImGui::ImageButton(m_assets.m_assetPickingIcon->asTextureView()->tex.GetPtr(), ImVec2(24, 24))) {
			m_sceneInstance.getInspector().setTool(&m_sceneInstance.getInspector().m_selectionTool);
		}
		ImGuiEx::TextTooltip("Enables the scene selection tool.\nShortcut: Q");

		ImGui::SameLine();

		if (m_assets.m_assetTranslationIcon &&
		    ImGui::ImageButton(m_assets.m_assetTranslationIcon->asTextureView()->tex.GetPtr(), ImVec2(24, 24))) {
			m_sceneInstance.getInspector().m_transformTool.m_mode = Gizmo3D::Mode_Translation;
			m_sceneInstance.getInspector().setTool(&m_sceneInstance.getInspector().m_transformTool);
		}
		ImGuiEx::TextTooltip("Enables the actor move tool.\nShortcut: W");

		ImGui::SameLine();

		if (m_assets.m_assetRotationIcon &&
		    ImGui::ImageButton(m_assets.m_assetRotationIcon->asTextureView()->tex.GetPtr(), ImVec2(24, 24))) {
			m_sceneInstance.getInspector().m_transformTool.m_mode = Gizmo3D::Mode_Rotation;
			m_sceneInstance.getInspector().setTool(&m_sceneInstance.getInspector().m_transformTool);
		}
		ImGuiEx::TextTooltip("Enables the actor rotation tool.\nShortcut: E");

		ImGui::SameLine();

		if (m_assets.m_assetScalingIcon && ImGui::ImageButton(m_assets.m_assetScalingIcon->asTextureView()->tex.GetPtr(), ImVec2(24, 24))) {
			m_sceneInstance.getInspector().m_transformTool.m_mode = Gizmo3D::Mode_Scaling;
			m_sceneInstance.getInspector().setTool(&m_sceneInstance.getInspector().m_transformTool);
		}
		ImGuiEx::TextTooltip("Enables the actor scaling tool\nShortcut: R.");

		ImGui::SameLine();

		if (m_assets.m_assetVolumeScaleIcon &&
		    ImGui::ImageButton(m_assets.m_assetVolumeScaleIcon->asTextureView()->tex.GetPtr(), ImVec2(24, 24))) {
			m_sceneInstance.getInspector().m_transformTool.m_mode = Gizmo3D::Mode_ScaleVolume;
			m_sceneInstance.getInspector().setTool(&m_sceneInstance.getInspector().m_transformTool);
		}
		ImGuiEx::TextTooltip("Enables the actor box-scaling tool.\nShortcut: Ctrl + R");

		ImGui::SameLine();

		if (ImGui::ImageButton(m_assets.m_assetForkPlayIcon->asTextureView()->tex.GetPtr(), ImVec2(24, 24))) {
			GamePlayWindow* oldGameplayWindow = getEngineGlobal()->findFirstWindowOfType<GamePlayWindow>();
			if (oldGameplayWindow != nullptr) {
				getEngineGlobal()->removeWindow(oldGameplayWindow);
			}

			JsonValueBuffer jvb;
			JsonValue* jWorld = serializeGameWorld(&m_sceneInstance.getWorld(), jvb);
			JsonWriter jw;
			WriteStdStringStream stringStream;
			jw.write(&stringStream, jWorld);
			getEngineGlobal()->addWindow(new GamePlayWindow("Game Play", stringStream.serializedString.c_str()));
		}
		ImGuiEx::TextTooltip("Play the level in isolation.");

		ImGui::SameLine();
		ImGui::Separator();
		ImGui::SameLine();

		if (m_sceneInstance.getInspector().m_transformTool.m_useSnapSettings) {
			if (m_assets.m_assetSnapToGridOnIcon &&
			    ImGui::ImageButton(m_assets.m_assetSnapToGridOnIcon->asTextureView()->tex.GetPtr(), ImVec2(24, 24))) {
				m_sceneInstance.getInspector().m_transformTool.m_useSnapSettings = false;
			}
		} else {
			if (m_assets.m_assetSnapToGridOffIcon &&
			    ImGui::ImageButton(m_assets.m_assetSnapToGridOffIcon->asTextureView()->tex.GetPtr(), ImVec2(24, 24))) {
				m_sceneInstance.getInspector().m_transformTool.m_useSnapSettings = true;
			}
		}
		ImGuiEx::TextTooltip("Toggle on/off snapping for tools.");

		ImGui::SameLine();

		ImGui::SameLine();
		ImGui::Separator();
		ImGui::SameLine();

		if (m_assets.m_assetRebuildIcon && ImGui::ImageButton(m_assets.m_orthoIcon->asTextureView()->tex.GetPtr(), ImVec2(24, 24))) {
			getInspector().getWorld()->m_editorCamera.isOrthograhpic = !getInspector().getWorld()->m_editorCamera.isOrthograhpic;
		}
		ImGuiEx::TextTooltip("Toggle the orthographic/perspective mode of the preview camera.");

		ImGui::SameLine();
		if (m_assets.m_assetRebuildIcon && ImGui::ImageButton(m_assets.m_xIcon->asTextureView()->tex.GetPtr(), ImVec2(24, 24))) {
			getInspector().getWorld()->m_editorCamera.m_orbitCamera.yaw = 0.f;
			getInspector().getWorld()->m_editorCamera.m_orbitCamera.pitch = 0.f;
		}
		ImGuiEx::TextTooltip("Align the preview camera to +X axis.");

		ImGui::SameLine();
		if (m_assets.m_assetRebuildIcon && ImGui::ImageButton(m_assets.m_yIcon->asTextureView()->tex.GetPtr(), ImVec2(24, 24))) {
			getInspector().getWorld()->m_editorCamera.m_orbitCamera.yaw =
			    deg2rad(90.f) * float(int(getInspector().getWorld()->m_editorCamera.m_orbitCamera.yaw / deg2rad(90.f)));
			getInspector().getWorld()->m_editorCamera.m_orbitCamera.pitch = deg2rad(90.f);
		}
		ImGuiEx::TextTooltip("Align the preview camera to +Y axis.");

		ImGui::SameLine();
		if (m_assets.m_assetRebuildIcon && ImGui::ImageButton(m_assets.m_zIcon->asTextureView()->tex.GetPtr(), ImVec2(24, 24))) {
			getInspector().getWorld()->m_editorCamera.m_orbitCamera.yaw = deg2rad(-90.f);
			getInspector().getWorld()->m_editorCamera.m_orbitCamera.pitch = 0.f;
		}
		ImGuiEx::TextTooltip("Align the preview camera to +Z axis.");

		// int editMode_int = (int)editMode;
		// if (ImGui::Combo("Mode", &editMode_int, "Actors\0Points\0")) {
		//	editMode = (EditMode)editMode_int;
		//}

		ImGui::SameLine();
		ImGui::Separator();
		ImGui::SameLine();
#if 1
		if (m_sceneInstance.getInspector().editMode == editMode_points) {
			// Todo: Reenable this!
			if (ImGui::Button("Tessalate")) {
				std::map<ALine*, std::vector<int>> tessalateBetweenIndices;
				std::map<ACRSpline*, std::vector<int>> tessalateBetweenIndicesCRSpline;

				for (int t = 0; t < m_sceneInstance.getInspector().m_selection.size(); ++t) {
					ALine* const spline =
					    m_sceneInstance.getWorld().getActor<ALine>(m_sceneInstance.getInspector().m_selection[t].objectId);
					if (spline) {
						tessalateBetweenIndices[spline].push_back(m_sceneInstance.getInspector().m_selection[t].index);
					}

					ACRSpline* const crSpline =
					    m_sceneInstance.getWorld().getActor<ACRSpline>(m_sceneInstance.getInspector().m_selection[t].objectId);
					if (crSpline) {
						tessalateBetweenIndicesCRSpline[crSpline].push_back(m_sceneInstance.getInspector().m_selection[t].index);
					}
				}

				for (auto& itr : tessalateBetweenIndices) {
					if (itr.second.size() > 1) {
						ASplineAddPoints* cmd = new ASplineAddPoints();
						cmd->setup(itr.first, itr.second);
						m_sceneInstance.getInspector().appendCommand(cmd, true);
					}
				}

				for (auto& itr : tessalateBetweenIndicesCRSpline) {
					if (itr.second.size() > 1) {
						ACRSplineAddPoints* cmd = new ACRSplineAddPoints();
						cmd->setup(itr.first, itr.second);
						m_sceneInstance.getInspector().appendCommand(cmd, true);
					}
				}
			}
		}
#endif
	}
	ImGui::EndChild();

	ImGuiID dockSpaceID = ImGui::GetID("SGE_CentralDock");

	// Check if ImGui loaded any layout from the imgui_layout_cache.ini file.
	// If so we assume that this is not the 1st run of the editor and the user has configured the windows
	// the way they like. Otherwise, we assume this is first use and load the default layout.
	if (ImGui::DockBuilderGetNode(dockSpaceID) == NULL) {
		const ImVec2 avilableSpace = ImGui::GetContentRegionAvail();

		ImGui::DockBuilderRemoveNode(dockSpaceID); // clear any previous layout
		ImGui::DockBuilderAddNode(dockSpaceID, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockSpaceID, avilableSpace);

		auto dock_id_left = ImGui::DockBuilderSplitNode(dockSpaceID, ImGuiDir_Left, 0.2f, nullptr, &dockSpaceID);
		auto dock_id_left_down = ImGui::DockBuilderSplitNode(dock_id_left, ImGuiDir_Down, 0.2f, nullptr, &dock_id_left);
		auto dock_id_down = ImGui::DockBuilderSplitNode(dockSpaceID, ImGuiDir_Down, 0.25f, nullptr, &dockSpaceID);
		auto dock_id_right = ImGui::DockBuilderSplitNode(dockSpaceID, ImGuiDir_Right, 0.25f, nullptr, &dockSpaceID);

		ImGui::DockBuilderDockWindow(ICON_FK_GLOBE " Scene", dockSpaceID);

		ImGui::DockBuilderDockWindow(ICON_FK_FILE " Assets", dock_id_down);
		ImGui::DockBuilderDockWindow("Prefabs", dock_id_down);
		ImGui::DockBuilderDockWindow("Actor Create", dock_id_down);
		ImGui::DockBuilderDockWindow(ICON_FK_LIST " Log", dock_id_down);

		ImGui::DockBuilderDockWindow(ICON_FK_SEARCH " Outliner", dock_id_left);
		ImGui::DockBuilderDockWindow("Tool Settings", dock_id_left_down);

		ImGui::DockBuilderDockWindow(ICON_FK_COGS " World Settings", dock_id_right);
		ImGui::DockBuilderDockWindow(ICON_FK_COG " Property Editor", dock_id_right);

		ImGui::DockBuilderFinish(dockSpaceID);
	} else {
		ImGui::DockSpace(dockSpaceID, ImVec2(0.f, 0.f), ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_PassthruCentralNode);
	}

	ImGui::End();

	if (m_isWelcomeWindowOpened && ImGui::Begin("Welcome Window", &m_isWelcomeWindowOpened,
	                                            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |
	                                                ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::SetWindowFontScale(2.f);
		ImGui::TextColored(ImVec4(0.25f, 1.f, 0.63f, 1.f), "Welcome to SGEEditor");
		ImGui::SetWindowFontScale(1.f);
		ImGui::NewLine();

		if (m_rescentOpenedSceneFiles.empty()) {
			ImGui::Text("Your reascenlty opened scene files will show here!");
		}

		for (int t = 0; t < minOf(int(m_rescentOpenedSceneFiles.size()), 7); ++t) {
			const std::string& levelFile = m_rescentOpenedSceneFiles[t];
			bool isSelected = false;

			if (ImGui::Selectable(levelFile.c_str(), &isSelected)) {
				std::string filenameToOpen = levelFile;
				loadWorldFromFile(filenameToOpen.c_str());
				m_isWelcomeWindowOpened = false;

				// Caution: we need to break the loop here as the loadWorldFromFile() would modify m_rescentOpenedSceneFiles.
				break;
			}

			if (t == 0) {
				ImGui::NewLine();
				ImGui::Text("Other reascent Files:");
			}
		}

		ImGui::NewLine();

		if (ImGui::Button(ICON_FK_BOOK " Keyboard shortcuts and Help")) {
			HelpWindow* const wnd = getEngineGlobal()->findFirstWindowOfType<HelpWindow>();
			if (wnd) {
				ImGui::SetWindowFocus(wnd->getWindowName());
			} else {
				getEngineGlobal()->addWindow(new HelpWindow(ICON_FK_BOOK " Keyboard Shortucts and Docs"));
			}

			m_isWelcomeWindowOpened = false;
		}

		ImGui::SameLine();

		if (ImGui::Button("Close")) {
			m_isWelcomeWindowOpened = false;
		}

		// If the user clicks somewhere outside of the start-up window, close it.
		const bool isWindowOrItsChildsHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) ||
		                                        ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
		if (!isWindowOrItsChildsHovered && ImGui::IsMouseClicked(0)) {
			m_isWelcomeWindowOpened = false;
		}

		const ImGuiIO& io = ImGui::GetIO();
		ImVec2 const windowSize = ImGui::GetWindowSize();
		ImVec2 pos;
		pos.x = io.DisplaySize.x * 0.5f - windowSize.x * 0.5f;
		pos.y = io.DisplaySize.y * 0.5f - windowSize.y * 0.5f;

		ImGui::SetWindowPos(pos);
		ImGui::End();
	}

	if (is.isKeyCombo(Key::Key_LCtrl, Key::Key_Z)) {
		getInspector().undoCommand();
	}

	if (is.isKeyCombo(Key::Key_LCtrl, Key::Key_Y)) {
		getInspector().redoCommand();
	}

	// Update the existing windows. if on the previous update the window was closed remove it.
	// Note that we do not remove the window on the frame it was closed, as ImGui could have generated
	// command which use data that the window provided to its draw calls (textures for example).
	// If the window is delete and imgui tries to access such data, the executable will crash (or worse).
	for (int iWnd = 0; iWnd < getEngineGlobal()->getAllWindows().size(); ++iWnd) {
		IImGuiWindow* const wnd = getEngineGlobal()->getAllWindows()[iWnd].get();
		sgeAssert(wnd && "This pointer should never be null");

		if (wnd->isClosed()) {
			getEngineGlobal()->removeWindow(wnd);
			iWnd -= 1;
			continue;
		}

		wnd->update(sgecon, is);
	}
}

} // namespace sge
