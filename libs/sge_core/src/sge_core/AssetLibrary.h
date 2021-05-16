#pragma once

#include <map>
#include <memory>

#include "sge_core/Sprite.h"
#include "sge_core/model/EvaluatedModel.h"
#include "sge_core/model/Model.h"
#include "sge_utils/utils/vector_map.h"
#include "sgecore_api.h"

namespace sge {
struct AssetLibrary;

struct AudioTrack;
using AudioAsset = std::shared_ptr<AudioTrack>;

struct SGE_CORE_API AssetTexture {
	GpuHandle<Texture> tex;
	/// The sampler description defined by the asset file.
	/// This is no necessery the actual descriptor assigned to @tex.
	SamplerDesc assetSamplerDesc;

	bool saveTextureSettingsToInfoFile(Asset& assetSelf);
	/// @brief
	/// @param [in] baseAssetPath is the path to the texture. It will get converted automatically to *.info file.
	/// @return
	static SamplerDesc loadTextureSettingInfoFile(const std::string& baseAssetPath);
};

struct AssetModel {
	Model::Model model;
	EvaluatedModel staticEval;
	EvaluatedModel sharedEval;
};

// Defines all posible asset types.
enum class AssetType : int {
	None,        ///< Just an invalid asset type, used as a default.
	Model,       ///< A 3D model.
	TextureView, ///< AssetTexture
	Text,        ///< A file containing some text (including code).
	Sprite,      ///< A 2D sprite sheet animation.
	Audio,       ///< Vorbis encoded audio file. Usually used for background music or longer audio tracks.

	Count,
};

/// @brief Returns a name suitable for displaying to the user for the specified asset type.
SGE_CORE_API const char* assetType_getName(const AssetType type);

/// @brief Guesses the potential assset type based in the specified extension. Keep in mind that this is a guess.
/// @param [in] ext the extension to be used for guessing. Should not include the dot.
///                 The comparison will be case insensitive ("txt" is the same as "TXT").
/// @param [in] includeExternalExtensions True if unconverted file types should be concidered,
///             like fbx/obj/dae could be guessed as a 3d model.
/// @return the guessed asset type.
SGE_CORE_API AssetType assetType_guessFromExtension(const char* const ext, bool includeExternalExtensions);

/// Provides an interface that is used to allocated a particular asset of a type.
struct SGE_CORE_API IAssetAllocator {
	virtual void* allocate() = 0;
	virtual void deallocate(void* ptr) = 0;
};

/// Provides an interface that is used to load/unload/ect a particular asset of a type.
struct SGE_CORE_API IAssetFactory {
	virtual void getDependancyList(void* UNUSED(asset), std::vector<std::string>& UNUSED(deps)) {}
	virtual bool load(void* const pAsset, const char* const pPath, AssetLibrary* const mpMngrngr) = 0;
	virtual void unload(void* const pAsset, AssetLibrary* const pMngr) = 0;
};

/// @brief Describes the current status of an asset.
enum class AssetStatus : int {
	/// The assets seems to exist but it is not loaded.
	NotLoaded,
	/// The asset is loaded.
	Loaded,
	/// Loading the asset failed. Maybe the files is broken or it does not exist.
	LoadFailed,
};

/// @brief Asset provides a data storage and and status tracker for all assets.
/// It is usually used as a weak/shared_ptr for reference counting.
struct SGE_CORE_API Asset {
	friend AssetLibrary;

	Asset(void* const pAsset, const AssetType type, const AssetStatus status, const char* pPath)
	    : m_pAsset(pAsset)
	    , m_type(type)
	    , m_status(status)
	    , m_path(pPath) {
		if (m_status == AssetStatus::Loaded) {
			sgeAssert(m_pAsset != nullptr);
		}
	}

	const void* asVoid() const { return m_pAsset; }

	AssetTexture* asTextureView() {
		if (getType() == AssetType::TextureView) {
			return (AssetTexture*)m_pAsset;
		}
		return nullptr;
	}

	const AssetTexture* asTextureView() const {
		if (getType() == AssetType::TextureView) {
			return (const AssetTexture*)m_pAsset;
		}
		return nullptr;
	}

	SpriteAnimationAsset* asSprite() {
		if (getType() == AssetType::Sprite) {
			return (SpriteAnimationAsset*)m_pAsset;
		}
		return nullptr;
	}

	const SpriteAnimationAsset* asSprite() const {
		if (getType() == AssetType::Sprite) {
			return (SpriteAnimationAsset*)m_pAsset;
		}
		return nullptr;
	}

	sge::AudioAsset* asAudio() {
		if (getType() == AssetType::Audio) {
			return (AudioAsset*)m_pAsset;
		}
		return nullptr;
	}

	const sge::AudioAsset* asAudio() const {
		if (getType() == AssetType::Audio) {
			return (AudioAsset*)m_pAsset;
		}
		return nullptr;
	}

	AssetModel* asModel() {
		if (getType() == AssetType::Model) {
			return (AssetModel*)m_pAsset;
		}
		return nullptr;
	}

	const AssetModel* asModel() const {
		if (getType() == AssetType::Model) {
			return (AssetModel*)m_pAsset;
		}
		return nullptr;
	}

