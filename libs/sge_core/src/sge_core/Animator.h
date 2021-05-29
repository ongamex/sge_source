#pragma once

#if 0

#include "model/EvaluatedModel.h"
#include "sgecore_api.h"
#include <unordered_map>

namespace sge {

enum AnimationTransition : int {
	animationTransition_loop,
	animationTransition_stop,
	// animationTransition_pingPong
	animationTransition_switchTo,
};

struct AnimatorAnimationTransition {
	int targetState = -1;
	float blendDurationSecs = 0.f; // The time time that we are going to blend during the transition.
};

struct AnimationSrc {
	AnimationSrc() = default;
	AnimationSrc(std::string srcModel, std::string animationName)
	    : srcModel(std::move(srcModel))
	    , animationName(std::move(animationName)) {}

	std::string srcModel; // The mdl file that contains the animaton.
	std::string animationName;
};

struct AnimatorAnimation {
	AnimatorAnimation() = default;

	std::string srcModel;      // The mdl file that contains the animaton.
	std::string animationName; // The name of the animation that we are going to play form that file,
	AnimationTransition transition = animationTransition_loop;

	// if the transition is set to animationTransition_switchTo then this hold the id of the animation that we need to switch to.
	int switchToAnimationId = -1;
	float playbackSpeed = 1.f;

	std::vector<AnimationSrc> animationSources;
};

struct SGE_CORE_API Animator {
	Animator() = default;

	const std::vector<EvalMomentSets>& getEvalMoments() const { return m_moments; }

	void setModel(const char* const modelPath);
	std::shared_ptr<Asset>& getModel() { return m_model; }

	void addAnimation(int const animid, const char* const srcModel, const char* const animName);

	AnimatorAnimation& getAnimation(int const animid) { return m_animations[animid]; }

	void playAnimation(int const animid, bool dontBlend = true);

	// Advanced the animation.
	void update(float const dt);

  private:
	void pickAnimation(EvalMomentSets& moment, int const animid, float pevanimProgress);

	void clear() { *this = Animator(); }

	// The model that we are animating.
	std::shared_ptr<Asset> m_model;

	// For each animation id, we associate an animation (or multiple animations).
	// Multiple animations mith be needed to example if we got multiple "idle" animations in the *.mdl files (or across multiple files)
	// we want to basically pick different animation each time.
	std::unordered_map<int, AnimatorAnimation> m_animations;

	// A reference to all models used by the animator.
	// NOTE: Now that I think about it, do we really need thid LUT, we could directly use the
	// the asset library, I doubt that it is going to get parallel?
	std::unordered_map<std::string, std::shared_ptr<Asset>> m_modelsLUT;

	//
	// std::unordered_map<int, int> m_animTransitions;

	// The animation that we are currently playing as the main thing.
	int m_playingAnimId = -1;

	// Intrernal state data.
	std::vector<EvalMomentSets> m_moments;
};

} // namespace sge

#endif