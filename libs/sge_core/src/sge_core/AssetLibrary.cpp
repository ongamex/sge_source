#include "AssetLibrary.h"
#include "sge_audio/AudioTrack.h"
#include "sge_core/ICore.h"
#include "sge_core/dds/dds.h"
#include "sge_core/model/EvaluatedModel.h"
#include "sge_core/model/Model.h"
#include "sge_core/model/ModelReader.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/utils/FileStream.h"
#include "sge_utils/utils/Path.h"
#include "sge_utils/utils/json.h"
#include "sge_utils/utils/optional.h"
#include "sge_utils/utils/strings.h"
#include "sge_utils/utils/timer.h"
#include <filesystem>
#include <stb_image.h>

namespace sge {

JsonValue* samplerDesc_toJson(const SamplerDesc& desc, JsonValueBuffer& jvb) {
	const auto addressModeToString = [](TextureAddressMode::Enum adressMode) -> const char* {
		switch (adressMode) {
			case TextureAddressMode::Repeat:
				return "repeat";
			case TextureAddressMode::ClampEdge:
				return "edge";
			case TextureAddressMode::ClampBorder:
				return "border";
			default:
				sgeAssert(false);
				return "";
				break;
		}
	};

	const auto filterToString = [](TextureFilter::Enum filter) -> const char* {
		switch (filter) {
			case sge::TextureFilter::Min_Mag_Mip_Point:
				return "point";
				break;
			case sge::TextureFilter::Min_Mag_Mip_Linear:
				return "linear";
				break;
			case sge::TextureFilter::Anisotropic:
				return "anisotropic";
				break;
			default:
				sgeAssert(false);
				return "";
				break;
		}
	};

	auto jRoot = jvb(JID_MAP);

	jRoot->setMember("addressModeX", jvb(addressModeToString(desc.addressModes[0])));
	jRoot->setMember("addressModeY", jvb(addressModeToString(desc.addressModes[1])));
	jRoot->setMember("addressModeZ", jvb(addressModeToString(desc.addressModes[2])));
	jRoot->setMember("filter", jvb(filterToString(desc.filter)));

	return jRoot;
}

Optional<SamplerDesc> samplerDesc_fromJson(const JsonValue& jRoot) {
	auto addressModeFromStringOrThrow = [](const char* str) -> TextureAddressMode::Enum {
		if (str == nullptr || str[0] == '\0') {
			throw std::exception();
		}

		if (strcmp("repeat", str) == 0)
			return TextureAddressMode::Repeat;
		if (strcmp("edge", str) == 0)
			return TextureAddressMode::ClampEdge;
		if (strcmp("border", str) == 0)
			return TextureAddressMode::ClampBorder;

		throw std::exception();
	};

	auto filterFormStringOrThrow = [](const char* str) -> TextureFilter::Enum {
		if (str == nullptr || str[0] == '\0') {
			throw std::exception();
		}

		if (strcmp("point", str) == 0)
			return TextureFilter::Min_Mag_Mip_Point;

		if (strcmp("linear", str) == 0)
			return TextureFilter::Min_Mag_Mip_Linear;

		if (strcmp("anisotropic", str) == 0)
			return TextureFilter::Anisotropic;

		throw std::exception();
	};

	try {
		const char* const addressModeXStr = jRoot.getMemberOrThrow("addressModeX").GetStringOrThrow();
		const char* const addressModeYStr = jRoot.getMemberOrThrow("addressModeY").GetStringOrThrow();
		const char* const addressModeZStr = jRoot.getMemberOrThrow("addressModeZ").GetStringOrThrow();
		const char* const filterStr = jRoot.getMemberOrThrow("filter").GetStringOrThrow();

		SamplerDesc result;

		result.addressModes[0] = addressModeFromStringOrThrow(addressModeXStr);
		result.addressModes[1] = addressModeFromStringOrThrow(addressModeYStr);
		result.addressModes[2] = addressModeFromStringOrThrow(addressModeZStr);
		result.filter = filterFormStringOrThrow(filterStr);

		return result;
	} catch (...) {
		sgeAssert(false);
		return NullOptional();
	}
}

SGE_CORE_API const char* assetType_getName(const AssetType type) {
	switch (type) {
		case AssetType::None:
			return "None";
		case AssetType::Model:
			return "3D Model";
		case AssetType::Texture2D:
			return "Texture";
		case AssetType::Text:
			return "Text";
		case AssetType::Sprite:
			return "Sprite";
		case AssetType::Audio:
			return "Vorbis Audio";
		default:
			sgeAssertFalse("Not implemented");
			return "NotImplemented";
	}
}

bool AssetTexture::saveTextureSettingsToInfoFile(Asset& assetSelf) {
	// Ensure that the user passed the right asset.
	if (assetSelf.asTextureView() != this) {
		sgeAssert(false && "It seems that thiis AssetTexture is not owned by assetSelf");
		return false;
	}

	// [TEXTURE_ASSET_INFO]
	const std::string infoPath = assetSelf.getPath() + ".info";

	JsonValueBuffer jvb;
	auto jRoot = jvb(JID_MAP);
	jRoot->setMember("version", jvb(1));
	jRoot->setMember("sampler", samplerDesc_toJson(assetSamplerDesc, jvb));

	JsonWriter jsonWriter;
	bool success = jsonWriter.WriteInFile(infoPath.c_str(), jRoot, true);
	return success;
}

SamplerDesc AssetTexture::loadTextureSettingInfoFile(const std::string& baseAssetPath) {
	// [TEXTURE_ASSET_INFO]
	const std::string infoPath = baseAssetPath + ".info";

	FileReadStream frs;
	if (!frs.open(infoPath.c_str())) {
		// No info file, just use the defaults.
		return SamplerDesc();
	}

	// Parse the json inside that file.
	JsonParser jp;
	if (!jp.parse(&frs)) {
		// No info file, just use the defaults.
		sgeAssert(false && "Parsing the texture info file, that contains additional settings for a texture asset cannot be parsed!");
		return SamplerDesc();
	}

	try {
		auto jRoot = jp.getRoot();

		int version = jRoot->getMemberOrThrow("version").getNumberAsOrThrow<int>();

		if (version == 1) {
			// Default version, nothing special in it.
			auto jSampler = jRoot->getMemberOrThrow("sampler", JID_MAP);
			Optional<SamplerDesc> samplerDesc = samplerDesc_fromJson(jSampler);
			if (samplerDesc) {
				return *samplerDesc;
			}
		}
	} catch (...) {
	}

	sgeAssert(false && "Parsing the texture info file, that contains additional settings for a texture asset cannot be parsed!");
	return SamplerDesc();
}

AssetType assetType_guessFromExtension(const char* const ext, bool includeExternalExtensions) {
	if (ext == nullptr) {
		return AssetType::None;
	}

	if (sge_stricmp(ext, "mdl") == 0) {
		return AssetType::Model;
	}

	if (includeExternalExtensions && sge_stricmp(ext, "fbx") == 0) {
		return AssetType::Model;
	}

	if (includeExternalExtensions && sge_stricmp(ext, "dae") == 0) {
		return AssetType::Model;
	}

	if (includeExternalExtensions && sge_stricmp(ext, "obj") == 0) {
		return AssetType::Model;
	}

	if (sge_stricmp(ext, "png") == 0 || sge_stricmp(ext, "dds") == 0 || sge_stricmp(ext, "jpg") == 0) {
		return AssetType::Texture2D;
	}

	if (sge_stricmp(ext, "txt") == 0) {
		return AssetType::Text;
	}

	if (sge_stricmp(ext, "sprite") == 0) {
		return AssetType::Sprite;
	}

	if (sge_stricmp(ext, "ogg") == 0) {
		return AssetType::Audio;
	}

	return AssetType::None;
}

template <typename T>
struct TAssetAllocatorDefault : public IAssetAllocator {
	void* allocate() final {
		data.push_back(new T);
		return data.back();
	}

