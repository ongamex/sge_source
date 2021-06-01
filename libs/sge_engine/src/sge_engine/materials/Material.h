#pragma once

#include "sge_engine/GameObject.h"
#include "sge_utils/math/vec4.h"

#include "sge_engine/AssetProperty.h"


namespace sge {

struct Material;

struct SGE_ENGINE_API OMaterial : public GameObject {
	void create() override {}

	virtual Material getMaterial() = 0;
};

struct SGE_ENGINE_API MDiffuseMaterial : public OMaterial {
	MDiffuseMaterial()
	    : diffuseTexture(AssetType::Texture2D)
	    , normalTexture(AssetType::Texture2D)
	    , metalnessTexture(AssetType::Texture2D)
	    , roughnessTexture(AssetType::Texture2D) {}

	void create() override {}

	Material getMaterial() override;

	vec2f textureShift = vec2f(0.f);
	vec2f textureTiling = vec2f(1.f);
	float textureRotation = 0.f;

	float metalness = 0.11f;
	float roughness = 0.30f;
	vec3f diffuseColor = vec3f(1.f);
	AssetProperty diffuseTexture;
	AssetProperty normalTexture;
	AssetProperty metalnessTexture;
	AssetProperty roughnessTexture;
};

} // namespace sge
