#pragma once

#include <string>

#include "imgui/imgui.h"
#include "sge_utils/utils/timer.h"

#include "IImGuiWindow.h"
#include "sge_engine/GameDrawer/GameDrawer.h"
#include "sge_engine/SceneInstance.h"

namespace sge {
struct WindowBase;
struct InputState;
struct Asset;

struct SceneWindow;

struct SGE_ENGINE_API EditorWindow : public IImGuiWindow {
  private:
	struct Assets {
		std::shared_ptr<Asset> m_assetForkPlayIcon;
		std::shared_ptr<Asset> m_assetPlayIcon;
		std::shared_ptr<Asset> m_assetPauseIcon;
		std::shared_ptr<Asset> m_assetOpenIcon;
		std::shared_ptr<Asset> m_assetSaveIcon;
		std::shared_ptr<Asset> m_assetRefreshIcon;
		std::shared_ptr<Asset> m_assetRebuildIcon;

		std::shared_ptr<Asset> m_assetPickingIcon;
		std::shared_ptr<Asset> m_assetTranslationIcon;
		std::shared_ptr<Asset> m_assetRotationIcon;
		std::shared_ptr<Asset> m_assetScalingIcon;
		std::shared_ptr<Asset> m_assetVolumeScaleIcon;

		std::shared_ptr<Asset> m_assetSnapToGridOffIcon;
		std::shared_ptr<Asset> m_assetSnapToGridOnIcon;

		std::shared_ptr<Asset> m_showGameUIOnIcon;
		std::shared_ptr<Asset> m_showGameUIOffIcon;

		std::shared_ptr<Asset> m_orthoIcon;
		std::shared_ptr<Asset> m_xIcon;
		std::shared_ptr<Asset> m_yIcon;
		std::shared_ptr<Asset> m_zIcon;

		void load();
	};

  public:
	EditorWindow(WindowBase& nativeWindow, std::string windowName);
	~EditorWindow();

	void onGamePluginPreUnload();
	void onGamePluginChanged();


	bool isClosed() override { return false; }

	void update(SGEContext* const sgecon, const InputState& is) override;
	const char* getWindowName() const override { return m_windowName.c_str(); }

	void newScene(bool forceKeepSameInspector = false);
	void loadWorldFromFile(const char* const filename, const char* overrideWorkingFilename = nullptr, bool forceKeepSameInspector = false);
	void loadWorldFromJson(const char* const json,
	                       bool disableAutoSepping,
	                       const char* const workingFileName = nullptr,
	                       bool forceKeepSameInspector = false);
	void saveWorldToFile(bool forceAskForFilename);
	void saveWorldToSpecificFile(const char* filename);

	void loadEditorSettings();
	void saveEditorSettings();
	void addReasecentScene(const char* const filename);

	void closeWelcomeWindow() { m_isWelcomeWindowOpened = false; }

	void openAssetImport(const std::string& filename);

	GameInspector& getInspector() { return m_sceneInstance.getInspector(); }
	const GameInspector& getInspector() const { return m_sceneInstance.getInspector(); }
	GameWorld& getWorld() { return m_sceneInstance.getWorld(); }
	const GameWorld& getWorld() const { return m_sceneInstance.getWorld(); }

	std::string loadLevelFile;

  private:
	Assets m_assets;

	WindowBase& m_nativeWindow;
	SceneInstance m_sceneInstance;
	std::unique_ptr<IGameDrawer> m_gameDrawer;
	std::string m_windowName;

	SceneWindow* m_sceneWindow = nullptr;

	Timer m_timer;

	bool m_isWelcomeWindowOpened = true;

	std::vector<std::string> m_rescentOpenedSceneFiles;
};
} // namespace sge