	void deallocate(void* ptr) final {
		auto itr = std::find(data.begin(), data.end(), ptr);

		if (itr != data.end()) {
			delete *itr;
			data.erase(itr);
		} else {
			// TODO; now wut?
			sgeAssert(false);
		}
	}

	std::vector<T*> data;
};

//-------------------------------------------------------
// ModelAssetFactory
//-------------------------------------------------------
struct ModelAssetFactory : public IAssetFactory {
	bool load(void* const pAsset, const char* const pPath, AssetLibrary* const pMngr) final {
		sgeAssert(pAsset != nullptr);
		sgeAssert(pPath != nullptr);
		sgeAssert(pMngr != nullptr);

		AssetModel& modelAsset = *(AssetModel*)(pAsset);

		FileReadStream frs(pPath);

		if (frs.isOpened() == false) {
			SGE_DEBUG_ERR("Unable to find model asset: '%s'!\n", pPath);
			// sgeAssert(false);
			return false;
		}

		ModelLoadSettings loadSettings;
		loadSettings.assetDir = extractFileDir(pPath, true);

		ModelReader modelReader;
		const bool succeeded = modelReader.loadModel(loadSettings, &frs, modelAsset.model);

		if (!succeeded) {
			SGE_DEBUG_ERR("Unable to load model asset: '%s'!\n", pPath);
			// sgeAssert(false);
			return false;
		}

		modelAsset.model.createRenderingResources(*pMngr->getDevice());

		modelAsset.staticEval.initialize(pMngr, &modelAsset.model);
		modelAsset.staticEval.evaluateStatic();

		modelAsset.sharedEval.initialize(pMngr, &modelAsset.model);
		modelAsset.sharedEval.evaluateStatic();

		return succeeded;
	}

