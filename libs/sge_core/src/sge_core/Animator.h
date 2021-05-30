#pragma once

#if 1

#include "model/EvaluatedModel.h"
#include "sge_core/AssetLibrary.h"
#include "sge_utils/utils/optional.h"
#include "sgecore_api.h"
#include <unordered_map>

namespace sge {

/// @brief Describes what should happen when an animation track finishes.
enum TrackTransition : int {
	trackTransition_loop,     ///< The animation should continue to loop when it ends. Example idle animation.
	trackTransition_stop,     ///< The animation should stop at the end frame. Example death animations.
	trackTransition_switchTo, ///< The animation should switch to another one when it ends. Example is when jump animation ends usually
	                          ///< we want to be in landing anticipation animation.
};

/// @brief AnimatorTrack describes a state in which our 3D model could be.
/// For example a character might be jumping, running, idling, dying and so on.
/// Each animation track could have multiple animations be available to it - again for example we might have multiple idle animations
/// to make the character feel a bit more alive.
///
/// AnimatorTrack state consists of the id of the track. This is defined by the user and it is assigned to some their own meaning:
/// it could mean that the character is idle, running and so on.
/// A transition - transition tell the animation what should happen when the animation ends. For example we want the idle and walking
/// animations to loop, while we want the dying animation to stop at the end.
struct AnimatorTrack {
	AnimatorTrack() = default;

	struct AnimationDonor {
		std::shared_ptr<Asset> modelAnimationDonor;
		int donorIndex = -1;
		int animationIndexInDonor = -1;
		float playbackSpeed = 1.f;

		const ModelAnimation* getAnimation() const {
			if (isAssetLoaded(modelAnimationDonor, AssetType::Model)) {
				return modelAnimationDonor->asModel()->model.animationAt(animationIndexInDonor);
			}

			return nullptr;
		}
	};

	std::vector<AnimationDonor> animations;

	/// The time that should be used to fade out the other animations that are playing and 100% fade in that one.
	float blendingTime = 0.f;

	TrackTransition transition = trackTransition_loop;
	/// If the @transition is set to animationTransition_switchTo then this hold the id of the animation that we need to switch to.
	int switchToPlaybackTrackId = -1;
};

struct SGE_CORE_API ModelAnimator {
	ModelAnimator() = default;

	void addTrack_begin(EvaluatedModel& modelToBeAnimated);
	void addTrack(int newTrackId, float fadeInTime, TrackTransition transition);
	void addAnimationToTrack(int newTrackId, const char* const donorModelPath, const char* donorAnimationName);
	void addTrack_finish();

	void playTrack(int const trackId, Optional<float> blendToSeconds = NullOptional());

	// Advanced the animation.
	void update(float const dt);

	/// @brief Generates the moments that are needed to evalute a 3d model in the specified state.
	/// Keep in mind that it is assument that the model has the same animation donors as the ModelAnimator in the same order.
	void computeEvalMoments(std::vector<EvalMomentSets>& outMoments);

  private:
	struct TrackPlayback {
		int trackId = -1;

		int iAnimation = -1;
		float timeInAnimation = 0.f;
		float unormWeight = 1.f;
	};

  private:
	std::unordered_map<int, AnimatorTrack> m_tracks;

	/// The main track that we are currently playing. This does not mean that we are playing only this track,
	/// there might be other track currently fading out.
	int m_playingTrack = -1;

	/// The state of each playing track.
	std::vector<TrackPlayback> m_playbacks;

	float fadeoutTimeTotal = 0.f;

	EvaluatedModel* m_modelToBeAnimated = nullptr;
};

} // namespace sge

#endif
