#include "AssetProperty.h"
#include "sge_core/ICore.h"
#include "sge_engine/TypeRegister.h"

namespace sge {

bool AssetProperty::update() {
	if (isUpToDate()) {
		return false;
	}

	m_currentAsset = m_targetAsset;
	if (m_currentAsset.empty() == false) {
		m_asset = getCore()->getAssetLib()->getAsset(m_currentAsset.c_str(), true);
	} else {
		m_asset = std::shared_ptr<Asset>();
	}

	return true;
}

AssetModel* AssetProperty::getAssetModel() {
	if (isAssetLoaded(m_asset) == false || m_asset->getType() != AssetType::Model) {
		return nullptr;
	}

	return m_asset->asModel();
}

const AssetModel* AssetProperty::getAssetModel() const {
	if (!isAssetLoaded(m_asset, AssetType::Model)) {
		return nullptr;
	}
	return m_asset->asModel();
}

AssetTexture* AssetProperty::getAssetTexture() {
	if (!isAssetLoaded(m_asset, AssetType::TextureView)) {
		return nullptr;
	}

	return m_asset->asTextureView();
}

const AssetTexture* AssetProperty::getAssetTexture() const {
	if (!isAssetLoaded(m_asset, AssetType::TextureView)) {
		return nullptr;
	}
	return m_asset->asTextureView();
}

SpriteAnimationAsset* AssetProperty::getAssetSprite() {
	if (!isAssetLoaded(m_asset, AssetType::Sprite)) {
		return nullptr;
	}

	return m_asset->asSprite();
}

const SpriteAnimationAsset* AssetProperty::getAssetSprite() const {
	if (!isAssetLoaded(m_asset, AssetType::Sprite)) {
		return nullptr;
	}

	return m_asset->asSprite();
}

AudioAsset* AssetProperty::getAssetAudio() {
	if (isAssetLoaded(m_asset) == false || m_asset->getType() != AssetType::Audio) {
		return nullptr;
	}

	return m_asset->asAudio();
}

const AudioAsset* AssetProperty::getAssetAudio() const {
	if (isAssetLoaded(m_asset) == false || m_asset->getType() != AssetType::Audio) {
		return nullptr;
	}

	return m_asset->asAudio();
}

void AssetProperty::setAsset(std::shared_ptr<Asset>& asset) {
	m_asset = asset;
	m_targetAsset = asset->getPath();
	m_currentAsset = m_asset->getPath();
}

void AssetProperty::setTargetAsset(const char* const assetPath) {
	m_targetAsset = assetPath ? assetPath : "";
}

DefineTypeId(AssetProperty, 20'03'01'0001);
ReflBlock() {
	ReflAddType(AssetProperty) ReflMember(AssetProperty, m_targetAsset);
}

} // namespace sge
