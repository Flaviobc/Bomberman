/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingAnimation.cpp                            :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "graphics/features/RenderingAnimation.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <utility>

namespace
{
using engine::graphics::TransformRotationKey;
using engine::graphics::TransformTranslationKey;
using engine::graphics::detail::interpolateQuaternionShortestArc;
using engine::graphics::detail::inverseUnitQuaternion;
using engine::graphics::detail::multiplyQuaternions;
using engine::graphics::detail::normalizeQuaternion;
using engine::graphics::detail::Quaternion;
using engine::graphics::detail::Vector3;

Vector3 sampleTranslation(const std::vector<TransformTranslationKey> &keys,
                          float timeSeconds)
{
	std::size_t nextIndex = 1;
	while (nextIndex < keys.size() && keys[nextIndex].timeSeconds < timeSeconds)
	{
		++nextIndex;
	}
	if (nextIndex >= keys.size())
	{
		return {keys.back().x, keys.back().y, keys.back().z};
	}
	const TransformTranslationKey &previous = keys[nextIndex - 1];
	const TransformTranslationKey &next = keys[nextIndex];
	const float interval = next.timeSeconds - previous.timeSeconds;
	const float factor = interval > 0.0F
	                         ? (timeSeconds - previous.timeSeconds) / interval
	                         : 0.0F;
	return {previous.x + (next.x - previous.x) * factor,
	        previous.y + (next.y - previous.y) * factor,
	        previous.z + (next.z - previous.z) * factor};
}

Quaternion sampleRotation(const std::vector<TransformRotationKey> &keys,
                          float timeSeconds)
{
	std::size_t nextIndex = 1;
	while (nextIndex < keys.size() && keys[nextIndex].timeSeconds < timeSeconds)
	{
		++nextIndex;
	}
	const auto toQuaternion = [](const TransformRotationKey &key)
	{ return normalizeQuaternion({key.x, key.y, key.z, key.w}); };
	if (nextIndex >= keys.size())
	{
		return toQuaternion(keys.back());
	}
	const TransformRotationKey &previous = keys[nextIndex - 1];
	const TransformRotationKey &next = keys[nextIndex];
	const float interval = next.timeSeconds - previous.timeSeconds;
	const float factor = interval > 0.0F
	                         ? (timeSeconds - previous.timeSeconds) / interval
	                         : 0.0F;
	return interpolateQuaternionShortestArc(toQuaternion(previous),
	                                        toQuaternion(next), factor);
}
} // namespace

