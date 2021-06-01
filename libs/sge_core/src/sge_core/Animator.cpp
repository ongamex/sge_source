
#include "Animator.h"
#include "sge_core/AssetLibrary.h"
#include "sge_core/ICore.h"
#include "sge_utils/utils/common.h"

namespace sge {

void ModelAnimator::addTrack_begin(EvaluatedModel& modelToBeAnimated) {
	m_modelToBeAnimated = &modelToBeAnimated;
}

void ModelAnimator::addTrack(int newTrackId, float fadeInTime, TrackTransition transition) {
	AnimatorTrack& track = m_tracks[newTrackId];
	track.blendingTime = fadeInTime;
	track.transition = transition;
}

void ModelAnimator::addAnimationToTrack(int newTrackId, const char* const donorModelPath, const char* donorAnimationName) {
	AnimatorTrack& track = m_tracks[newTrackId];

	std::shared_ptr<Asset> donorModel = getCore()->getAssetLib()->getAsset(donorModelPath, true);
	if (isAssetLoaded(donorModel, AssetType::Model)) {
		AnimatorTrack::AnimationDonor donor;
		donor.modelAnimationDonor = donorModel;
		donor.donorIndex = m_modelToBeAnimated->addAnimationDonor(donorModel);
		donor.animationIndexInDonor = donorModel->asModel()->model.getAnimationIndexByName(donorAnimationName);

		track.animations.push_back(donor);
	}
}

void ModelAnimator::addTrack_finish() {
}

void ModelAnimator::playTrack(int const trackId, Optional<float> blendToSeconds) {
	// If we are already playing the same track do nothing.
	if (m_playingTrack == trackId) {
		return;
	}

	const auto& itrTrackDesc = m_tracks.find(trackId);

	if (itrTrackDesc == m_tracks.end()) {
		sgeAssert(false && "Invalid track id.");
		return;
	}

	fadeoutTimeTotal = blendToSeconds ? *blendToSeconds : itrTrackDesc->second.blendingTime;
	if (fadeoutTimeTotal == 0.f) {
		m_playbacks.clear();
		fadeoutTimeTotal = 0.f;
	}

	TrackPlayback playback;

	playback.trackId = trackId;
	playback.timeInAnimation = 0.f;
	playback.iAnimation = 0; // TODO: randomize this.
	// If there is going to be a fadeout start form 0 weight growing to one, to smoothly transition.
	// otherwise durectly use 100% weight.
	playback.unormWeight = m_playbacks.empty() ? 1.f : 0.f;

	m_playbacks.emplace_back(playback);
}

void ModelAnimator::update(float const dt) {
	if (m_playbacks.empty()) {
		return;
	}

	int switchToPlayTrackId = -1;

	// Update the tracks progress.
	for (int iPlayback = 0; iPlayback < m_playbacks.size(); ++iPlayback) {
		TrackPlayback& playback = m_playbacks[iPlayback];

		const AnimatorTrack& trackInfo = m_tracks[playback.trackId];

		AnimatorTrack::AnimationDonor animationDonor = trackInfo.animations[playback.iAnimation];
		const ModelAnimation* const animInfo = animationDonor.getAnimation();

		if (animInfo == nullptr) {
			// This should never happen, all the animations should be available.
			// Handle this so we do not crash.
			sgeAssert(false);
			m_playbacks.erase(m_playbacks.begin() + iPlayback);
			--iPlayback;
			continue;
		}

		// Advance the playback time.
		playback.timeInAnimation += dt;

		int const signedRepeatCnt = (int)(playback.timeInAnimation / animInfo->durationSec);
		int const repeatCnt = abs(signedRepeatCnt);

		// In that case the animation has ended. Handle the track transitions.
		if (repeatCnt != 0) {
			bool trackNeedsRemoving = false;

			switch (trackInfo.transition) {
				case trackTransition_loop: {
					playback.timeInAnimation = playback.timeInAnimation - animInfo->durationSec * (float)signedRepeatCnt;
					playback.iAnimation = 0; // TODO: randomize the animation.
				} break;
				case trackTransition_stop: {
					playback.timeInAnimation = animInfo->durationSec;
				} break;
				case trackTransition_switchTo: {
					// In that case we need to switch to another track if we aren't already in it.
					if (m_playingTrack != playback.trackId && trackInfo.switchToPlaybackTrackId != playback.trackId &&
					    trackInfo.switchToPlaybackTrackId != -1) {
						switchToPlayTrackId = trackInfo.switchToPlaybackTrackId;
					}

					// Finally remove that track it is no longer needed we assume that the new
					// track that we've switched to blends perfectly.
					trackNeedsRemoving = true;

				} break;

				default:
					// Unimplemented transition, assert and make it stop.
					sgeAssert(false && "Unimplemented transiton");
					playback.timeInAnimation = animInfo->durationSec;
					break;
			}

			if (trackNeedsRemoving) {
				m_playbacks.erase(m_playbacks.begin() + iPlayback);
				--iPlayback;
				continue;
			}
		}
	}

	if (switchToPlayTrackId != -1) {
		playTrack(switchToPlayTrackId);
	}

	// Update the track weights.
	if (fadeoutTimeTotal > 1e-6f) {
		const float fadePercentage = dt / fadeoutTimeTotal;

		for (int iPlayback = 0; iPlayback < m_playbacks.size(); ++iPlayback) {
			TrackPlayback& playback = m_playbacks[iPlayback];

			// The last element here is the primary track. It needs to fade in.
			if (iPlayback != (int(m_playbacks.size()) - 1)) {
				playback.unormWeight -= fadePercentage;
				if (playback.unormWeight <= 0.f) {
					m_playbacks.erase(m_playbacks.begin() + iPlayback);
					--iPlayback;
					continue;
				}
			} else {
				playback.unormWeight += fadePercentage;
				playback.unormWeight = clamp01(playback.unormWeight);
			}
		}
	}
}

void ModelAnimator::computeEvalMoments(std::vector<EvalMomentSets>& outMoments) {
	outMoments.clear();

	if (m_playbacks.empty()) {
		// In that case evaluate at static moment.
		outMoments.push_back(EvalMomentSets());
	}

	const float maxUnormWeightTotal = float(m_playbacks.size());

	float totalWeigth = 0.f;
	for (int iPlayback = 0; iPlayback < m_playbacks.size(); ++iPlayback) {
		TrackPlayback& playback = m_playbacks[iPlayback];
		const AnimatorTrack& trackInfo = m_tracks[playback.trackId];

		EvalMomentSets momentForTrack;
		momentForTrack.weight = playback.unormWeight / maxUnormWeightTotal;
		momentForTrack.time = playback.timeInAnimation;
		momentForTrack.donorIndex = trackInfo.animations[playback.iAnimation].donorIndex;
		momentForTrack.animationIndex = trackInfo.animations[playback.iAnimation].animationIndexInDonor;

		totalWeigth += momentForTrack.weight;

		outMoments.push_back(momentForTrack);
	}

	// Finally normalize the weigths to sum to 1.
	if (totalWeigth > 1e-6f) {
		for (EvalMomentSets& m : outMoments) {
			m.weight = m.weight / totalWeigth;
		}
	} else {
		sgeAssert(false);
		outMoments.clear();
		outMoments.push_back(EvalMomentSets());
	}
}
} // namespace sge
