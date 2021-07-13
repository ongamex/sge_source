#pragma once

#include "sge_engine/GameDrawer/IRenderItem.h"

namespace sge {

struct TraitModel;
struct EvaluatedModel;

struct TraitSpriteRenderItem : public IRenderItem {
	Actor* actor = nullptr;
	Texture* spriteTexture = nullptr;
	mat4f obj2world = mat4f::getIdentity();
	mat4f uvwTransform = mat4f::getIdentity();
	vec4f colorTint = vec4f(1.f);
	bool forceNoLighting = false;
	bool forceNoCulling = false;
};

} // namespace sge
