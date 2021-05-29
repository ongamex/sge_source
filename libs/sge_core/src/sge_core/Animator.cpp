#if 0
#include "Animator.h"
#include "sge_core/AssetLibrary.h"
#include "sge_core/ICore.h"
#include "sge_utils/utils/common.h"

namespace sge {

template <typename TStdMap>
typename TStdMap::mapped_type* find_in_map(TStdMap& map, const typename TStdMap::key_type& key) {
	auto itr = map.find(key);
	if (itr == map.end())
		return nullptr;
	return &itr->second;
}

template <typename TStdMap>
const typename TStdMap::mapped_type* find_in_map(const TStdMap& map, const typename TStdMap::key_type& key) {
	auto itr = map.find(key);
	if (itr == map.end())
		return nullptr;
	return &itr->second;
}

void Animator::setModel(const char* const modelPath) {
	clear();

	AssetLibrary* const assetLib = getCore()->getAssetLib();
	m_model = assetLib->getAsset(AssetType::Model, modelPath, true);
}

void Animator::addAnimation(int const animid, const char* const srcModel, const char* const animName) {
	AssetLibrary* const assetLib = getCore()->getAssetLib();

	AnimatorAnimation& anim = m_animations[animid];
	anim.animationSources.emplace_back(AnimationSrc(srcModel, animName));
	m_modelsLUT[srcModel] = assetLib->getAsset(AssetType::Model, srcModel, true);
}

void Animator::playAnimation(int const animid, bool dontBlend) {
	if (m_playingAnimId == animid)
		return;

	m_playingAnimId = animid;

	// Clean up the current state.
	if (dontBlend) {
		m_moments.clear();
	}

	EvalMomentSets newMoment;
	pickAnimation(newMoment, animid, 0.f);

	push_front(m_moments, newMoment);

	if (m_moments.size() == 1) {
		m_moments[0].weight = 1.f;
		return;
	}

	m_moments[0].weight = 0.f;
}

void Animator::pickAnimation(EvalMomentSets& moment, int const animid, float pevanimProgress) {
	const auto itrAnimation = m_animations.find(animid);
	if (itrAnimation == m_animations.end()) {
		sgeAssert(false && "Expecting animation id to be vaild");
		return;
	}

	const std::vector<AnimationSrc>& animationSources = itrAnimation->second.animationSources;
	sgeAssert(animationSources.size() > 0);

	// TODO: Pick a random number for the random animation source of the animation. To it in a deteministic way.
	int const randomIndex = 0;
	const AnimationSrc& animation = animationSources[randomIndex];

	std::shared_ptr<Asset>* const pSrcModel = find_in_map(m_modelsLUT, animation.srcModel);

	if (pSrcModel == nullptr) {
		sgeAssert(false && "Unexpected null pointer in pickAnimation()");
		return;
	}

	if (isAssetLoaded(*pSrcModel) == false) {
		sgeAssert(false && "pickAnimation expects the model asset to be loaded");
		return;
	}

	moment = EvalMomentSets(&pSrcModel->get()->asModel()->model, animation.animationName, pevanimProgress, 1.f);
}

void Animator::update(float const dt) {
	float totalNonFocusWeight = 0.f;

	int animtoplay = -1;

	for (int iMoment = 0; iMoment < m_moments.size(); ++iMoment) {
		EvalMomentSets& moment = m_moments[iMoment];

		if (moment.model == nullptr) {
			continue;
		}

		moment.time += dt;

		if (iMoment != 0) {
			const float kTransitionTimeSeconds = 0.05f;
			moment.weight -= dt * (1.f / kTransitionTimeSeconds);

			if (moment.weight <= 0.f) {
				m_moments.erase(m_moments.begin() + iMoment);
				iMoment--;
				continue;
			}

			totalNonFocusWeight += moment.weight;
		}

		const Model::AnimationInfo* const animInfo = moment.model->findAnimation(moment.animationName);

		if (animInfo == nullptr)
			continue;

		int const signedRepeatCnt = (int)(moment.time / animInfo->duration);
		int const repeatCnt = abs(signedRepeatCnt);
		moment.time = moment.time - animInfo->duration * (float)signedRepeatCnt;

		if (moment.time < 0.f) {
			moment.time += animInfo->duration;
		}

		// The animation has ended. Find what animation should be played next.
		if (repeatCnt != 0 && iMoment == 0) {
			const AnimatorAnimation& animationThatJustEnded = m_animations[m_playingAnimId];

			animtoplay = m_playingAnimId; // assume looping,
			if (animationThatJustEnded.transition == animationTransition_loop) {
				animtoplay = m_playingAnimId;
			} else if (animationThatJustEnded.transition == animationTransition_switchTo) {
				animtoplay = animationThatJustEnded.switchToAnimationId;
			} else if (animationThatJustEnded.transition == animationTransition_stop) {
				moment.time = animInfo->duration;
				animtoplay = m_playingAnimId;
			}

			sgeAssert(m_animations.find(animtoplay) != m_animations.end() && "The switch-to animation doesn't exist");
		}
	}

	if (m_moments.size() > 1) {
		m_moments[0].weight = 1.f - totalNonFocusWeight;
	}

	if (m_moments.size() == 1) {
		m_moments[0].weight = 1.f;
	}

	if (animtoplay >= 0)
		playAnimation(animtoplay, false);
}

} // namespace sge

#endif