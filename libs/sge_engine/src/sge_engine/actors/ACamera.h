#pragma once

#include "sge_engine/traits/TraitCamera.h"
#include "sge_engine/traits/TraitViewportIcon.h"
#include "sge_renderer/renderer/renderer.h"

namespace sge {

//---------------------------------------------------------------
//
//---------------------------------------------------------------
struct PerspectiveCameraSettings {
	bool orthoGraphicMode = false;
	float nearPlane = 1.f;
	float farPlane = 10000.f;
	float fov = deg2rad(60.f);
	float orthographicWidth = 1.f;
	float orthographicHeight = 1.f;
	bool orthographicMantainRatio = true;

	mat4f calcMatrix(float const aspect) const {
		if (orthoGraphicMode) {
			float w = orthographicWidth * 0.5f;
			float h = orthographicHeight * 0.5f;

			if (orthographicMantainRatio) {
				w = h * aspect;
			}

			return mat4f::getOrthoRH(-w, w, h, -h, nearPlane, farPlane, kIsTexcoordStyleD3D);
		}

		return mat4f::getPerspectiveFovRH(fov, aspect, nearPlane, farPlane, kIsTexcoordStyleD3D);
	}
};

struct SGE_ENGINE_API CameraTraitCamera : public TraitCamera, public ICamera {
	SGE_TraitDecl_Final(CameraTraitCamera);

	void update(const GameUpdateSets& updateSets);

	// From TraitCamera
	ICamera* getCamera() override final { return this; }
	const ICamera* getCamera() const override final { return this; }

	// From ICamera
	vec3f getCameraPosition() const final { return getActor()->getTransform().p; }
	virtual vec3f getCameraLookDir() const final { return -getView().getRow(2).xyz(); }
	mat4f getView() const final { return m_view; }
	mat4f getProj() const final { return m_proj; }
	mat4f getProjView() const final { return m_projView; }
	const Frustum* getFrustumWS() const final { return &m_cachedFrustumWS; }

  public:
	PerspectiveCameraSettings m_cameraSettings;

	mat4f m_view = mat4f::getIdentity();
	mat4f m_proj = mat4f::getIdentity();
	mat4f m_projView = mat4f::getIdentity();
	Frustum m_cachedFrustumWS;
};

//---------------------------------------------------------------
//
//---------------------------------------------------------------
struct ACamera : public Actor {
	ACamera();

	void create() final;
	AABox3f getBBoxOS() const final;
	void update(const GameUpdateSets& updateSets) final;


  public:
	CameraTraitCamera m_traitCamera;
	TraitViewportIcon m_traitViewportIcon;
};

} // namespace sge
