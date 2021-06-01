#include "Sprite.h"
#include "sge_core/AssetLibrary.h"
#include "sge_utils/utils/FileStream.h"
#include "sge_utils/utils/Path.h"
#include "sge_utils/utils/json.h"

namespace sge {

bool SpriteAnimation::importSprite(SpriteAnimation& outSprite, const char* const filename) {
	outSprite = SpriteAnimation();

	FileReadStream frs;
	frs.open(filename);
	if (!frs.open(filename)) {
		return false;
	}

	JsonParser jp;
	if (jp.parse(&frs) == false) {
		return false;
	}

	const JsonValue* const jRoot = jp.getRoot();

	outSprite.animationDuration = jRoot->getMember("duration")->getNumberAs<float>();
	outSprite.texturePath = jRoot->getMember("texturePath")->GetString();

	auto jFrames = jRoot->getMember("frames");
	outSprite.frames.resize(jFrames->arrSize());
	for (int t = 0; t < jFrames->arrSize(); ++t) {
		auto jFrame = jFrames->arrAt(t);

		outSprite.frames[t].xy.x = jFrame->getMember("x")->getNumberAs<int>();
		outSprite.frames[t].xy.y = jFrame->getMember("y")->getNumberAs<int>();
		outSprite.frames[t].wh.x = jFrame->getMember("w")->getNumberAs<int>();
		outSprite.frames[t].wh.y = jFrame->getMember("h")->getNumberAs<int>();

		outSprite.frames[t].uvRegion.x = jFrame->getMember("uvTopLeftX")->getNumberAs<float>();
		outSprite.frames[t].uvRegion.y = jFrame->getMember("uvTopLeftY")->getNumberAs<float>();
		outSprite.frames[t].uvRegion.z = jFrame->getMember("uvBottomRightX")->getNumberAs<float>();
		outSprite.frames[t].uvRegion.w = jFrame->getMember("uvBottomRightY")->getNumberAs<float>();

		outSprite.frames[t].duration = jFrame->getMember("duration")->getNumberAs<float>();
		outSprite.frames[t].frameStart = jFrame->getMember("frameTimeStart")->getNumberAs<float>();
	}

	return true;
}

bool SpriteAnimation::importFromAsepriteSpriteSheetJsonFile(SpriteAnimation& outSprite, const char* const filename) {
	try {
		outSprite = SpriteAnimation();

		FileReadStream frs;
		frs.open(filename);
		if (!frs.open(filename)) {
			return false;
		}

		JsonParser jp;
		if (jp.parse(&frs) == false) {
			return false;
		}

		const JsonValue* const jRoot = jp.getRoot();
		const JsonValue* const jMeta = jRoot->getMember("meta");

		if (!jRoot || !jMeta) {
			return false;
		}

		outSprite.texturePath = jMeta->getMemberOrThrow("image").GetStringOrThrow();

		auto jMetaSize = jMeta->getMemberOrThrow("size");
		float fFullSheetWidth = jMetaSize.getMemberOrThrow("w").getNumberAsOrThrow<float>();
		float fFullSheetHeight = jMetaSize.getMemberOrThrow("h").getNumberAsOrThrow<float>();

		const JsonValue* const jFrames = jRoot->getMember("frames");

		if (!jFrames || jFrames->jid != JID_ARRAY) {
			sgeAssert("Importing Aseprite Sprite Sheets in 'hash' mode is not supported. Use 'array'");
			return false;
		}

		float totalAnimationDuration = 0.f;
		for (int iFrame = 0; iFrame < jFrames->arrSize(); ++iFrame) {
			const JsonValue* const jFrame = jFrames->arrAt(iFrame);
			if (!jFrame) {
				return false;
			}

			const JsonValue& jRegion = jFrame->getMemberOrThrow("frame");
			const int x = jRegion.getMemberOrThrow("x").getNumberAsOrThrow<int>();
			const int y = jRegion.getMemberOrThrow("y").getNumberAsOrThrow<int>();
			const int w = jRegion.getMemberOrThrow("w").getNumberAsOrThrow<int>();
			const int h = jRegion.getMemberOrThrow("h").getNumberAsOrThrow<int>();

			Frame frame;
			frame.xy = vec2i(x, y);
			frame.wh = vec2i(w, h);
			frame.uvRegion =
			    vec4f(float(x) / fFullSheetWidth, float(y) / fFullSheetHeight, float(w) / fFullSheetWidth, float(h) / fFullSheetHeight);
			frame.uvRegion.z += frame.uvRegion.x;
			frame.uvRegion.w += frame.uvRegion.y;
			frame.frameStart = totalAnimationDuration;
			// Aseprite store duration in miliseconds while we need seconds.
			frame.duration = (float)(jFrame->getMemberOrThrow("duration").getNumberAsOrThrow<int>()) / 1000.f;

			outSprite.frames.push_back(frame);

			totalAnimationDuration += frame.duration;
		}

		outSprite.animationDuration = totalAnimationDuration;
	} catch (...) {
		return false;
	}

	return true;
}

bool SpriteAnimation::saveSpriteToFile(const char* path) const {
	JsonWriter jw;
	JsonValueBuffer jvb;

	auto jRoot = jvb(JID_MAP);
	jRoot->setMember("version", jvb(1));
	jRoot->setMember("duration", jvb(animationDuration));
	jRoot->setMember("texturePath", jvb(texturePath));

	auto jFrames = jRoot->setMember("frames", jvb(JID_ARRAY));
	for (const Frame& f : frames) {
		auto jFrame = jFrames->arrPush(jvb(JID_MAP));
		jFrame->setMember("x", jvb(f.xy.x));
		jFrame->setMember("y", jvb(f.xy.y));
		jFrame->setMember("w", jvb(f.wh.x));
		jFrame->setMember("h", jvb(f.wh.y));

		jFrame->setMember("uvTopLeftX", jvb(f.uvRegion.x));
		jFrame->setMember("uvTopLeftY", jvb(f.uvRegion.y));
		jFrame->setMember("uvBottomRightX", jvb(f.uvRegion.z));
		jFrame->setMember("uvBottomRightY", jvb(f.uvRegion.w));
		jFrame->setMember("duration", jvb(f.duration));
		jFrame->setMember("frameTimeStart", jvb(f.frameStart));
	}

	bool succeeded = jw.WriteInFile(path, jRoot, true);
	return succeeded;
}

const SpriteAnimation::Frame* SpriteAnimation::getFrameForTime(float time) const {
	if (frames.size() == 0) {
		return nullptr;
	}

	const Frame* result = &frames[0];
	for (const Frame& frm : frames) {
		const float frameEndTime = frm.frameStart + frm.duration;
		if (time >= frm.frameStart) {
			result = &frm;
		}
	}

	return result;
}


bool SpriteAnimationAsset::importSprite(SpriteAnimationAsset& outSprite, const char* const filename, AssetLibrary& assetLib) {
	if (SpriteAnimation::importSprite(outSprite.spriteAnimation, filename)) {
		outSprite.textureAsset = assetLib.getAsset(AssetType::Texture2D, outSprite.spriteAnimation.texturePath.c_str(), true);
		return isAssetLoaded(outSprite.textureAsset);
	}
	return false;
}

bool SpriteAnimationAsset::importFromAsepriteSpriteSheetJsonFile(SpriteAnimationAsset& outSprite,
                                                                 const char* const filename,
                                                                 AssetLibrary& assetLib) {
	if (SpriteAnimation::importFromAsepriteSpriteSheetJsonFile(outSprite.spriteAnimation, filename)) {
		outSprite.textureAsset = assetLib.getAsset(AssetType::Texture2D, outSprite.spriteAnimation.texturePath.c_str(), true);
		return isAssetLoaded(outSprite.textureAsset);
	}
	return false;
}

} // namespace sge
