#include "ASky.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/windows/PropertyEditorWindow.h"

namespace sge {

// clang-format off
DefineTypeId(SkyShaderSettings::Mode, 21'06'23'0001);
DefineTypeId(ASky, 21'06'23'0002);

ReflBlock() {
	ReflAddType(SkyShaderSettings::Mode)
		ReflEnumVal(SkyShaderSettings::mode_colorGradinet, "Top-Down Gradient")
		ReflEnumVal(SkyShaderSettings::mode_textureSphericalMapped, "Sphere Mapped")
		ReflEnumVal(SkyShaderSettings::mode_textureCubeMapped, "Cube Mapped 2D")
	;

	ReflAddActor(ASky)
		ReflMember(ASky, m_mode)
		ReflMember(ASky, m_topColor).addMemberFlag(MFF_Vec3fAsColor)
		ReflMember(ASky, m_bottomColor).addMemberFlag(MFF_Vec3fAsColor)
		ReflMember(ASky, m_textureAssetProp)

	;

}
// clang-format on

void ASky::create() {
	registerTrait(ttViewportIcon);
	registerTrait(static_cast<IActorCustomAttributeEditorTrait&>(*this));

	ttViewportIcon.setTexture(getEngineGlobal()->getEngineAssets().getIconForObjectType(sgeTypeId(ASky)), true);
}

void ASky::postUpdate(const GameUpdateSets& UNUSED(updateSets)) {
	m_textureAssetProp.update();
}

void ASky::doAttributeEditor(GameInspector* inspector) {
	MemberChain chain;

	chain.add(typeLib().findMember(&ASky::m_mode));
	ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
	chain.pop();

	if (m_mode == SkyShaderSettings::mode_colorGradinet) {
		chain.add(typeLib().findMember(&ASky::m_topColor));
		ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
		chain.pop();

		chain.add(typeLib().findMember(&ASky::m_bottomColor));
		ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
		chain.pop();
	} else {
		chain.add(typeLib().findMember(&ASky::m_textureAssetProp));
		ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
		chain.pop();
	}
}

SkyShaderSettings ASky::getSkyShaderSetting() const {
	SkyShaderSettings result;

	result.mode = m_mode;
	result.topColor = m_topColor;
	result.bottomColor = m_bottomColor;
	result.texture = m_textureAssetProp.getAssetTexture() ? m_textureAssetProp.getAssetTexture()->tex.GetPtr() : nullptr;

	return result;
}

} // namespace sge
