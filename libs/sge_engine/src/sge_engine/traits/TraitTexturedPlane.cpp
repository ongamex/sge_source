#include "TraitTexturedPlane.h"

namespace sge {

//-------------------------------------------------------------------
// TraitTexturedPlane
//-------------------------------------------------------------------

// clang-format off
DefineTypeId(Anchor, 20'03'01'0003);
DefineTypeId(TraitTexturedPlane, 20'03'01'0021);

ReflBlock() {
	ReflAddType(TraitTexturedPlane)
		ReflMember(TraitTexturedPlane, m_anchor)
		ReflMember(TraitTexturedPlane, m_localXOffset)
		ReflMember(TraitTexturedPlane, m_pixelSizeInWorld)
		ReflMember(TraitTexturedPlane, m_billboarding)
		ReflMember(TraitTexturedPlane, m_assetProperty)
	;
}
// clang-format on

mat4f TraitTexturedPlane::getAnchorAlignMtxOS() const {
	const AssetTexture* const assetTextureView = getAssetProperty().getAssetTexture();
	if (assetTextureView && assetTextureView->tex.IsResourceValid()) {
		Texture* const texture = assetTextureView->tex.GetPtr();
		if (texture) {
			const float sz = texture->getDesc().texture2D.width * m_pixelSizeInWorld;
			const float sy = texture->getDesc().texture2D.height * m_pixelSizeInWorld;

			const mat4f anchorAlineMtx = anchor_getPlaneAlignMatrix(m_anchor, vec2f(sz, sy));
			return anchorAlineMtx;
		}
	}

	return mat4f::getIdentity();
}

// This function is designed to transform a rectangle with coordinates:
// min (0, 0, 0)
// max (0, 1, 1)

AABox3f TraitTexturedPlane::getBBoxOS() const {
	const AssetTexture* const assetTextureView = getAssetProperty().getAssetTexture();
	if (assetTextureView && assetTextureView->tex.IsResourceValid()) {
		AABox3f bbox;

		const float width = (float)assetTextureView->tex.GetPtr()->getDesc().texture2D.width;
		const float height = (float)assetTextureView->tex.GetPtr()->getDesc().texture2D.height;

		const vec3f halfDiagonal(1e-3f, height * m_pixelSizeInWorld * 0.5f, width * m_pixelSizeInWorld * 0.5f);

		bbox.setEmpty();
		bbox.expand(vec3f(0.f));
		bbox.expand(-halfDiagonal);
		bbox.expand(halfDiagonal);

		if (m_anchor == anchor_bottomLeft) {
			// Nothing in that case.
		} else if (m_anchor == anchor_bottomMid) {
			bbox.move(-halfDiagonal.zOnly());
		} else if (m_anchor == anchor_mid) {
			bbox.move(-halfDiagonal);
		} else {
			sgeAssert(false);
		}

		return bbox;
	}

	return AABox3f(); // Return an empty box.
}

} // namespace sge
