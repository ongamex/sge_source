#include "Material.h"

#include "sge_core/Geometry.h"

namespace sge {

// clang-format off
DefineTypeId(OMaterial, 20'10'14'0001);
DefineTypeId(MDiffuseMaterial, 20'10'14'0002);
ReflBlock() {
	ReflAddObject(OMaterial);

	ReflAddObject(MDiffuseMaterial).inherits<MDiffuseMaterial, OMaterial>()
		ReflMember(MDiffuseMaterial, diffuseColor).addMemberFlag(MFF_Vec3fAsColor)
		ReflMember(MDiffuseMaterial, textureShift).uiRange(0.f, 0.f, 0.01f)
		ReflMember(MDiffuseMaterial, textureTiling)
		ReflMember(MDiffuseMaterial, textureRotation).addMemberFlag(MFF_FloatAsDegrees)
		ReflMember(MDiffuseMaterial, metalness).uiRange(0.f, 1.f, 0.01f)
		ReflMember(MDiffuseMaterial, roughness).uiRange(0.f, 1.f, 0.01f)
		ReflMember(MDiffuseMaterial, diffuseTexture)
		ReflMember(MDiffuseMaterial, normalTexture)
		ReflMember(MDiffuseMaterial, metalnessTexture)
		ReflMember(MDiffuseMaterial, roughnessTexture)
	;
}
// clang-format on

Material MDiffuseMaterial::getMaterial() {
	Material result;

	result.uvwTransform =
	    mat4f::getTRS(vec3f(textureShift, 0.f), quatf::getAxisAngle(vec3f(0.f, 0.f, 1.f), textureRotation), vec3f(textureTiling, 1.f));

	diffuseTexture.update();
	normalTexture.update();
	metalnessTexture.update();
	roughnessTexture.update();

	if (AssetTexture* assetTex = diffuseTexture.getAssetTexture(); assetTex && assetTex->tex.IsResourceValid()) {
		result.diffuseTexture = assetTex->tex.GetPtr();
	}

	if (AssetTexture* assetTex = normalTexture.getAssetTexture(); assetTex && assetTex->tex.IsResourceValid()) {
		result.texNormalMap = assetTex->tex.GetPtr();
	}

	if (AssetTexture* assetTex = metalnessTexture.getAssetTexture(); assetTex && assetTex->tex.IsResourceValid()) {
		result.texMetalness = assetTex->tex.GetPtr();
	}

	if (AssetTexture* assetTex = roughnessTexture.getAssetTexture(); assetTex && assetTex->tex.IsResourceValid()) {
		result.texRoughness = assetTex->tex.GetPtr();
	}

	result.diffuseColor = vec4f(diffuseColor, 1.f);
	result.metalness = metalness;
	result.roughness = roughness;

	return result;
}

} // namespace sge
