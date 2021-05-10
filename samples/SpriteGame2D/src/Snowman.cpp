#include "sge_engine/Actor.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/traits/TraitModel.h"
#include "sge_engine/traits/TraitRigidBody.h"
#include "sge_engine/typelibHelper.h"

namespace sge {

vec2f getIntersectionDepth(const AABox2f& rectA, const AABox2f& rectB) {
	vec2f centerA = rectA.center();
	vec2f centerB = rectB.center();

	vec2f distance = centerA - centerB;
	vec2f minDistance = rectA.halfDiagonal() + rectB.halfDiagonal();

	if (abs(distance.x) >= minDistance.x || abs(distance.y) >= minDistance.y) {
		return vec2f(0.f);
	}

	float depthX = distance.x > 0 ? minDistance.x - distance.x : -minDistance.x - distance.x;
	float depthY = distance.y > 0 ? minDistance.y - distance.y : -minDistance.y - distance.y;

	return vec2f(depthX, depthY);
}

struct TraitCollisionRect2d;
DefineTypeIdExists(TraitCollisionRect2d);
struct TraitCollisionRect2d : public Trait {
	SGE_TraitDecl_Full(TraitCollisionRect2d);

	vec2f rectSize;

	AABox2f getRect() const {
		vec2f pos = getActor()->getPosition().xy();
		AABox2f rect = AABox2f(pos, pos + rectSize);
		return rect;
	}
};

DefineTypeId(TraitCollisionRect2d, 21'04'17'0003);

struct Snowman : public Actor {
	TraitModel ttModel;
	TraitCollisionRect2d collider;
	vec2f vel;
	float timeSpentInAir = 0.f;
	int jumpCounter = 0;

	virtual AABox3f getBBoxOS() const { return ttModel.getBBoxOS(); }

	void create() {
		registerTrait(ttModel);
		registerTrait(collider);

		collider.rectSize = vec2f(24.f / 64.f, 32.f / 64.f);

		ttModel.setModel("assets/sprites/snowman.png", true);
		ttModel.imageSettings.m_anchor = anchor_topLeft;
	}

	void update(const GameUpdateSets& u) {
		if (u.isGamePaused()) {
			return;
		}

		const float jumpHeight = 36.f;
		const float jumpTimeApex = 0.3f;
		const float minJumpHeight = 8.f;
		const float fallingGravityMultiplier = 1.f;
		const float gravity = 2.f * jumpHeight / (jumpTimeApex * jumpTimeApex);
		const float fallingGravity = gravity * fallingGravityMultiplier;
		const float maxJumpVelocity = gravity * jumpTimeApex;
		const float minJumpVelocity = sqrt(2.f * gravity * minJumpHeight);

		// Apply the gravity.
		if (vel.y < 0) {
			vel.y -= gravity * u.dt;
		} else {
			vel.y -= fallingGravity * u.dt;
		}

		vel.y = maxOf(vel.y, -400.f);

		// Apply horizontal movement drag.
		vel.x -= vel.x * 3.f * u.dt;

		// Move the player and then perform the collision detection, determine if the player is grounded.
		setPosition(getPosition() + vec3f(vel / 64.f, 0.f) * u.dt);

		bool isGrounded = false;
		getWorld()->iterateOverPlayingObjects(
		    [&](GameObject* go) -> bool {
			    if (Actor* otherActor = go->getActor(); otherActor != this) {
				    if (TraitCollisionRect2d* otherRect = getTrait<TraitCollisionRect2d>(otherActor)) {
					    vec2f intersection = getIntersectionDepth(collider.getRect(), otherRect->getRect());
					    if (intersection != vec2f(0.f)) {
						    if (fabsf(intersection.x) < fabsf(intersection.y)) {
							    setPosition(getPosition() + vec3f(intersection.x, 0.f, 0.f));
						    } else {
							    setPosition(getPosition() + vec3f(0.f, intersection.y, 0.f));

							    isGrounded |= intersection.y > 0.f;

							    if (vel.y < 0.f && intersection.y > 0.f)
								    vel.y = 0.f;

							    if (vel.y > 0.f && intersection.y < 0.f && intersection.x == 0.f)
								    vel.y = 0.f;
						    }
					    }
				    }
			    }

			    return true;
		    },
		    false);

		// Update the state of the actor collsion, if it is airbourne, can it jump and so on.
		if (isGrounded) {
			timeSpentInAir = 0.f;
			jumpCounter = 0;
			if (vel.y < 0) {
				vel.y = 0.f;
			}
		} else {
			timeSpentInAir += u.dt;
		}

		// Handle the input of hte player.
		if (u.is.wasActiveWhilePolling()) {
			vec2f input = u.is.GetArrowKeysDir(false, false, 0);

			if (input.x == 0 && u.is.IsKeyDown(Key_MouseLeft)) {
				if (u.is.getCursorPosUV().x < 0.25f) {
					input.x = -1.f;
				} else if (u.is.getCursorPosUV().x < 0.5f) {
					input.x = 1.f;
				}
			}

			// Horizontal movement.
			if (input.x != 0.f) {
				const float kMaxSpeed = 140.f;            // units/second.
				const float kVelocityChangeSpeed = 350.f; // units/second^2.
				vel.x = speedLerp(vel.x, kMaxSpeed * input.x, kVelocityChangeSpeed * u.dt);

				// Change the sprite orientation so it faces the walking direction.
				ttModel.imageSettings.flipHorizontally = input.x < 0.f;

				// Update the sprite animation.
				ttModel.setModel("assets/sprites/snowman_walk.sprite", true);
				ttModel.imageSettings.spriteFrameTime += u.dt;

				if (ttModel.imageSettings.spriteFrameTime >
				    ttModel.getAssetProperty().getAssetSprite()->spriteAnimation.animationDuration) {
					ttModel.imageSettings.spriteFrameTime -= ttModel.imageSettings.spriteFrameTime;
				}
			}

			// Handle jumping.
			const bool jumpBtnPressed = u.is.IsKeyPressed(Key_Space) || u.is.getXInputDevice(0).isBtnPressed(GamepadState::btn_a);
			const bool jumpBtnReleased = u.is.IsKeyReleased(Key_Space) || u.is.getXInputDevice(0).isBtnReleased(GamepadState::btn_a);

			// If the jump button is pressed apply the high jump velocity.
			// Later if the player released the button, and the player is still going up
			// apply the minJumpVelocity. As a result the if the player hold the jump button
			// longer the jump will be higher.
			if ((timeSpentInAir < 0.15f || jumpCounter == 1) && jumpBtnPressed) {
				vel.y = maxJumpVelocity;
				jumpCounter++;
			}

			if (jumpBtnReleased && jumpCounter <= 1) {
				if (vel.y > minJumpVelocity) {
					vel.y = minJumpVelocity;
				}
			}
		}
	}
};

DefineTypeId(Snowman, 21'04'17'0001);

ReflBlock() {
	ReflAddActor(Snowman);
}

struct IceBlock : public Actor {
	TraitModel ttModel;
	TraitCollisionRect2d collider;

	virtual AABox3f getBBoxOS() const { return ttModel.getBBoxOS(); }

	void create() {
		registerTrait(ttModel);
		registerTrait(collider);

		collider.rectSize = vec2f(32.f / 64.f, 32.f / 64.f);
		ttModel.setModel("assets/sprites/tile.png", true);
		ttModel.imageSettings.m_anchor = anchor_topLeft;
	}
};

DefineTypeId(IceBlock, 21'04'17'0002);

ReflBlock() {
	ReflAddActor(IceBlock);
}

} // namespace sge