	void unload(void* const pAsset, [[maybe_unused]] AssetLibrary* const pMngr) final {
		sgeAssert(pAsset != nullptr);
		sgeAssert(pMngr != nullptr);

		AssetModel& model = *(AssetModel*)(pAsset);

		model.staticEval = EvaluatedModel();
		model.sharedEval = EvaluatedModel();

		// TOOD: Should we do something here?
	}
};

//-------------------------------------------------------
// TextureViewAssetFactory
//-------------------------------------------------------
struct TextureViewAssetFactory : public IAssetFactory {
	enum DDSLoadCode {
		ddsLoadCode_fine = 0,
		ddsLoadCode_fileDoesntExist,
		ddsLoadCode_importOrCreationFailed,
	};

	SamplerDesc getTextureSamplerDesc(const char* const pAssetPath) const { return AssetTexture::loadTextureSettingInfoFile(pAssetPath); }

	// Check if the file version in DDS already exists, if not or the import fails the function returns false;
	DDSLoadCode loadDDS(void* const pAsset, const char* const pPath, AssetLibrary* const pMngr) {
		std::string const ddsPath = (extractFileExtension(pPath) == "dds") ? pPath : std::string(pPath) + ".dds";

		// Load the File contents.
		std::vector<char> ddsDataRaw;
		if (FileReadStream::readFile(ddsPath.c_str(), ddsDataRaw) == false) {
			return ddsLoadCode_fileDoesntExist;
		}


		// Parse the file and generate the texture creation strctures.
		DDSLoader loader;
		TextureDesc desc;
		std::vector<TextureData> initalData;

		if (loader.load(ddsDataRaw.data(), ddsDataRaw.size(), desc, initalData) == false) {
			return ddsLoadCode_importOrCreationFailed;
		}

		// Create the texture.
		AssetTexture& texture = *(AssetTexture*)(pAsset);
		texture.tex = pMngr->getDevice()->requestResource<Texture>();

		texture.assetSamplerDesc = getTextureSamplerDesc(pPath);
		bool const createSucceeded = texture.tex->create(desc, &initalData[0], texture.assetSamplerDesc);

		if (createSucceeded == false) {
			texture.tex.Release();
			return ddsLoadCode_importOrCreationFailed;
		}

		return ddsLoadCode_fine;
	}

