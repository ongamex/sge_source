#pragma once

#include "sge_utils/math/vec4.h"
#include "sgecore_api.h"
#include <memory>
#include <string>
#include <vector>

namespace sge {

struct AssetLibrary;
struct Asset;

struct SGE_CORE_API SpriteAnimation {
	/// @brief Frame describes a region for a texture that contains a sub image.
	struct Frame {
		vec2i xy = vec2i(0);         /// The position of the 1st pixel of the frame.
		vec2i wh = vec2i(0);         /// The width and height of the region of the frame starting form "xy".
		vec4f uvRegion = vec4f(0.f); /// The uv region in the source texture to be used. (x,y) top-left, (w,z) bottom-right
		float duration = 0.f;        /// The duration of the frame in seconds.
		float frameStart = 0.f;      /// The time offset in the global animation when this frame should appear.
	};

	/// @brief Imports a Sprite Sheet form our internal format.
	/// @param [out] outSprite hold the imported sprite
	/// @param [in] filename the path to the json file to be imported.
	/// @param [in] assetLib the asset library needed to loaded the image texture.
	/// @return true if succeeded.
	static bool importSprite(SpriteAnimation& outSprite, const char* const filename);

	/// @brief Imports a Sprite Sheet form Asperite exported json. Expects that the json is in "array" format.
	/// @param [out] outSprite hold the imported sprite
	/// @param [in] filename the path to the json file to be imported.
	/// @return true if succeeded.
	static bool importFromAsepriteSpriteSheetJsonFile(SpriteAnimation& outSprite, const char* const filename);

	/// @brief Saves the specified sprite as our internal format to a file.
	/// @param path the path to the file that will contain the file.
	/// @return true if succeeded
	bool saveSpriteToFile(const char* path) const;

	/// @brief Retrieves a data pointer to the frame for the specified time.
	/// May be nullptr if no frames are present.
	const Frame* getFrameForTime(float time) const;

  public:
	std::string texturePath;       /// The texture that holds the frames of the sprite.
	std::vector<Frame> frames;     /// The available frames.
	float animationDuration = 0.f; /// The duration of the animation in seconds.
};

struct SpriteAnimationAsset {
	/// @brief Imports a Sprite Sheet form our internal format.
	/// @param [out] outSprite hold the imported sprite
	/// @param [in] filename the path to the json file to be imported.
	/// @param [in] assetLib the asset library needed to loaded the image texture.
	/// @return true if succeeded.
	static bool importSprite(SpriteAnimationAsset& outSprite, const char* const filename, AssetLibrary& assetLib);

	/// @brief Imports a Sprite Sheet form Asperite exported json. Expects that the json is in "array" format.
	/// @param [out] outSprite hold the imported sprite
	/// @param [in] filename the path to the json file to be imported.
	/// @param [in] assetLib the asset library needed to loaded the image texture.
	/// @return true if succeeded.
	static bool importFromAsepriteSpriteSheetJsonFile(SpriteAnimationAsset& outSprite, const char* const filename, AssetLibrary& assetLib);

	SpriteAnimation spriteAnimation;
	std::shared_ptr<Asset> textureAsset; /// The texture that holds the frames of the sprite.
};

} // namespace sge
