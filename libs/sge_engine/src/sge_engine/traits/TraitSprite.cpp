#include "TraitSprite.h"
#include "IconsForkAwesome/IconsForkAwesome.h"
#include "sge_engine/GameDrawer/RenderItems/TraitSpriteRrenderItem.h"
#include "sge_core/SGEImGui.h"
#include "sge_engine/Camera.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/windows/PropertyEditorWindow.h"

namespace sge {
// clang-format off
DefineTypeId(TraitSprite, 20'07'11'0001);
DefineTypeId(TraitSprite::ImageSettings, 21'04'04'0001);

ReflBlock() {
	ReflAddType(TraitSprite::ImageSettings)
		ReflMember(TraitSprite::ImageSettings, m_anchor)
		ReflMember(TraitSprite::ImageSettings, colorTint).addMemberFlag(MFF_Vec4fAsColor)
		ReflMember(TraitSprite::ImageSettings, defaultFacingAxisZ)
		ReflMember(TraitSprite::ImageSettings, m_localXOffset).uiRange(-FLT_MAX, FLT_MAX, 0.01f)
		ReflMember(TraitSprite::ImageSettings, m_pixelsPerUnit).uiRange(0.00001f, 100000.f, 0.1f)
		ReflMember(TraitSprite::ImageSettings, m_billboarding)
		ReflMember(TraitSprite::ImageSettings, forceNoLighting)
		ReflMember(TraitSprite::ImageSettings, forceNoCulling)
		ReflMember(TraitSprite::ImageSettings, flipHorizontally)
		ReflMember(TraitSprite::ImageSettings, spriteFrameTime).uiRange(0.f, 100000.f, 0.01f)
	;

	ReflAddType(TraitSprite)
		ReflMember(TraitSprite, m_assetProperty)
		ReflMember(TraitSprite, imageSettings)
	;
}
// clang-format on

AABox3f TraitSprite::getBBoxOS() const {
	const SpriteAnimationAsset* const assetSprite = getAssetProperty().getAssetSprite();
	const AssetTexture* const assetTexture = getAssetProperty().getAssetTexture();

	if ((assetSprite || assetTexture) && isAssetLoaded(getAssetProperty().getAsset())) {
		return imageSettings.computeBBoxOS(*getAssetProperty().getAsset().get(), m_additionalTransform);
	}

	// Not implemented or no asset is loaded.
	return AABox3f();
}

mat4f TraitSprite::ImageSettings::computeObjectToWorldTransform(const Asset& asset,
                                                                const ICamera* const drawCamera,
                                                                const transf3d& nodeToWorldTransform,
                                                                const mat4f& additionaTransform) const {
	vec2f imageSize = vec2f(0.f);
	bool hasValidImageToRender = false;
	if (isAssetLoaded(asset, AssetType::Sprite)) {
		const SpriteAnimationAsset* sprite = asset.asSprite();
		const SpriteAnimation::Frame* frame = sprite->spriteAnimation.getFrameForTime(spriteFrameTime);
		imageSize = vec2f(float(frame->wh.x), float(frame->wh.y));
		hasValidImageToRender = true;
	} else if (isAssetLoaded(asset, AssetType::Texture2D)) {
		const AssetTexture* assetTex = asset.asTextureView();
		if (assetTex && assetTex->tex.IsResourceValid()) {
			imageSize = vec2f(float(assetTex->tex->getDesc().texture2D.width), float(assetTex->tex->getDesc().texture2D.height));
			hasValidImageToRender = true;
		}
	}

	if (hasValidImageToRender) {
		const mat4f localOffsetmtx = mat4f::getTranslation(m_localXOffset, 0.f, 0.f);
		const mat4f anchorAlignMtx = getAnchorAlignMtxOS(imageSize.x, imageSize.y);

		mat4f objToWorld;
		if (drawCamera != nullptr) {
			const mat4f billboardFacingMtx = billboarding_getOrentationMtx(
			    m_billboarding, nodeToWorldTransform, drawCamera->getCameraPosition(), drawCamera->getView(), defaultFacingAxisZ);
			objToWorld = billboardFacingMtx * anchorAlignMtx * localOffsetmtx * additionaTransform;
		} else {
			objToWorld = anchorAlignMtx * localOffsetmtx * additionaTransform;

			// Hack:
			if (m_billboarding == billboarding_none && defaultFacingAxisZ) {
				const mat4f billboardFacingMtx = mat4f::getRotationY(deg2rad(-90.f));
				objToWorld = billboardFacingMtx * objToWorld;
			}
		}

		if (flipHorizontally) {
			objToWorld = objToWorld * mat4f::getTranslation(0.f, 0.f, 1.f) * mat4f::getScaling(1.f, 1.f, -1.f);
		}

		return objToWorld;
	}

	return mat4f::getIdentity();
}

AABox3f TraitSprite::ImageSettings::computeBBoxOS(const Asset& asset, const mat4f& additionaTransform) const {
	// What is happening here is the following:
	// When there is no billboarding applied the bounding box computation is pretty strait forward -
	// Just transform the verticies of of quad by object-to-node space transfrom and we are done.
	// However when there is billboarding the plane is allowed to rotate (based on the type of the bilboarding).
	// it might be sweeping around an axis (Up only billboarding) or facing the camera (meaning int can rotate freely around its center
	// point).
	// In order to compute the bounding box for those cases we take the un-billboarded transformation, obtain its size around that
	// center point and then use that vector to extend the bounding box in each cardinal direction that the quad could rotate. If we do
	// not apply the @additionaTransform the point (0,0,0) in quad space is going to lay on the axis that the quad rotates about. So to
	// compute the bounding box we take the furthest point of the quard form (0,0,0) and extend the initial (without billboarding
	// bounding box based on it. After that we can safely apply @additionaTransform to move the bounding box in node space safely
	// together with the plane. Note: it seems that this function is too complicated for what it does. The bounding box doesn't really
	// need to be the smallest fit, just good enough so if it is a problem a simpler function might be written.
	const transf3d fakeNodeToWorld;
	const mat4f noAdditionalTransform = mat4f::getIdentity();
	const mat4f planeTransfromObjectToNode = computeObjectToWorldTransform(asset, nullptr, fakeNodeToWorld, noAdditionalTransform);

	AABox3f bboxOs;

	vec3f p0 = mat_mul_pos(planeTransfromObjectToNode, vec3f(0.f));
	vec3f p1 = mat_mul_pos(planeTransfromObjectToNode, vec3f(0.f, 1.f, 1.f));

	bboxOs.expand(p0);
	bboxOs.expand(p1);

	switch (m_billboarding) {
		case billboarding_none: {
			return bboxOs;
		} break;

		case billboarding_yOnly: {
			vec3f pExtreme = p0.getAbs().pickMax(p1.getAbs());
			bboxOs.expand(vec3f(0.f, 0.f, pExtreme.z));
			bboxOs.expand(vec3f(0.f, 0.f, -pExtreme.z));
			bboxOs.expand(vec3f(pExtreme.z, 0.f, 0.f));
			bboxOs.expand(vec3f(-pExtreme.z, 0.f, 0.f));
		} break;
		case billboarding_faceCamera: {
			vec3f pExtreme = p0.getAbs().pickMax(p1.getAbs());
			float d = pExtreme.length();
			bboxOs.expand(vec3f(-d));
			bboxOs.expand(vec3f(d));
		} break;
		default:
			sgeAssertFalse("Unimplemented billboarding");
	}

	if (additionaTransform != mat4f::getIdentity()) {
		bboxOs.getTransformed(additionaTransform);
	}

	return bboxOs;
}

void TraitSprite::getRenderItems(const GameDrawSets& drawSets, std::vector<TraitSpriteRenderItem>& renderItems) {
	TraitSpriteRenderItem renderItem;

	renderItem.actor = getActor();
	renderItem.forceNoCulling = imageSettings.forceNoCulling;
	renderItem.forceNoLighting = imageSettings.forceNoLighting;
	renderItem.colorTint = imageSettings.colorTint;

	const vec3f camPos = drawSets.drawCamera->getCameraPosition();
	const vec3f camLookDir = drawSets.drawCamera->getCameraLookDir();
	Actor* const actor = getActor();

	PAsset const asset = getAssetProperty().getAsset();

	if (isAssetLoaded(asset) && asset->getType() == AssetType::Texture2D) {
		AssetTexture* ppTexture = asset->asTextureView();
		if (ppTexture && ppTexture->tex.IsResourceValid()) {
			Texture* const texture = ppTexture->tex.GetPtr();
			if (texture) {
				renderItem.spriteTexture = texture;

				mat4f obj2world = imageSettings.computeObjectToWorldTransform(*asset.get(), drawSets.drawCamera, actor->getTransform(),
				                                                              m_additionalTransform);

				renderItem.obj2world = obj2world;

				renderItem.zSortingPositionWs = obj2world.c3.xyz();
				renderItem.needsAlphaSorting = ppTexture->meta.isSemiTransparent || renderItem.colorTint.w < 0.999f;
			}
		}
	} else if (isAssetLoaded(asset) && asset->getType() == AssetType::Sprite) {
		SpriteAnimationAsset* const pSprite = asset->asSprite();
		if (pSprite && isAssetLoaded(pSprite->textureAsset, AssetType::Texture2D)) {
			// Get the frame of the sprite to be rendered.
			const SpriteAnimation::Frame* const frame = pSprite->spriteAnimation.getFrameForTime(imageSettings.spriteFrameTime);
			if (frame) {
				renderItem.spriteTexture = pSprite->textureAsset->asTextureView()->tex.GetPtr();

				mat4f obj2world = imageSettings.computeObjectToWorldTransform(*asset.get(), drawSets.drawCamera, actor->getTransform(),
				                                                              m_additionalTransform);
				renderItem.obj2world = obj2world;

				// Compute the UVW transform so we get only this frame portion of the texture to be displayed.
				renderItem.uvwTransform =
				    mat4f::getTranslation(frame->uvRegion.x, frame->uvRegion.y, 0.f) *
				    mat4f::getScaling(frame->uvRegion.z - frame->uvRegion.x, frame->uvRegion.w - frame->uvRegion.y, 0.f);

				renderItem.zSortingPositionWs = obj2world.c3.xyz();
				renderItem.needsAlphaSorting =
				    pSprite->textureAsset->asTextureView()->meta.isSemiTransparent || renderItem.colorTint.w < 0.999f;
			}
		}
	} else {
		return;
	}

	if (renderItem.spriteTexture != nullptr) {
		renderItems.emplace_back(renderItem);
	}
}

/// TraitSprite Attribute Editor.
void editTraitSprite(GameInspector& inspector, GameObject* actor, MemberChain chain) {
	TraitSprite& traitSprite = *(TraitSprite*)chain.follow(actor);

	if (ImGui::CollapsingHeader(ICON_FK_PICTURE_O " Sprte Trait")) {
		chain.add(sgeFindMember(TraitSprite, m_assetProperty));
		ProperyEditorUIGen::doMemberUI(inspector, actor, chain);
		chain.pop();

		if (traitSprite.m_assetProperty.getAssetSprite()) {
			chain.add(sgeFindMember(TraitSprite, imageSettings));
			ProperyEditorUIGen::doMemberUI(inspector, actor, chain);
			chain.pop();
		}

		if (traitSprite.m_assetProperty.getAssetTexture()) {
			chain.add(sgeFindMember(TraitSprite, imageSettings));
			ProperyEditorUIGen::doMemberUI(inspector, actor, chain);
			chain.pop();
		}
	}
}

SgePluginOnLoad() {
	getEngineGlobal()->addPropertyEditorIUGeneratorForType(sgeTypeId(TraitSprite), editTraitSprite);
}


} // namespace sge
