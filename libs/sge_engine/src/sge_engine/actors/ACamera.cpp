#include "ACamera.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/typelibHelper.h"

namespace sge {

// clang-format off
DefineTypeId(PerspectiveCameraSettings, 20'03'01'0018);
DefineTypeId(CameraTraitCamera, 20'03'01'0019);
DefineTypeId(ACamera, 20'03'01'0020);

ReflBlock()
{
	ReflAddType(PerspectiveCameraSettings)
	ReflMember(PerspectiveCameraSettings, orthoGraphicMode)
	ReflMember(PerspectiveCameraSettings, nearPlane)
	ReflMember(PerspectiveCameraSettings, farPlane)
	ReflMember(PerspectiveCameraSettings, fov)
		.addMemberFlag(MFF_FloatAsDegrees)
	ReflMember(PerspectiveCameraSettings, orthographicWidth)
	ReflMember(PerspectiveCameraSettings, orthographicHeight)
	ReflMember(PerspectiveCameraSettings, orthographicMantainRatio)
;
	
	ReflAddType(CameraTraitCamera)
		ReflMember(CameraTraitCamera, m_cameraSettings)
	;

	ReflAddActor(ACamera)
		ReflMember(ACamera, m_traitCamera)
	;
}
// clang-format on

//---------------------------------------------------------------
//
//---------------------------------------------------------------
void CameraTraitCamera::update(const GameUpdateSets& UNUSED(updateSets)) {
	GameWorld* const world = getWorld();

	const CameraProjectionSettings& projSets = world->userProjectionSettings;
	m_proj = m_cameraSettings.calcMatrix(projSets.aspectRatio);
	m_view = getActor()->getTransform().toMatrix().inverse();
	// Make the camera look along +X
	m_view = mat4f::getRotationY(sgeHalfPi) * m_view;
	m_projView = m_proj * m_view;
	m_cachedFrustumWS = Frustum::extractClippingPlanes(m_projView, kIsTexcoordStyleD3D);
}

//---------------------------------------------------------------
//
//---------------------------------------------------------------
ACamera::ACamera() {
	registerTrait(m_traitViewportIcon);
	m_traitViewportIcon.setTexture("assets/editor/textures/icons/obj/ACamera.png", true);
}

void ACamera::create() {
	registerTrait(m_traitCamera);
}

AABox3f ACamera::getBBoxOS() const {
	return AABox3f::getFromHalfDiagonal(vec3f(1.f, 1.f, 1.f));
}


void ACamera::update(const GameUpdateSets& updateSets) {
	m_traitCamera.update(updateSets);
}

} // namespace sge