	bool load(void* const pAsset, const char* const pPath, AssetLibrary* const pMngr) final {
		sgeAssert(pAsset != nullptr);
		sgeAssert(pPath != nullptr);
		sgeAssert(pMngr != nullptr);

#if !defined(__EMSCRIPTEN__)
		DDSLoadCode const ddsLoadStatus = loadDDS(pAsset, pPath, pMngr);

		if (ddsLoadStatus == ddsLoadCode_fine) {
			return true;
		} else if (ddsLoadStatus == ddsLoadCode_importOrCreationFailed) {
			SGE_DEBUG_ERR("Failed to load the DDS equivalent to '%s'!\n", pPath);
			return false;
		}
#endif

		// If we are here than the DDS file doesn't exist and
		// we must try to load the exact file that we were asked for.
		AssetTexture& texture = *(AssetTexture*)(pAsset);

		// Now check for the actual asset that is requested.
		FileReadStream frs(pPath);
		if (!frs.isOpened()) {
			SGE_DEBUG_ERR("Unable to find texture view asset: '%s'!\n", pPath);
			return false;
		}

		int width, height, components;
		const unsigned char* textureData = stbi_load(pPath, &width, &height, &components, 4);

		TextureDesc textureDesc;

		textureDesc.textureType = UniformType::Texture2D;
		textureDesc.format = TextureFormat::R8G8B8A8_UNORM;
		textureDesc.usage = TextureUsage::ImmutableResource;
		textureDesc.texture2D.arraySize = 1;
		textureDesc.texture2D.numMips = 1;
		textureDesc.texture2D.numSamples = 1;
		textureDesc.texture2D.sampleQuality = 0;
		textureDesc.texture2D.width = width;
		textureDesc.texture2D.height = height;

		TextureData textureDataDesc;
		textureDataDesc.data = textureData;
		textureDataDesc.rowByteSize = width * 4;

		texture.tex = pMngr->getDevice()->requestResource<Texture>();

		const SamplerDesc samplerDesc = getTextureSamplerDesc(pPath);
		texture.tex->create(textureDesc, &textureDataDesc, samplerDesc);

		if (textureData != nullptr) {
			stbi_image_free((void*)textureData);
			textureData = nullptr;
		}

		const bool succeeded = texture.tex.IsResourceValid();

		return succeeded;
	}

	void unload([[maybe_unused]] void* const pAsset, [[maybe_unused]] AssetLibrary* const pMngr) final {
		sgeAssert(pAsset != nullptr);
		sgeAssert(pMngr != nullptr);

		// TOOD: Should we do something here?
	}
};

//-------------------------------------------------------
// TextAssetFactory
//-------------------------------------------------------
struct TextAssetFactory : public IAssetFactory {
	bool load(void* const pAsset, const char* const pPath, AssetLibrary* const UNUSED(pMngr)) final {
		std::string& text = *(std::string*)(pAsset);

		std::vector<char> fileContents;
		if (!FileReadStream::readFile(pPath, fileContents)) {
			return false;
		}

		fileContents.push_back('\0');
		text = fileContents.data();

		return true;
	}

	void unload(void* const pAsset, AssetLibrary* const UNUSED(pMngr)) final {
		std::string& text = *(std::string*)(pAsset);
		text = std::string();
	}
};

//-------------------------------------------------------
// SpriteAssetFactory
//-------------------------------------------------------
struct SpriteAssetFactory : public IAssetFactory {
	bool load(void* const pAsset, const char* const pPath, AssetLibrary* const assetLib) final {
		SpriteAnimationAsset& sprite = *(SpriteAnimationAsset*)(pAsset);
		const bool success = SpriteAnimationAsset::importSprite(sprite, pPath, *assetLib);
		return success;
	}

	void unload(void* const pAsset, AssetLibrary* const UNUSED(pMngr)) final {
		std::string& text = *(std::string*)(pAsset);
		text = std::string();
	}
};

//-------------------------------------------------------
// AudioAssetFactory
//-------------------------------------------------------
struct AudioAssetFactory : public IAssetFactory {
	bool load(void* const pAsset, const char* const pPath, AssetLibrary* const UNUSED(assetLib)) final {
		AudioAsset& audio = *reinterpret_cast<AudioAsset*>(pAsset);

		std::vector<char> fileContents;
		if (!FileReadStream::readFile(pPath, fileContents)) {
			return false;
		}
		audio.reset(new AudioTrack(std::move(fileContents)));
		return true;
	}

