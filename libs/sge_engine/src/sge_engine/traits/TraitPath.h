#pragma once

#include "sge_engine/Actor.h"
#include "sge_utils/utils/optional.h"

namespace sge {

struct TraitPath3D;

enum BounceType {
	bounceType_bounce,
	bouceType_reset,
	bounceType_stop,
	bounceType_onForwardOffBackwards,
	bounceType_idle,
};

float computePathLength(const std::vector<vec3f>& path);
vec3f samplePathLazy(const std::vector<vec3f>& path, float distance);

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
namespace PathLengthFollow {
	struct SGE_ENGINE_API Settings {
		bool isReversed =
		    false; // true if the movement starts form the end of the path. (the includes the distance progress along the path).
		float speed = 1.f;
		float restingTime = 0.f;
		BounceType bounceType = bounceType_bounce; // what should happen when the end of the path is reached.
	};

	struct SGE_ENGINE_API State {
		float timeLeftToRest = 0.f; // The time that the follower need to spend resting.
		float distanceAlongPath =
		    0.f; // The distance travelled in the paths object space. the value is maintained the same if reversed is specified to the
		         // settings, the only thing that is changed is that the starting point is now the end point.
		float evalDistanceAlongPath = 0.f; // The actual distance used for evaluation along the path.
		bool speedIsPositive = true;

		bool isResting() const { return timeLeftToRest > 0.f; }
	};

	/// @param recursionDepth Also leave default value. Used to prevent endless recursion if there is a mistake inside the function.
	SGE_ENGINE_API State compute(const float pathLength,
	                             const float dt,
	                             GameWorld* const world,
	                             const Settings& settings,
	                             const State& prevState,
	                             const int recursionDepth = 0);
} // namespace PathLengthFollow

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
namespace PathFollow {
	struct SGE_ENGINE_API Settings {
		ObjectId pathId;
		bool isReversed =
		    false; // true if the movement starts form the end of the path. (the includes the distance progress along the path).
		float speed = 4.f;
		float restingTime = 0.5f;
		BounceType bounceType = bounceType_bounce; // what should happen when the end of the path is reached.
	};

	struct SGE_ENGINE_API State {
		float timeLeftToRest = 0.f; // The time that the follower need to spend resting.
		float distanceAlongPath =
		    0.f; // The distance travelled in the paths object space. the value is maintained the same if reversed is specified to the
		         // settings, the only thing that is changed is that the starting point is now the end point.
		float evalDistanceAlongPath = 0.f; // The actual distance used for evaluation along the path.
		bool speedIsPositive = true;
		vec3f ptWs = vec3f(0.f);

		bool isResting() const { return timeLeftToRest > 0.f; }
	};

	/// @param recursionDepth Also leave default value. Used to prevent endless recursion if there is a mistake inside the function.
	SGE_ENGINE_API Optional<State> compute(const float dt, GameWorld* const world, const Settings& settings, const State& prevState);
}; // namespace PathFollow

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
DefineTypeIdInline(TraitPath3D, 20'03'06'0000);
struct SGE_ENGINE_API TraitPath3D : public Trait {
	SGE_TraitDecl_BaseFamily(TraitPath3D);

	// Checks if the curve is empty.
	virtual bool isEmpty() const = 0;

	// Evaluates the the curve at the specified distance form the begining.
	virtual bool evaluateAtDistance(vec3f* outPosition, vec3f* outTanget, float const distance) = 0;

	// Retrieves the length, or an approximation of it.
	virtual float getTotalLength() = 0;
};

} // namespace sge