namespace engine::graphics::detail
{
bool RenderingAnimation::setTransformClips(
    const std::vector<TransformAnimationClip> &clips)
{
	if (clips.empty())
	{
		std::fputs("Object animation collection is invalid.\n", stderr);
		return false;
	}
	for (const TransformAnimationClip &clip : clips)
	{
		if (!std::isfinite(clip.durationSeconds) ||
		    clip.durationSeconds <= 0.0F || clip.translations.empty() ||
		    clip.rotations.empty())
		{
			std::fputs("Object animation clip is invalid.\n", stderr);
			return false;
		}
		float previousTime = -1.0F;
		for (const TransformTranslationKey &key : clip.translations)
		{
			if (!std::isfinite(key.timeSeconds) ||
			    key.timeSeconds < previousTime || !std::isfinite(key.x) ||
			    !std::isfinite(key.y) || !std::isfinite(key.z))
			{
				std::fputs("Object animation translation keys are invalid.\n",
				           stderr);
				return false;
			}
			previousTime = key.timeSeconds;
		}
		previousTime = -1.0F;
		for (const TransformRotationKey &key : clip.rotations)
		{
			const float lengthSquared =
			    key.x * key.x + key.y * key.y + key.z * key.z + key.w * key.w;
			if (!std::isfinite(key.timeSeconds) ||
			    key.timeSeconds < previousTime ||
			    !std::isfinite(lengthSquared) || lengthSquared <= 0.000001F)
			{
				std::fputs("Object animation rotation keys are invalid.\n",
				           stderr);
				return false;
			}
			previousTime = key.timeSeconds;
		}
	}

	transformClips_ = clips;
	activeClip_ = 0;
	const TransformAnimationClip &initialClip = clips.front();
	activeClipStartSeconds_ =
	    std::min(initialClip.translations.front().timeSeconds,
		         initialClip.rotations.front().timeSeconds);
	activeClipTimeSeconds_ = 0.0;
	currentTransform_ = sampleTransform(initialClip, activeClipStartSeconds_);
	crossfadeSourceTransform_ = currentTransform_;
	crossfadeElapsedSeconds_ = crossfadeDurationSeconds;
	std::fprintf(stderr,
	             "Object animation collection copied: clips=%zu active=0 "
	             "crossfade=%.3f s.\n",
	             clips.size(), crossfadeDurationSeconds);
	return true;
}

bool RenderingAnimation::setSkeletalPoseSource(
    animation::SkeletalPoseSource source)
{
	if (source.jointNodeIndices.size() != jointCount ||
	    source.inverseBindMatrices.size() != jointCount ||
	    source.clips.size() != transformClips_.size())
	{
		std::fputs("Skeletal pose source is invalid for this renderer.\n",
		           stderr);
		return false;
	}
	const animation::SkeletalPoseResult bindPose =
	    animation::evaluateSkeletalPose(source, nullptr, 0.0F);
	if (!bindPose || bindPose.jointPalette.size() != jointCount)
	{
		std::fprintf(stderr, "Skeletal bind pose evaluation failed: %s\n",
		             bindPose.error.c_str());
		return false;
	}

	skeletalSource_ = std::move(source);
	skeletalSourceClip_ = activeClip_;
	skeletalTargetClip_ = activeClip_;
	skeletalSourceTimeSeconds_ = activeClipTimeSeconds_;
	skeletalTargetTimeSeconds_ = activeClipTimeSeconds_;
	skeletalBlendStartFactor_ = 0.0F;
	skeletalBlendTargetFactor_ = 0.0F;
	skeletalBlendFactor_ = 0.0F;
	std::fprintf(stderr,
	             "Skeletal pose source copied: joints=%zu clips=%zu "
	             "palette_matrices=%zu.\n",
	             skeletalSource_->jointNodeIndices.size(),
	             skeletalSource_->clips.size(), jointCount);
	return true;
}

bool RenderingAnimation::selectClip(std::size_t clipIndex)
{
	if (clipIndex >= transformClips_.size())
	{
		std::fprintf(stderr, "Object animation index is invalid: %zu.\n",
		             clipIndex);
		return false;
	}
	if (activeClip_ == clipIndex)
	{
		return true;
	}
	if (skeletalSource_)
	{
		if (skeletalSourceClip_ == skeletalTargetClip_)
		{
			skeletalTargetClip_ = clipIndex;
			skeletalTargetTimeSeconds_ = 0.0;
			skeletalBlendFactor_ = 0.0F;
		}
		skeletalBlendStartFactor_ = skeletalBlendFactor_;
		if (clipIndex == skeletalSourceClip_)
		{
			skeletalBlendTargetFactor_ = 0.0F;
		}
		else if (clipIndex == skeletalTargetClip_)
		{
			skeletalBlendTargetFactor_ = 1.0F;
		}
		else
		{
			skeletalSourceClip_ = activeClip_;
			skeletalSourceTimeSeconds_ = activeClipTimeSeconds_;
			skeletalTargetClip_ = clipIndex;
			skeletalTargetTimeSeconds_ = 0.0;
			skeletalBlendStartFactor_ = 0.0F;
			skeletalBlendTargetFactor_ = 1.0F;
			skeletalBlendFactor_ = 0.0F;
		}
	}
	crossfadeSourceTransform_ = currentTransform_;
	activeClip_ = clipIndex;
	const TransformAnimationClip &clip = transformClips_[clipIndex];
	activeClipStartSeconds_ = std::min(clip.translations.front().timeSeconds,
	                                   clip.rotations.front().timeSeconds);
	activeClipTimeSeconds_ = 0.0;
	crossfadeElapsedSeconds_ = 0.0;
	std::fprintf(stderr,
	             "Object animation selected: index=%zu crossfade=%.3f s "
	             "skeletal=%zu->%zu factor=%.3f->%.3f.\n",
	             clipIndex, crossfadeDurationSeconds, skeletalSourceClip_,
	             skeletalTargetClip_, skeletalBlendStartFactor_,
	             skeletalBlendTargetFactor_);
	return true;
}

std::optional<AnimationPlaybackFrame>
RenderingAnimation::update(double deltaSeconds)
{
	if (transformClips_.empty() || !skeletalSource_ ||
	    activeClip_ >= skeletalSource_->clips.size())
	{
		std::fputs("Skeletal pose source is unavailable for rendering.\n",
		           stderr);
		return std::nullopt;
	}
	const TransformAnimationClip &animation = transformClips_[activeClip_];
	if (deltaSeconds > 0.0)
	{
		activeClipTimeSeconds_ =
		    std::fmod(activeClipTimeSeconds_ + deltaSeconds,
			          static_cast<double>(animation.durationSeconds));
	}
	const float animationTime =
	    activeClipStartSeconds_ + static_cast<float>(activeClipTimeSeconds_);
	const LocalTransform targetTransform =
	    sampleTransform(animation, animationTime);
	const float crossfadeFactor = static_cast<float>(std::clamp(
	    crossfadeElapsedSeconds_ / crossfadeDurationSeconds, 0.0, 1.0));
	currentTransform_ = interpolateTransform(crossfadeSourceTransform_,
	                                         targetTransform, crossfadeFactor);
	if (deltaSeconds > 0.0 && crossfadeFactor < 1.0F)
	{
		crossfadeElapsedSeconds_ = std::min(
		    crossfadeDurationSeconds, crossfadeElapsedSeconds_ + deltaSeconds);
	}

	if (skeletalSourceClip_ == skeletalTargetClip_)
	{
		advanceSkeletalClock(skeletalSourceClip_, skeletalSourceTimeSeconds_,
		                     deltaSeconds);
		skeletalTargetTimeSeconds_ = skeletalSourceTimeSeconds_;
	}
	else if (activeClip_ == skeletalSourceClip_)
	{
		advanceSkeletalClock(skeletalSourceClip_, skeletalSourceTimeSeconds_,
		                     deltaSeconds);
	}
	else
	{
		advanceSkeletalClock(skeletalTargetClip_, skeletalTargetTimeSeconds_,
		                     deltaSeconds);
	}
	skeletalBlendFactor_ = skeletalBlendStartFactor_ +
	                       crossfadeFactor * (skeletalBlendTargetFactor_ -
	                                          skeletalBlendStartFactor_);
	const animation::SkeletalPoseResult skeletalPose =
	    animation::evaluateBlendedSkeletalPose(
	        *skeletalSource_, skeletalSource_->clips[skeletalSourceClip_],
	        static_cast<float>(skeletalSourceTimeSeconds_),
	        skeletalSource_->clips[skeletalTargetClip_],
	        static_cast<float>(skeletalTargetTimeSeconds_),
	        skeletalBlendFactor_);
	if (!skeletalPose || skeletalPose.jointPalette.size() != jointCount)
	{
		std::fprintf(stderr, "Skeletal pose evaluation failed: %s\n",
		             skeletalPose.error.c_str());
		return std::nullopt;
	}

	AnimationPlaybackFrame frame;
	frame.localTranslation = currentTransform_.translation;
	frame.localRotation = currentTransform_.rotation;
	for (std::size_t jointIndex = 0; jointIndex < jointCount; ++jointIndex)
	{
		std::copy(skeletalPose.jointPalette[jointIndex].elements.begin(),
		          skeletalPose.jointPalette[jointIndex].elements.end(),
		          frame.jointPalette.begin() + jointIndex * 16);
	}
	if (crossfadeFactor >= 1.0F && skeletalSourceClip_ != skeletalTargetClip_)
	{
		if (activeClip_ == skeletalTargetClip_)
		{
			skeletalSourceClip_ = skeletalTargetClip_;
			skeletalSourceTimeSeconds_ = skeletalTargetTimeSeconds_;
		}
		else
		{
			skeletalTargetClip_ = skeletalSourceClip_;
			skeletalTargetTimeSeconds_ = skeletalSourceTimeSeconds_;
		}
		skeletalBlendStartFactor_ = 0.0F;
		skeletalBlendTargetFactor_ = 0.0F;
		skeletalBlendFactor_ = 0.0F;
		std::fprintf(stderr, "Skeletal crossfade completed: active=%zu.\n",
		             activeClip_);
	}
	return frame;
}

RenderingAnimation::LocalTransform
RenderingAnimation::sampleTransform(const TransformAnimationClip &clip,
                                    float timeSeconds) const
{
	const Vector3 sampledTranslation =
	    sampleTranslation(clip.translations, timeSeconds);
	const TransformTranslationKey &initialTranslation =
	    clip.translations.front();
	const TransformRotationKey &initialRotation = clip.rotations.front();
	const Quaternion initialQuaternion =
	    normalizeQuaternion({initialRotation.x, initialRotation.y,
		                     initialRotation.z, initialRotation.w});
	return {{sampledTranslation.x - initialTranslation.x,
	         sampledTranslation.y - initialTranslation.y,
	         sampledTranslation.z - initialTranslation.z},
	        normalizeQuaternion(multiplyQuaternions(
	            inverseUnitQuaternion(initialQuaternion),
	            sampleRotation(clip.rotations, timeSeconds)))};
}

RenderingAnimation::LocalTransform RenderingAnimation::interpolateTransform(
    const LocalTransform &from, const LocalTransform &to, float factor) const
{
	return {
	    {from.translation.x + (to.translation.x - from.translation.x) * factor,
		 from.translation.y + (to.translation.y - from.translation.y) * factor,
		 from.translation.z + (to.translation.z - from.translation.z) * factor},
	    interpolateQuaternionShortestArc(from.rotation, to.rotation, factor)};
}

void RenderingAnimation::advanceSkeletalClock(std::size_t clipIndex,
                                              double &timeSeconds,
                                              double deltaSeconds) const
{
	if (deltaSeconds <= 0.0)
	{
		return;
	}
	const float duration = transformClips_[clipIndex].durationSeconds;
	timeSeconds =
	    std::fmod(timeSeconds + deltaSeconds, static_cast<double>(duration));
}
} // namespace engine::graphics::detail