	void unload(void* const pAsset, AssetLibrary* const UNUSED(pMngr)) final {
		AudioAsset& audio = *reinterpret_cast<AudioAsset*>(pAsset);
		audio.reset();
	}
};

//-------------------------------------------------------
// AssetLibrary
//-------------------------------------------------------
AssetLibrary::AssetLibrary(SGEDevice* const sgedev) {
	m_sgedev = sgedev;

	// Register all supported asset types.
	this->registerAssetType(AssetType::Model, new TAssetAllocatorDefault<AssetModel>(), new ModelAssetFactory());
	this->registerAssetType(AssetType::Texture2D, new TAssetAllocatorDefault<AssetTexture>(), new TextureViewAssetFactory());
	this->registerAssetType(AssetType::Text, new TAssetAllocatorDefault<std::string>(), new TextAssetFactory());
	this->registerAssetType(AssetType::Sprite, new TAssetAllocatorDefault<SpriteAnimationAsset>(), new SpriteAssetFactory());
	this->registerAssetType(AssetType::Audio, new TAssetAllocatorDefault<sge::AudioAsset>(), new AudioAssetFactory());
}

void AssetLibrary::registerAssetType(const AssetType type, IAssetAllocator* const pAllocator, IAssetFactory* const pFactory) {
	sgeAssert(pAllocator != nullptr);
	sgeAssert(pFactory != nullptr);

	m_assetAllocators[type] = pAllocator;
	m_assetFactories[type] = pFactory;
	// CAUTION: This is here to ensure that the element "type" is present in "std::map<...> m_assets" from nwo on. This is pretty stupid and
	// has to be fixed.
	m_assets[type];
}

std::shared_ptr<Asset> AssetLibrary::makeRuntimeAsset(AssetType type, const char* path) {
	IAssetAllocator* const pAllocator = getAllocator(type);
	if (!pAllocator) {
		return nullptr;
	}

	// Check if the asset already exists.
	std::map<std::string, std::shared_ptr<Asset>>& assets = m_assets[type];

	auto findItr = assets.find(path);
	if (findItr != assets.end() && isAssetLoaded(findItr->second)) {
		sgeAssertFalse("Asset with the same path already exists");
		// The asset already exists, we cannot make a new one.
		return nullptr;
	}

	void* pAsset = pAllocator->allocate();
	if (pAsset == nullptr) {
		sgeAssertFalse("Failed to allocate the asset");
		return nullptr;
	}

	std::shared_ptr<Asset> result = std::make_shared<Asset>(pAsset, type, AssetStatus::Loaded, path);
	assets[path] = result;

	return result;
}

std::shared_ptr<Asset> AssetLibrary::getAsset(AssetType type, const char* pPath, const bool loadIfMissing) {
	if (!pPath || pPath[0] == '\0') {
		sgeAssert(false);
		return std::shared_ptr<Asset>();
	}

	if (AssetType::None == type) {
		return std::shared_ptr<Asset>();
	}

	const double loadStartTime = Timer::now_seconds();

	// Now make the path relative to the current directory, as some assets
	// might refer it relative to them, and this wolud lead us loading the same asset
	// via different path and we don't want that.
	std::error_code pathToAssetRelativeError;
	const std::filesystem::path pathToAssetRelative = std::filesystem::relative(pPath, pathToAssetRelativeError);

	// The commented code below, not only makes the path cannonical, but it also makes it absolute, which we don't want.
	// std::error_code pathToAssetCanonicalError;
	// const std::filesystem::path pathToAssetCanonical = std::filesystem::weakly_canonical(pathToAssetRelative, pathToAssetCanonicalError);

	// canonizePathRespectOS makes makes the slashes UNUX Style.
	const std::string pathToAsset = canonizePathRespectOS(pathToAssetRelative.string());

	if (pathToAssetRelativeError) {
		sgeAssert(false && "Failed to transform the asset path to relative");
	}

	if (pathToAsset.empty()) {
		// Because std::filesystem::canonical() returns empty string if the path doesn't exists
		// we assume that the loading failed.
		return std::shared_ptr<Asset>();
	}

	// Check if the asset already exists.
	std::map<std::string, std::shared_ptr<Asset>>& assets = m_assets[type];

	auto findItr = assets.find(pathToAsset);
	if (findItr != assets.end() && isAssetLoaded(findItr->second)) {
		return findItr->second;
	}

	if (!loadIfMissing) {
		// Empty asset shared ptr.
		// TODO: Should I create an empty asset to that path with unknown state? It sounds logical?
		return findItr != assets.end() ? findItr->second : std::shared_ptr<Asset>();
	}

	// Load the asset.
	IAssetAllocator* const pAllocator = getAllocator(type);
	IAssetFactory* const pFactory = getFactory(type);

	if (!pAllocator || !pFactory) {
		sgeAssert(false && "Cannot lode an asset of the specified type");
		return std::shared_ptr<Asset>();
	}

	void* pAsset = pAllocator->allocate();

	const bool succeeded = pFactory->load(pAsset, pathToAsset.c_str(), this);
	if (succeeded == false) {
		SGE_DEBUG_ERR("Failed on asset %s\n", pPath);
		pAllocator->deallocate(pAsset);
		pAsset = NULL;
	}

	std::shared_ptr<Asset> result;

	if (findItr == assets.end()) {
		// Add the asset to the library.
		std::shared_ptr<Asset> asset =
		    std::make_shared<Asset>(pAsset, type, (pAsset) ? AssetStatus::Loaded : AssetStatus::LoadFailed, pathToAsset.c_str());

		asset->m_loadedModifiedTime = FileReadStream::getFileModTime(pPath);

		assets[pathToAsset] = asset;
		result = asset;
	} else {
		sgeAssert(findItr->second->asVoid() == nullptr);
		*findItr->second = Asset(pAsset, type, (pAsset) ? AssetStatus::Loaded : AssetStatus::LoadFailed, pathToAsset.c_str());
		findItr->second->m_loadedModifiedTime = FileReadStream::getFileModTime(pPath);
		result = findItr->second;
	}

	// Measure the loading time.
	const float loadEndTime = Timer::now_seconds();
	SGE_DEBUG_LOG("Asset '%s' loaded in %f seconds.\n", pathToAsset.c_str(), loadEndTime - loadStartTime);

	return result;
}

std::shared_ptr<Asset> AssetLibrary::getAsset(const char* pPath, bool loadIfMissing) {
	AssetType assetType = assetType_guessFromExtension(extractFileExtension(pPath).c_str(), false);
	return getAsset(assetType, pPath, loadIfMissing);
}

bool AssetLibrary::loadAsset(Asset* asset) {
	if (asset) {
		return getAsset(asset->getType(), asset->getPath().c_str(), true) != nullptr;
	}

	return false;
}



bool AssetLibrary::reloadAssetModified(Asset* const asset) {
	if (!asset) {
		sgeAssert(false);
		return false;
	}

	if (asset->getStatus() != AssetStatus::Loaded && asset->getStatus() != AssetStatus::LoadFailed) {
		return false;
	}

	const sint64 fileNewModTime = FileReadStream::getFileModTime(asset->getPath().c_str());

	if (asset->getLastModTime() == fileNewModTime) {
		return false;
	}

	const double reloadStartTime = Timer::now_seconds();

	IAssetFactory* const pFactory = getFactory(asset->getType());
	sgeAssert(pFactory);

	std::string pathToAsset = asset->getPath();

	pFactory->unload(asset->m_pAsset, this);
	bool const succeeded = pFactory->load(asset->m_pAsset, pathToAsset.c_str(), this);

	if (!succeeded) {
		sgeAssert(false);
		return false;
	}

	// Measure the loading time.
	const float reloadEndTime = Timer::now_seconds();
	SGE_DEBUG_LOG("Asset '%s' loaded in %f seconds.\n", pathToAsset.c_str(), reloadEndTime - reloadStartTime);

	return true;
}

void AssetLibrary::scanForAvailableAssets(const char* const path) {
	using namespace std;

	m_gameAssetsDir = absoluteOf(path);
	sgeAssert(m_gameAssetsDir.empty() == false);

	if (filesystem::is_directory(path)) {
		for (const filesystem::directory_entry& entry : filesystem::recursive_directory_iterator(path)) {
			if (entry.status().type() == filesystem::file_type::regular) {
				// Extract the extension of the found file use it for guessing the type of the asset,
				// then mark the asset as existing without loading it.
				const std::string ext = entry.path().extension().u8string();
				const char* extCStr = ext.c_str();

				// the extension() method returns the dot, however assetType_guessFromExtension
				// needs the extension without the dot.
				if (extCStr != nullptr && extCStr[0] == '.') {
					extCStr++;

					const AssetType guessedType = assetType_guessFromExtension(extCStr, false);
					if (guessedType != AssetType::None) {
						markThatAssetExists(entry.path().generic_u8string().c_str(), guessedType);
					}
				}
			}
		}
	}
}

void AssetLibrary::reloadChangedAssets() {
	for (auto& assetsPerType : m_assets) {
		for (auto& assetPair : assetsPerType.second) {
			std::shared_ptr<Asset>& asset = assetPair.second;
			reloadAssetModified(asset.operator->());
		}
	}
}

void AssetLibrary::markThatAssetExists(const char* path, AssetType const type) {
	if (!path || path[0] == '\0') {
		return;
	}

	std::map<std::string, std::shared_ptr<Asset>>& assets = m_assets[type];
	std::shared_ptr<Asset>& asset = assets[path];

	// Check if the asset is allocaded.
	if (asset) {
		return;
	}

	asset = std::make_shared<Asset>(nullptr, type, AssetStatus::NotLoaded, canonizePathRespectOS(path).c_str());
}

} // namespace sge
