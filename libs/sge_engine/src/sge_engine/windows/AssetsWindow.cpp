#include <filesystem>

#include "AssetsWindow.h"
#include "IconsForkAwesome/IconsForkAwesome.h"
#include "ModelParseSettings.h"
#include "sge_core/AssetLibrary.h"
#include "sge_core/ICore.h"
#include "sge_core/SGEImGui.h"
#include "sge_core/model/ModelReader.h"
#include "sge_core/model/ModelWriter.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GameInspector.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/ui/ImGuiDragDrop.h"
#include "sge_utils/tiny/FileOpenDialog.h"
#include "sge_utils/utils/Path.h"
#include "sge_utils/utils/strings.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINAMX
#include <Shlobj.h>
#include <Windows.h>
#include <shellapi.h>
#endif

namespace sge {

AssetsWindow::AssetsWindow(std::string windowName, GameInspector& inspector)
    : m_windowName(std::move(windowName))
    , m_inspector(inspector) {
	// Try to load the functions that will be used for importing 3D models.
	if (mdlconvlibHandler.loadNoExt("mdlconvlib")) {
		m_sgeImportFBXFile = reinterpret_cast<sgeImportFBXFileFn>(mdlconvlibHandler.getProcAdress("sgeImportFBXFile"));
		m_sgeImportFBXFileAsMultiple =
		    reinterpret_cast<sgeImportFBXFileAsMultipleFn>(mdlconvlibHandler.getProcAdress("sgeImportFBXFileAsMultiple"));
	}

	if (m_sgeImportFBXFile == nullptr || m_sgeImportFBXFileAsMultiple == nullptr) {
		SGE_DEBUG_WAR("Failed to load dynamic library mdlconvlib. Importing FBX files would not be possible without it!");
	}
}

void AssetsWindow::openAssetImport(const std::string& filename) {
	shouldOpenImportPopup = true;
	openAssetImport_filename = filename;
}

bool AssetsWindow::importAsset(AssetImportData& aid) {
	AssetLibrary* const assetLib = getCore()->getAssetLib();

	std::string fullAssetPath = aid.outputDir + "/" + aid.outputFilename;

	if (aid.assetType == AssetType::Model && aid.importModelsAsMultipleFiles == false) {
		Model importedModel;

		if (m_sgeImportFBXFile == nullptr) {
			SGE_DEBUG_ERR("mdlconvlib dynamic library is not loaded. We cannot import FBX files without it!");
		}

		std::vector<std::string> referencedTextures;
		if (m_sgeImportFBXFile && m_sgeImportFBXFile(importedModel, aid.fileToImportPath.c_str(), &referencedTextures)) {
			createDirectory(extractFileDir(aid.outputDir.c_str(), false).c_str());

			// Convert the 3d model to our internal type.
			ModelWriter modelWriter;
			const bool succeeded = modelWriter.write(importedModel, fullAssetPath.c_str());

			PAsset assetModel = assetLib->getAsset(AssetType::Model, fullAssetPath.c_str(), true);
			assetLib->reloadAssetModified(assetModel.get());

			std::string notificationMsg = string_format("Imported %s", fullAssetPath.c_str());
			SGE_DEBUG_LOG(notificationMsg.c_str());
			getEngineGlobal()->showNotification(notificationMsg);

			// Copy the referenced textures.
			const std::string modelInputDir = extractFileDir(aid.fileToImportPath.c_str(), true);
			for (const std::string& texturePathLocal : referencedTextures) {
				const std::string textureDestDir = aid.outputDir + "/" + extractFileDir(texturePathLocal.c_str(), true);
				const std::string textureFilename = extractFileNameWithExt(texturePathLocal.c_str());

				const std::string textureSrcPath = canonizePathRespectOS(modelInputDir + texturePathLocal);
				const std::string textureDstPath = canonizePathRespectOS(textureDestDir + textureFilename);

				createDirectory(textureDestDir.c_str());
				copyFile(textureSrcPath.c_str(), textureDstPath.c_str());

				PAsset assetTexture = assetLib->getAsset(AssetType::Texture2D, textureDstPath.c_str(), true);
				assetLib->reloadAssetModified(assetTexture.get());
			}

			return true;
		} else {
			std::string notificationMsg = string_format("Failed to import %s", fullAssetPath.c_str());
			SGE_DEBUG_ERR(notificationMsg.c_str());
			getEngineGlobal()->showNotification(notificationMsg);

			return false;
		}
	} else if (aid.assetType == AssetType::Model && aid.importModelsAsMultipleFiles == true) {
		if (m_sgeImportFBXFileAsMultiple == nullptr) {
			SGE_DEBUG_ERR("mdlconvlib dynamic library is not loaded. We cannot import FBX files without it!");
		}

		std::vector<std::string> referencedTextures;
		std::vector<MultiModelImportResult> importedModels;

		if (m_sgeImportFBXFileAsMultiple &&
		    m_sgeImportFBXFileAsMultiple(importedModels, aid.fileToImportPath.c_str(), &referencedTextures)) {
			createDirectory(extractFileDir(aid.outputDir.c_str(), false).c_str());

			for (MultiModelImportResult& model : importedModels) {
				if (model.propsedFilename.empty())
					continue;

				std::string path = aid.outputDir + "/" + model.propsedFilename;

				// Convert the 3d model to our internal type.
				ModelWriter modelWriter;
				const bool succeeded = modelWriter.write(model.importedModel, path.c_str());

				PAsset assetModel = assetLib->getAsset(AssetType::Model, path.c_str(), true);
				assetLib->reloadAssetModified(assetModel.get());

				std::string notificationMsg = string_format("Imported %s", path.c_str());
				SGE_DEBUG_LOG(notificationMsg.c_str());
				getEngineGlobal()->showNotification(notificationMsg);
			}

			// Copy the referenced textures.
			const std::string modelInputDir = extractFileDir(aid.fileToImportPath.c_str(), true);
			for (const std::string& texturePathLocal : referencedTextures) {
				const std::string textureDestDir = aid.outputDir + "/" + extractFileDir(texturePathLocal.c_str(), true);
				const std::string textureFilename = extractFileNameWithExt(texturePathLocal.c_str());

				const std::string textureSrcPath = canonizePathRespectOS(modelInputDir + texturePathLocal);
				const std::string textureDstPath = canonizePathRespectOS(textureDestDir + textureFilename);

				createDirectory(textureDestDir.c_str());
				copyFile(textureSrcPath.c_str(), textureDstPath.c_str());

				PAsset assetTexture = assetLib->getAsset(AssetType::Texture2D, textureDstPath.c_str(), true);
				assetLib->reloadAssetModified(assetTexture.get());
			}

			return true;
		} else {
			std::string notificationMsg = string_format("Failed to import %s", fullAssetPath.c_str());
			SGE_DEBUG_ERR(notificationMsg.c_str());
			getEngineGlobal()->showNotification(notificationMsg);

			return false;
		}
	} else if (aid.assetType == AssetType::Texture2D) {
		// TODO: DDS conversion.
		createDirectory(extractFileDir(aid.outputDir.c_str(), false).c_str());
		copyFile(aid.fileToImportPath.c_str(), fullAssetPath.c_str());

		PAsset assetTexture = assetLib->getAsset(AssetType::Texture2D, fullAssetPath.c_str(), true);
		assetLib->reloadAssetModified(assetTexture.get());

		return true;
	} else if (aid.assetType == AssetType::Sprite) {
		SpriteAnimation tempSpriteAnimation;
		if (SpriteAnimation::importFromAsepriteSpriteSheetJsonFile(tempSpriteAnimation, aid.fileToImportPath.c_str())) {
			// There is a texture associated with this sprite, import it as well.
			std::string importTextureSource = extractFileDir(aid.fileToImportPath.c_str(), true) + tempSpriteAnimation.texturePath;
			std::string importTextureDest = aid.outputDir + "/" + tempSpriteAnimation.texturePath;
			createDirectory(extractFileDir(importTextureDest.c_str(), false).c_str());
			copyFile(importTextureSource.c_str(), importTextureDest.c_str());

			// Finally make the path to the texture relative to the .sprite file.
			tempSpriteAnimation.texturePath = relativePathTo(aid.outputDir.c_str(), "./") + "/" + tempSpriteAnimation.texturePath;

			return tempSpriteAnimation.saveSpriteToFile(fullAssetPath.c_str());
		} else {
			SGE_DEBUG_ERR("Failed to import %s as a Sprite!", aid.fileToImportPath.c_str());
		}
	} else if (aid.assetType == AssetType::Text) {
		createDirectory(extractFileDir(aid.outputDir.c_str(), false).c_str());
		copyFile(aid.fileToImportPath.c_str(), fullAssetPath.c_str());

		PAsset asset = assetLib->getAsset(AssetType::Text, fullAssetPath.c_str(), true);
		assetLib->reloadAssetModified(asset.get());

		return true;
	} else {
		createDirectory(extractFileDir(aid.outputDir.c_str(), false).c_str());
		copyFile(aid.fileToImportPath.c_str(), fullAssetPath.c_str());
		SGE_DEBUG_WAR("Imported a files by just copying it as it is not recognized asset type!")
		return true;
	}

	return false;
}

void AssetsWindow::update_assetImport(SGEContext* const sgecon, const InputState& is) {
	if (ImGui::Button(ICON_FK_PLUS " Add Asset")) {
		std::string filename = FileOpenDialog("Import 3D Model", true, "*.fbx\0*.fbx\0*.dae\0*.dae\0*.obj\0*.obj\0*.*\0*.*\0", nullptr);
		if (filename.empty() == false) {
			openAssetImport(filename);
		}
	}

	if (ImGui::BeginChild("Child Assets To Import")) {
		std::string groupPanelName;
		for (int iAsset = 0; iAsset < m_assetsToImport.size(); ++iAsset) {
			AssetImportData& aid = m_assetsToImport[iAsset];
			ImGui::PushID(&aid);

			if (aid.importFailed) {
				ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "Failed to Import");
			}

			string_format(groupPanelName, "3D Model %s", aid.fileToImportPath.c_str());
			ImGuiEx::BeginGroupPanel(groupPanelName.c_str());
			{
				if (ImGui::Button("Import As")) {
					aid.outputFilename = FileSaveDialog("Import 3D Model As", "*.mdl\0*.mdl", "mdl", nullptr);
				}
				ImGui::SameLine();
				char importAs[1024] = {0};
				sge_strcpy(importAs, aid.outputFilename.c_str());
				if (ImGui::InputText("##Import As", importAs, SGE_ARRSZ(importAs))) {
					aid.outputFilename = importAs;
				}

				ImGui::Checkbox("Preview", &aid.preview);
				if (aid.preview) {
					if (aid.assetType == AssetType::Model) {
						aid.modelPreviewWidget.doWidget(sgecon, is, aid.tempAsset->asModel()->staticEval, vec2f(-1.f, 256.f));
					}
				}

				if (ImGui::Button(ICON_FK_DOWNLOAD "Import")) {
					if (importAsset(aid)) {
						m_assetsToImport.erase(m_assetsToImport.begin() + iAsset);
						iAsset--;
					}
				}

				ImGui::SameLine();
				if (ImGui::Button(ICON_FK_TRASH " Remove")) {
					m_assetsToImport.erase(m_assetsToImport.begin() + iAsset);
					iAsset--;
				}
			}
			ImGuiEx::EndGroupPanel();

			ImGui::PopID();
		}

		if (ImGui::Button("Import All")) {
		}

		ImGui::EndChild();
	}
}

