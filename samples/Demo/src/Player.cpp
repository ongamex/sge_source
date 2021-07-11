#include "sge_core/ICore.h"
#include "sge_engine/Actor.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/traits/TraitModel.h"
#include "sge_engine/traits/TraitRigidBody.h"
#include "sge_engine/typelibHelper.h"
#include "sge_utils/math/EulerAngles.h"

namespace sge {

struct Player : public Actor {
	TraitRigidBody ttRigidbody;
	TraitModel ttModel;

	ObjectId cameraObject;
	vec3f cameraOffset = vec3f(0.f, 2.f, 3.f);

	float yPositionRestart = -50.f;
	vec3f respawnPosition = vec3f(0.f);

	float wobbleAmplitude = 1.f;
	float wobbleForce = 0.f;

	float jumpButtonHeldTime = 0.f;
	static inline const float kMaxJumpHeldTime = 1.f;

	virtual AABox3f getBBoxOS() const { return AABox3f(); }

	void create() {
		registerTrait(ttRigidbody);
		registerTrait(ttModel);

		ttModel.setModel("assets/models/catBall.mdl", true);
		ttRigidbody.getRigidBody()->create(this, CollsionShapeDesc::createSphere(1.f), 1.f, false);
		ttRigidbody.getRigidBody()->setRollingFriction(0.025f);
		ttRigidbody.getRigidBody()->setSpinningFriction(0.025f);
		ttRigidbody.getRigidBody()->setFriction(0.8f);
		ttRigidbody.getRigidBody()->setBounciness(0.9f);

		ttRigidbody.getRigidBody()->setDamping(0.05f, 0.005f);
	}

	void onPlayStateChanged(bool const isStartingToPlay) {
		Actor::onPlayStateChanged(isStartingToPlay);
		if (isStartingToPlay) {
			respawnPosition = this->getPosition();
		}
	}

	void update(const GameUpdateSets& u) {
		if (u.isGamePaused()) {
			return;
		}

		// Lock the cursor to the center of the screen and hide it
		// as we want to control the camera with the mouse.
		getEngineGlobal()->setNeedForRelativeCursorThisFrame();

		if (this->getPosition().y < yPositionRestart) {
			this->setPosition(respawnPosition);
			wobbleAmplitude = 1.f;
			wobbleForce = 0.f;
			jumpButtonHeldTime = 0.f;
		}

		Actor* const cameraActor = getWorld()->getActorById(cameraObject);

		vec3f wsForward = vec3f(0.f, 0.f, -1.f);
		vec3f wsRight = vec3f(1.f, 0.f, 0.f);

		if (cameraActor) {
			float motionCamera = u.is.isCursorRelative() ? -u.is.GetCursorMotion().x * 0.25f : 0.f;
			if (auto gamepad = u.is.getHookedGemepad(0); gamepad && motionCamera == 0) {
				motionCamera = -gamepad->axisR.x * 3.f;
			}

			quatf rotationQuat = quatf::getAxisAngle(vec3f::axis_y(), motionCamera * deg2rad(60.f) * u.dt);

			cameraOffset = quat_mul_pos(rotationQuat, cameraOffset);

			transf3d cameraTransform = cameraActor->getTransform();
			cameraTransform.p = getTransform().p + cameraOffset;
			// cameraTransform.r = quatf::getAxisAngle(vec3f::axis_x(), -deg2rad(30.f)) * quatf::getAxisAngle(vec3f::axis_y(),
			// deg2rad(90.f));
			cameraTransform.r = rotationQuat * cameraTransform.r;
			cameraActor->setTransform(cameraTransform);

			wsForward = cameraActor->getTransformMtx().c0.xyz().x0z().normalized0();
			wsRight = cameraActor->getTransformMtx().c2.xyz().x0z().normalized0();
		}

		wobbleAmplitude -= ttRigidbody.getRigidBody()->getLinearVel().x0z().length() * u.dt * 0.2f;
		wobbleAmplitude -= ttRigidbody.getRigidBody()->getLinearVel().y * u.dt * 0.30f;
		wobbleForce += (0.85f - wobbleAmplitude) * 1.75f;
		wobbleForce -= wobbleForce * u.dt * 2.f;
		wobbleAmplitude += wobbleForce * u.dt;

		// Fake touch screen input.
		vec2f inputDir = u.is.GetArrowKeysDir(true, true, 0);
		if (u.is.IsKeyDown(Key_MouseLeft)) {
			inputDir.x = 2.f * u.is.getCursorPosUV().x - 1.f;
			inputDir.y = -(2.f * u.is.getCursorPosUV().y - 1.f);
		}

		const vec3f inputDirWs = wsRight * inputDir.x + inputDir.y * wsForward;
		const vec3f inputDirWsRight = vec3f(-inputDirWs.z, 0.f, inputDirWs.x);

		if (u.is.IsKeyDown(Key_Space) || u.is.getXInputDevice(0).isBtnDown(GamepadState::btn_a)) {
			wobbleForce -= 16.f * u.dt;
			jumpButtonHeldTime += u.dt;
			jumpButtonHeldTime = clamp(kMaxJumpHeldTime, 0.f, kMaxJumpHeldTime);
		}

		vec3f additionalForce = vec3f(0.f);
		float dr = inputDirWsRight.dot(ttRigidbody.getRigidBody()->getLinearVel().x0z());
		float df = inputDirWs.dot(ttRigidbody.getRigidBody()->getLinearVel().x0z());
		additionalForce = -dr * inputDirWsRight * 1.5f;
		if (df < 0.f) {
			additionalForce += -df * inputDirWs * 3.f;
		}

		if (u.is.IsKeyReleased(Key_Space) || u.is.getXInputDevice(0).isBtnReleased(GamepadState::btn_a)) {
			ttRigidbody.getRigidBody()->applyLinearVelocity(vec3f(0.f, 15.f * jumpButtonHeldTime / kMaxJumpHeldTime + 1.f, 0.f));
			jumpButtonHeldTime = 0.f;
		}

		ttRigidbody.getRigidBody()->applyForce((inputDirWs * 10.f + additionalForce), vec3f(0.f, 0.65f, 0.f));

		float squash = wobbleAmplitude;

		ttModel.m_models[0].m_additionalTransform = mat4f::getRotationQuat(getTransform().r.inverse()) *
		                                            mat4f::getTranslation(0.f, -0.5f, 0.f) * mat4f::getSquashyScalingY(squash) *
		                                            mat4f::getTranslation(0.f, 0.5f, 0.f) * mat4f::getRotationQuat(getTransform().r);
	}
};

DefineTypeId(Player, 30'02'22'0001);
ReflBlock() {
	ReflAddActor(Player) ReflMember(Player, cameraObject) ReflMember(Player, cameraOffset);
}

} // namespace sge
