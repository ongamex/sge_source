#pragma once

#include <functional>
#include <memory>
#include <string>

#include "sge_core/AssetLibrary.h"
#include "sge_engine/sge_engine_api.h"

namespace sge {

struct ObjectId;
struct GameWorld;
struct Actor;
struct Asset;

SGE_ENGINE_API bool assetPicker(
    const char* label, std::string& assetPath, AssetLibrary* const assetLibrary, const AssetType assetTypes[], const int numAssetTypes);

SGE_ENGINE_API bool assetPicker(const char* label,
                                std::shared_ptr<Asset>& asset,
                                AssetLibrary* const assetLibrary,
                                const AssetType assetTypes[],
                                const int numAssetTypes);

SGE_ENGINE_API bool actorPicker(const char* label,
                                GameWorld& world,
                                ObjectId& ioValue,
                                std::function<bool(const GameObject&)> filter = nullptr,
                                bool pickPrimarySelection = false);
SGE_ENGINE_API bool gameObjectTypePicker(const char* label, TypeId& ioValue, const TypeId needsToInherit = TypeId());

} // namespace sge