void AssetsWindow::update(SGEContext* const sgecon, const InputState& is) {
	if (isClosed()) {
		return;
	}

	AssetLibrary* const assetLib = getCore()->getAssetLib();

	if (ImGui::Begin(m_windowName.c_str(), &m_isOpened)) {
		namespace fs = std::filesystem;

		if (ImGui::Button(ICON_FK_BACKWARD)) {
			if (directoryTree.empty() == false) {
				directoryTree.pop_back();
			}
		}

		ImGui::SameLine();
		ImGui::Separator();
		ImGui::SameLine();
		ImGui::Text("Path: ");

		{
			int eraseAfter = -1;
			for (int t = 0; t < int(directoryTree.size()); ++t) {
				ImGui::SameLine();

				if (ImGui::Button(directoryTree[t].c_str())) {
					eraseAfter = t + 1;
				}
			}

			if (eraseAfter > -1) {
				while (directoryTree.size() > eraseAfter) {
					directoryTree.pop_back();
				}
			}
		}

		ImGui::NewLine();

		bool explorePreviewAssetChanged = false;
		if (explorePreviewAsset) {
			ImGui::Columns(2);
		}

		ImGui::Text(ICON_FK_SEARCH " File Filter");
		ImGui::SameLine();
		m_exploreFilter.Draw("##File Filter");
		if (ImGui::IsItemClicked(2)) {
			ImGui::ClearActiveID(); // Hack: (if we do not make this call ImGui::InputText will set it's cached value.
			m_exploreFilter.Clear();
		}

		// List all files in the currently selected directory in the interfance.
		if (ImGui::BeginChildFrame(ImGui::GetID("FilesChildFrameID"), ImVec2(-1.f, -1.f))) {
			try {
				if (!directoryTree.empty() && ImGui::Selectable(ICON_FK_BACKWARD " [/..]")) {
					directoryTree.pop_back();
				}

				bool shouldOpenNewFolderPopup = false;

				std::string label;
				fs::path pathToAssets = assetLib->getAssetsDirAbs();
				for (auto& p : directoryTree) {
					pathToAssets.append(p);
				}

				if (ImGui::Selectable(ICON_FK_FOLDER_O " [New Folder]")) {
					shouldOpenNewFolderPopup = true;
				}

				Optional<fs::path> rightClickedPath;

				std::string dirToAdd;
				for (const fs::directory_entry& entry : fs::directory_iterator(pathToAssets)) {
					if (entry.is_directory() && m_exploreFilter.PassFilter(entry.path().filename().string().c_str())) {
						string_format(label, "%s %s", ICON_FK_FOLDER, entry.path().filename().string().c_str());
						if (ImGui::Selectable(label.c_str())) {
							dirToAdd = entry.path().filename().string();
						}

						if (ImGui::IsItemClicked(1)) {
							rightClickedPath = entry.path();
						}
					}
				}

				// Show every file in the current directory with an icon next to it.
				for (const fs::directory_entry& entry : fs::directory_iterator(pathToAssets)) {

					const std::string localAssetPath = relativePathToCwdCanoize(entry.path().string());

					if (entry.is_regular_file() && m_exploreFilter.PassFilter(entry.path().filename().string().c_str())) {
						AssetType assetType =
						    assetType_guessFromExtension(extractFileExtension(entry.path().string().c_str()).c_str(), false);
						if (assetType == AssetType::Model) {
							string_format(label, "%s %s", ICON_FK_CUBE, entry.path().filename().string().c_str());
						} else if (assetType == AssetType::Texture2D) {
							string_format(label, "%s %s", ICON_FK_PICTURE_O, entry.path().filename().string().c_str());
						} else if (assetType == AssetType::Text) {
							string_format(label, "%s %s", ICON_FK_FILE, entry.path().filename().string().c_str());
						} else if (assetType == AssetType::Audio) {
							string_format(label, "%s %s", ICON_FK_FILE_AUDIO_O, entry.path().filename().string().c_str());
						} else {
							string_format(label, "%s %s", ICON_FK_FILE_TEXT_O, entry.path().filename().string().c_str());
						}

						if (ImGui::Selectable(label.c_str())) {
							explorePreviewAssetChanged = true;
							explorePreviewAsset = assetLib->getAsset(localAssetPath.c_str(), true);
						}
						if (ImGui::IsItemClicked(1)) {
							rightClickedPath = entry.path();
						}

						if (ImGui::BeginDragDropSource()) {
							DragDropPayloadAsset::setPayload(localAssetPath);
							ImGui::Text(localAssetPath.c_str());
							ImGui::EndDragDropSource();
						}
					}
				}

				// Handle right-clicking over an asset in the explorer.
				if (rightClickedPath.hasValue()) {
					ImGui::OpenPopup("RightClickMenuAssets");
					m_rightClickedPath = rightClickedPath.get();
				} else if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1)) {
					ImGui::OpenPopup("RightClickMenuAssets");
					m_rightClickedPath.clear();
				}

				// Right click menu.
				fs::path importOverAsset;
				if (ImGui::BeginPopup("RightClickMenuAssets")) {
					if (ImGui::MenuItem(ICON_FK_DOWNLOAD " Import here...")) {
						shouldOpenImportPopup = true;
						openAssetImport_filename.clear();
					}

					if (ImGui::MenuItem(ICON_FK_FOLDER " New Folder")) {
						shouldOpenNewFolderPopup = true;
					}

					if (!m_rightClickedPath.empty()) {
						ImGui::Separator();

						if (ImGui::MenuItem(ICON_FK_CLIPBOARD " Copy Path")) {
							ImGui::SetClipboardText(m_rightClickedPath.string().c_str());
						}

						if (ImGui::MenuItem(ICON_FK_REFRESH " Import Over")) {
							importOverAsset = m_rightClickedPath;
							shouldOpenImportPopup = true;
							openAssetImport_filename.clear();
						}

#ifdef WIN32
						if (ImGui::MenuItem(ICON_FK_WINDOWS " Open in Explorer")) {
							PIDLIST_ABSOLUTE pItem = ILCreateFromPathA(m_rightClickedPath.string().c_str());
							if (pItem) {
								SHOpenFolderAndSelectItems(pItem, 0, NULL, 0);
								ILFree(pItem);
							}
						}
#endif
					}

					ImGui::EndPopup();
				}

				// Import Popup
				if (shouldOpenImportPopup) {
					ImGui::OpenPopup("SGE Assets Window Import Popup");
				}

				if (ImGui::BeginPopup("SGE Assets Window Import Popup")) {
					if (shouldOpenImportPopup) {
						shouldOpenImportPopup = false;
						// If this is still true then the popup has just been opened.
						// Initialize it with the information about the asset we are about to import.
						m_importAssetToImportInPopup = AssetImportData();

						// If openAssetImport_filename is specified then we must use it, it means that the popup was somehow forced
						// externally like a drag-and-drop.
						if (openAssetImport_filename.empty()) {
							m_importAssetToImportInPopup.fileToImportPath =
							    FileOpenDialog("Pick a file to import", true, "*.*\0*.*\0", nullptr);
						} else {
							m_importAssetToImportInPopup.fileToImportPath = openAssetImport_filename;
							openAssetImport_filename.clear();
						}

						// If the user clicked over an assed and clicked import over, use the name of already imported asset,
						// otherwise create a new name based on the input name.
						m_importAssetToImportInPopup.outputDir = pathToAssets.string();
						if (importOverAsset.empty()) {
							m_importAssetToImportInPopup.outputFilename =
							    extractFileNameWithExt(m_importAssetToImportInPopup.fileToImportPath.c_str());
						} else {
							m_importAssetToImportInPopup.outputFilename = importOverAsset.filename().string();
							importOverAsset.clear();
						}

						// Guess the type of the inpute asset.
						const std::string inputExtension = extractFileExtension(m_importAssetToImportInPopup.fileToImportPath.c_str());
						m_importAssetToImportInPopup.assetType = assetType_guessFromExtension(inputExtension.c_str(), true);

						// If the asset type is None, maybe the asset has a commonly used extension
						// like Aseprite json sprite sheet descriptors, or they might be just generic
						// files that the user wants to copy. Try to guess the type of this generic file
						// for convinice of the user.
						if (m_importAssetToImportInPopup.assetType == AssetType::None) {
							if (inputExtension == "json") {
								SpriteAnimation tempSpriteAnimation;
								if (SpriteAnimation::importFromAsepriteSpriteSheetJsonFile(
								        tempSpriteAnimation, m_importAssetToImportInPopup.fileToImportPath.c_str())) {
									m_importAssetToImportInPopup.assetType = AssetType::Sprite;
								}
							}
						}

						// Change the extension of the imported file based on the asset type.
						if (m_importAssetToImportInPopup.assetType == AssetType::Model) {
							m_importAssetToImportInPopup.outputFilename =
							    replaceExtension(m_importAssetToImportInPopup.outputFilename.c_str(), "mdl");
						}

						if (m_importAssetToImportInPopup.assetType == AssetType::Sprite) {
							m_importAssetToImportInPopup.outputFilename =
							    replaceExtension(m_importAssetToImportInPopup.outputFilename.c_str(), "sprite");
						}

						if (m_importAssetToImportInPopup.fileToImportPath.empty()) {
							ImGui::CloseCurrentPopup();
						}
					}

					// The UI of the pop-up itself:
					if (m_importAssetToImportInPopup.assetType == AssetType::Model) {
						ImGui::Text(ICON_FK_CUBE " 3D Model");
						ImGui::Checkbox(ICON_FK_CUBES " Import As Multiple Models",
						                &m_importAssetToImportInPopup.importModelsAsMultipleFiles);
						ImGuiEx::TextTooltip(
						    "When multiple game objects are defined in one 3D model file. You can import them as a separate 3D "
						    "models using this option!");
					} else if (m_importAssetToImportInPopup.assetType == AssetType::Texture2D) {
						ImGui::Text(ICON_FK_PICTURE_O " Texture");
					} else if (m_importAssetToImportInPopup.assetType == AssetType::Text) {
						ImGui::Text(ICON_FK_FILE " Text");
					} else if (m_importAssetToImportInPopup.assetType == AssetType::Sprite) {
						ImGui::Text(ICON_FK_FILM " Sprite");
					} else {
						ImGui::Text(ICON_FK_FILE_TEXT_O " Unknown, the file is going to be copyied!");
						ImGui::Text("If you know the type of the asset you can override it below.");

						const char* assetTypeNames[int(AssetType::Count)] = {nullptr};
						assetTypeNames[int(AssetType::None)] = ICON_FK_FILE_TEXT_O " Unknown";
						assetTypeNames[int(AssetType::Model)] = ICON_FK_CUBE " 3D Model";
						assetTypeNames[int(AssetType::Texture2D)] = ICON_FK_PICTURE_O " Texture";
						assetTypeNames[int(AssetType::Text)] = ICON_FK_FILE " Text";
						assetTypeNames[int(AssetType::Sprite)] = ICON_FK_FILM " Sprite";

						ImGuiEx::Label("Import As:");
						if (ImGui::BeginCombo("##Import As: ", assetTypeNames[int(m_importAssetToImportInPopup.assetType)])) {
							for (int t = 0; t < SGE_ARRSZ(assetTypeNames); ++t) {
								if (assetTypeNames[t] != nullptr) {
									if (ImGui::Selectable(assetTypeNames[t])) {
										m_importAssetToImportInPopup.assetType = AssetType(t);
										if (t == int(AssetType::Model)) {
											m_importAssetToImportInPopup.outputFilename =
											    replaceExtension(m_importAssetToImportInPopup.outputFilename.c_str(), "mdl");
										}
										if (t == int(AssetType::Sprite)) {
											m_importAssetToImportInPopup.outputFilename =
											    replaceExtension(m_importAssetToImportInPopup.outputFilename.c_str(), "sprite");
										}
									}
								}
							}

							ImGui::EndCombo();
						}
					}

					ImGuiEx::InputText("Read From", m_importAssetToImportInPopup.fileToImportPath, ImGuiInputTextFlags_ReadOnly);

					if (m_importAssetToImportInPopup.importModelsAsMultipleFiles == false) {
						ImGuiEx::InputText("Import As", m_importAssetToImportInPopup.outputFilename);
					}

					// Show a warning that the import will fail if mdlconvlib is not loaded.
					if (m_sgeImportFBXFile == nullptr && m_importAssetToImportInPopup.assetType == AssetType::Model) {
						ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "Importing 3D files cannot be done! mdlconvlib is missing!");
					}


					if (ImGui::Button(ICON_FK_DOWNLOAD " Import")) {
						importAsset(m_importAssetToImportInPopup);
						ImGui::CloseCurrentPopup();
					}
					ImGui::SameLine();
					if (ImGui::Button(ICON_FK_DOWNLOAD " Cancel")) {
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}

				// Create Directory Popup.
				{
					if (shouldOpenNewFolderPopup) {
						ImGui::OpenPopup("SGE Assets Window Create Dir");
					}

					static char createDirFileName[1024] = {0};
					if (ImGui::BeginPopup("SGE Assets Window Create Dir")) {
						ImGui::InputText(ICON_FK_FOLDER " Folder Name", createDirFileName, SGE_ARRSZ(createDirFileName));
						if (ImGui::Button(ICON_FK_CHECK " Create")) {
							createDirectory((pathToAssets.string() + "/" + std::string(createDirFileName)).c_str());
							createDirFileName[0] = '\0';
							ImGui::CloseCurrentPopup();
						}

						if (ImGui::Button("Cancel")) {
							ImGui::CloseCurrentPopup();
						}

						ImGui::EndPopup();
					}

					if (dirToAdd.empty() == false) {
						directoryTree.emplace_back(std::move(dirToAdd));
					}
				}
			}


			catch (...) {
			}
		}
		ImGui::EndChildFrame();

		if (isAssetLoaded(explorePreviewAsset)) {
			ImGui::NextColumn();

			if (explorePreviewAsset->getType() == AssetType::Model) {
				if (explorePreviewAssetChanged) {
					AABox3f bboxModel = explorePreviewAsset->asModel()->staticEval.aabox;
					if (bboxModel.IsEmpty() == false) {
						m_exploreModelPreviewWidget.camera.orbitPoint = bboxModel.center();
						m_exploreModelPreviewWidget.camera.radius = bboxModel.diagonal().length() * 1.25f;
						m_exploreModelPreviewWidget.camera.yaw = deg2rad(45.f);
						m_exploreModelPreviewWidget.camera.pitch = deg2rad(45.f);
					}
				}

				m_exploreModelPreviewWidget.doWidget(sgecon, is, explorePreviewAsset->asModel()->staticEval);

				ImGui::Text("Animation Count: %d", explorePreviewAsset->asModel()->model.numAnimations());
				for (int iAnim = 0; iAnim < explorePreviewAsset->asModel()->model.numAnimations(); ++iAnim) {
					ImGui::Text("\t%s", explorePreviewAsset->asModel()->model.animationAt(iAnim)->animationName.c_str());
				}

				ImGui::Text("Node Count: %d", explorePreviewAsset->asModel()->model.numNodes());
				ImGui::Text("Mesh Count: %d", explorePreviewAsset->asModel()->model.numMeshes());

				for (int iMesh = 0; iMesh < explorePreviewAsset->asModel()->model.numMeshes(); ++iMesh) {
					ImGui::Text("\t%s", explorePreviewAsset->asModel()->model.meshAt(iMesh)->name.c_str());
				}

				ImGui::Text("Material Count: %d", explorePreviewAsset->asModel()->model.numMaterials());
				for (int iMtl = 0; iMtl < explorePreviewAsset->asModel()->model.numMaterials(); ++iMtl) {
					ImGui::Text("\t%s", explorePreviewAsset->asModel()->model.materialAt(iMtl)->name.c_str());
				}

			} else if (explorePreviewAsset->getType() == AssetType::Texture2D) {
				const Texture2DDesc desc = explorePreviewAsset->asTextureView()->tex->getDesc().texture2D;
				const ImVec2 availableContentSize = ImGui::GetContentRegionAvail();

				const float imageSizeX = availableContentSize.x;
				const float imageSizeY = desc.height * availableContentSize.x / float(desc.width);

				ImGui::Image(explorePreviewAsset->asTextureView()->tex.GetPtr(), ImVec2(imageSizeX, imageSizeY));

				AssetTexture* const texAsset = explorePreviewAsset->asTextureView();

				const auto getAddressModeName = [](TextureAddressMode::Enum mode) -> const char* {
					switch (mode) {
						case TextureAddressMode::Repeat:
							return "Repeat";
						case TextureAddressMode::ClampEdge:
							return "Edge";
						case TextureAddressMode::ClampBorder:
							return "Border";

						default:
							return "Unknown";
					}
				};

				const auto getFilterModeName = [](TextureFilter::Enum mode) -> const char* {
					switch (mode) {
						case TextureFilter::Min_Mag_Mip_Linear:
							return "Linear (with mip mapping)";
						case TextureFilter::Min_Mag_Mip_Point:
							return "Point (with mip mapping)";
						default:
							return "Unknown";
					}
				};

				const auto doAddressModeUI = [&getAddressModeName](const char* comboLabel, TextureAddressMode::Enum& mode) -> bool {
					bool hadChange = false;
					if (ImGui::BeginCombo(comboLabel, getAddressModeName(mode))) {
						if (ImGui::Selectable(getAddressModeName(TextureAddressMode::Repeat))) {
							mode = TextureAddressMode::Repeat;
							hadChange = true;
						}

						if (ImGui::Selectable(getAddressModeName(TextureAddressMode::ClampEdge))) {
							mode = TextureAddressMode::ClampEdge;
							hadChange = true;
						}

						if (ImGui::Selectable(getAddressModeName(TextureAddressMode::ClampBorder))) {
							mode = TextureAddressMode::ClampBorder;
							hadChange = true;
						}

						ImGui::EndCombo();
					}

					return hadChange;
				};

				bool hadChange = false;
				hadChange |= doAddressModeUI("Tiling X", texAsset->assetSamplerDesc.addressModes[0]);
				hadChange |= doAddressModeUI("Tiling Y", texAsset->assetSamplerDesc.addressModes[1]);
				hadChange |= doAddressModeUI("Tiling Z", texAsset->assetSamplerDesc.addressModes[2]);

				if (ImGui::BeginCombo("Filtering", getFilterModeName(texAsset->assetSamplerDesc.filter))) {
					if (ImGui::Selectable(getFilterModeName(TextureFilter::Min_Mag_Mip_Linear))) {
						texAsset->assetSamplerDesc.filter = TextureFilter::Min_Mag_Mip_Linear;
						hadChange = true;
					}

					if (ImGui::Selectable(getFilterModeName(TextureFilter::Min_Mag_Mip_Point))) {
						texAsset->assetSamplerDesc.filter = TextureFilter::Min_Mag_Mip_Point;
						hadChange = true;
					}

					ImGui::EndCombo();
				}

				if (hadChange) {
					GpuHandle<SamplerState> sampler = getCore()->getDevice()->requestResource<SamplerState>();
					sampler->create(texAsset->assetSamplerDesc);
					texAsset->tex->setSamplerState(sampler);

					// Save the modified settings to the *.info file of the texture.
					texAsset->saveTextureSettingsToInfoFile(*explorePreviewAsset.get());
				}
			} else if (explorePreviewAsset->getType() == AssetType::Sprite) {
				if (isAssetLoaded(explorePreviewAsset->asSprite()->textureAsset)) {
					auto desc = explorePreviewAsset->asSprite()->textureAsset->asTextureView()->tex->getDesc().texture2D;
					ImVec2 sz = ImGui::GetContentRegionAvail();
					ImGui::Image(explorePreviewAsset->asSprite()->textureAsset->asTextureView()->tex.GetPtr(), sz);
				}
			} else if (explorePreviewAsset->getType() == AssetType::Audio) {
				auto track = explorePreviewAsset->asAudio();
				ImGui::Text("Vorbis encoded Audio file");
				ImGui::Text("Sample Rate: %.1f kHZ", (float)(*track)->info.samplesPerSecond / 1000.0f);
				ImGui::Text("Number of channels: %d", (*track)->info.numChannels);
				ImGui::Text("Length: %.2f s", (float)(*track)->info.numSamples / (float)(*track)->info.samplesPerSecond);
			} else {
				ImGui::Text("No Preview");
			}
		}
	}
	ImGui::End();
}

} // namespace sge
