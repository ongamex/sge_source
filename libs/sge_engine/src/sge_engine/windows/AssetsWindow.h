#pragma once

#include "IImGuiWindow.h"
#include "ModelPreviewWindow.h"
#include "imgui/imgui.h"
#include "sgeImportFBXFile.h"
#include "sge_core/AssetLibrary.h"
#include "sge_utils/utils/DLLHandler.h"
#include <string>

namespace sge {
struct InputState;
struct GameInspector;

/// AssetsWindow is a window in the engine interface for exploring and importing assets.
struct SGE_ENGINE_API AssetsWindow : public IImGuiWindow {
  private:
	struct AssetImportData {
		bool importFailed = false;
		std::string filename;
		AssetType assetType;
		bool importModelsAsMultipleFiles = false;
		std::string outputDir;
		std::string outputFilename;

		bool preview = true;
		ModelPreviewWidget modelPreviewWidget;

		PAsset tempAsset;
	};

  public:
	AssetsWindow(std::string windowName, GameInspector& inspector);
	bool isClosed() override { return !m_isOpened; }
	void update(SGEContext* const sgecon, const InputState& is) override;
	const char* getWindowName() const override { return m_windowName.c_str(); }

	void openAssetImport(const std::string& filename);

  private:
	void update_assetImport(SGEContext* const sgecon, const InputState& is);

	/// @brief Imports the specified asset with the specified settings.
	bool importAsset(AssetImportData& aid);

  private:
	bool shouldOpenImportPopup = false;
	std::string openAssetImport_filename;

	bool m_isOpened = true;
	GameInspector& m_inspector;
	std::string m_windowName;

	/// The path to the currently right clicked object. If the path is empty there is no right clicked object and we should not display the
	/// right click menu.
	std::filesystem::path m_rightClickedPath;

	// A pointer to the asset that currently has a preview.
	PAsset explorePreviewAsset;

	/// When in explorer the user has selected a 3d model this widget is used to draw the preview.
	ModelPreviewWidget m_exploreModelPreviewWidget;
	/// The path to the current directory that we show in the explore.
	std::vector<std::string> directoryTree;
	/// A filter for searching in the current directory in the explorer.
	ImGuiTextFilter m_exploreFilter;

	AssetImportData m_importAssetToImportInPopup;
	std::vector<AssetImportData> m_assetsToImport;

	/// A reference to the dll that holds the mdlconvlib that is used to convert 3d models.
	/// it might be missing the the user hasn't specified the FBX SDK path.
	DLLHandler mdlconvlibHandler;

	/// A pointer to the function from mdlconvlib (if available) for importing 3D models (fbx, obj, dae).
	sgeImportFBXFileFn m_sgeImportFBXFile = nullptr;
	/// A pointer to the function from mdlconvlib (if available) for importing 3D files as multiple models (fbx, dae).
	sgeImportFBXFileAsMultipleFn m_sgeImportFBXFileAsMultiple = nullptr;
};
} // namespace sge