	const std::string* asText() const {
		if (getType() == AssetType::Text) {
			return (const std::string*)m_pAsset;
		}

		return nullptr;
	}

	AssetType getType() const { return m_type; }
	AssetStatus getStatus() const { return m_status; }
	const std::string& getPath() const { return m_path; }
	sint64 getLastModTime() const { return m_loadedModifiedTime; }

  private:
	void* m_pAsset = nullptr; ///< Holds the dynamically allocated storage of the asset.
	AssetType m_type;         ///< Holds the type of the asset.
	AssetStatus m_status = AssetStatus::NotLoaded;
	std::string m_path;
	sint64 m_loadedModifiedTime; ///< The last time the file was modified since last load.
};

using PAsset = std::shared_ptr<Asset>;
using WAsset = std::weak_ptr<Asset>;

/// @brief AssetLibrary provides a way for loading and tracking used assets of any kind.
struct SGE_CORE_API AssetLibrary {
  public:
	AssetLibrary(SGEDevice* const sgedev);

	/// Make runtime asset
	std::shared_ptr<Asset> makeRuntimeAsset(AssetType type, const char* path);

	// Attempts load an asset. Returns true if new version was found and the asset was reloaded.
	std::shared_ptr<Asset> getAsset(AssetType type, const char* pPath, const bool loadIfMissing);

	/// Retrieves the asset and loading it by guess the Asset Type based on the file extension.
	std::shared_ptr<Asset> getAsset(const char* pPath, const bool loadIfMissing);

	bool loadAsset(Asset* asset);

	// Reloads an asset is the source file modified time has changed.
	bool reloadAssetModified(Asset* const asset);

	IAssetAllocator* getAllocator(const AssetType type) { return m_assetAllocators[type]; }
	IAssetFactory* getFactory(const AssetType type) { return m_assetFactories[type]; }

	SGEDevice* getDevice() { return m_sgedev; }

	std::map<std::string, std::shared_ptr<Asset>>& getAllAssets(AssetType type) {
		const auto& itr = m_assets.find(type);
		sgeAssert(itr != m_assets.end());
		return itr->second;
	}

	const std::map<std::string, std::shared_ptr<Asset>>& getAllAssets(AssetType type) const {
		const auto& itr = m_assets.find(type);
		sgeAssert(itr != m_assets.end());
		return itr->second;
	}

	void scanForAvailableAssets(const char* const path);
	void reloadChangedAssets();

	const std::string& getAssetsDirAbs() const { return m_gameAssetsDir; }

  private:
	std::string m_gameAssetsDir;

	void markThatAssetExists(const char* path, AssetType const type);

	// TODO: Maybe we should make this public in order to support "more" asset types on the go... but who cares!?
	// Registers a new asset type
	void registerAssetType(const AssetType type, IAssetAllocator* const pAllocator, IAssetFactory* const pFactory);

	std::map<AssetType, IAssetAllocator*> m_assetAllocators;
	std::map<AssetType, IAssetFactory*> m_assetFactories;

	// TODO: in order not to automatically unload assets shared_ptr is used. This should be optional. An example emplementation is to
	// use weak_ptr here, and another vector<shared_ptr> that holds those special assets(like characters, menu textures, ect.).
	std::map<AssetType, std::map<std::string, std::shared_ptr<Asset>>> m_assets;

	SGEDevice* m_sgedev;
};

/// @brief Returns true if the specified asset is loaded.
inline bool isAssetLoaded(const Asset& asset) {
	bool loaded = asset.getStatus() == AssetStatus::Loaded;
	return loaded && asset.asVoid() != nullptr;
}

/// @brief Returns true if the specified asset is loaded and if it is of the specified type.
inline bool isAssetLoaded(const Asset& asset, const AssetType type) {
	bool loaded = asset.getStatus() == AssetStatus::Loaded;
	return loaded && asset.asVoid() != nullptr && asset.getType() == type;
}

/// @brief Returns true if the specified asset is loaded.
inline bool isAssetLoaded(const std::shared_ptr<Asset>& asset) {
	bool loaded = asset && (asset->getStatus() == AssetStatus::Loaded);
	if (loaded) {
		sgeAssert(asset && !!asset->asVoid());
	}
	return loaded;
}

/// @brief Returns true if the specified asset is loaded and if it is of the specified type.
inline bool isAssetLoaded(const std::shared_ptr<Asset>& asset, const AssetType type) {
	bool loaded = asset && (asset->getStatus() == AssetStatus::Loaded);
	if (loaded) {
		sgeAssert(asset && !!asset->asVoid());
	}
	return loaded && asset->getType() == type;
}

/// @brief Returns true if the asset is not loaded or loading it failed.
inline bool isAssetNotLoadedOrLoadFailed(const std::shared_ptr<Asset>& asset) {
	const bool notLoadedOrFailed = !asset || (asset->getStatus() == AssetStatus::LoadFailed);
	return notLoadedOrFailed;
}

/// @brief Returns true if the asset is not allocated or if the loading failed.
inline bool isAssetLoadFailed(const std::shared_ptr<Asset>& asset) {
	const bool res = asset && (asset->getStatus() == AssetStatus::LoadFailed);
	return res;
}

} // namespace sge
