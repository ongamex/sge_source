#include "TraitViewportIcon.h"
#include "sge_engine/Camera.h"
#include "sge_engine/enums2d.h"

namespace sge {
DefineTypeId(TraitViewportIcon, 20'05'22'0001);

ReflBlock() {
	ReflAddType(TraitViewportIcon);
}

//-----------------------------------------------------------------------
// TraitViewportIcon
//-----------------------------------------------------------------------
TraitViewportIcon::TraitViewportIcon()
    : m_assetProperty(AssetType::TextureView)
    , m_pixelSizeUnitsScreenSpace(0.0011f) {
}

mat4f TraitViewportIcon::computeNodeToWorldMtx(const ICamera& camera) const {
	Texture* const texture = getIconTexture();

	if (texture != nullptr) {
		const float sz = texture->getDesc().texture2D.width * m_pixelSizeUnitsScreenSpace;
		const float sy = texture->getDesc().texture2D.height * m_pixelSizeUnitsScreenSpace;

		transf3d objToWorldNoBillboarding = getActor()->getTransform().getSelfNoScaling();
		objToWorldNoBillboarding.p += quat_mul_pos(objToWorldNoBillboarding.r, m_objectSpaceIconOffset * getActor()->getTransform().s);

		const float distToCamWs = distance(camera.getCameraPosition(), objToWorldNoBillboarding.p);

		const mat4f anchorAlignMtx = anchor_getPlaneAlignMatrix(anchor_mid, vec2f(sz, sy));
		const mat4f scalingFogScreenSpaceConstantSize = mat4f::getScaling(distToCamWs);

		const mat4f billboardFacingMtx = billboarding_getOrentationMtx(billboarding_faceCamera, objToWorldNoBillboarding,
		                                                               camera.getCameraPosition(), camera.getView(), false);
		const mat4f objToWorld = billboardFacingMtx * scalingFogScreenSpaceConstantSize * anchorAlignMtx;

		return objToWorld;
	} else {
		return getActor()->getTransformMtx();
	}
}

Texture* TraitViewportIcon::getIconTexture() const {
	const AssetTexture* const assetTextureView = m_assetProperty.getAssetTexture();
	Texture* const texture =
	    (assetTextureView != nullptr && assetTextureView->tex.IsResourceValid()) ? assetTextureView->tex.GetPtr() : nullptr;
	return texture;
}

} // namespace sge
