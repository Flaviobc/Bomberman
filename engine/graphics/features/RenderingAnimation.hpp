/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingAnimation.hpp                            :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

#include "graphics/foundation/RenderingMath.hpp"

#include "animation/SkeletalPose.hpp"
#include "graphics/Renderer.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

namespace engine::graphics::detail
{
struct AnimationPlaybackFrame
{
	Vector3 localTranslation;
	Quaternion localRotation;
	std::array<float, 32> jointPalette{};
};

class RenderingAnimation
{
  public:
	static constexpr std::size_t jointCount = 2;
	static constexpr double crossfadeDurationSeconds = 0.12;

	bool setTransformClips(const std::vector<TransformAnimationClip> &clips);
	bool setSkeletalPoseSource(animation::SkeletalPoseSource source);
	bool selectClip(std::size_t clipIndex);
	[[nodiscard]] std::optional<AnimationPlaybackFrame>
	update(double deltaSeconds);

  private:
	struct LocalTransform
	{
		Vector3 translation;
		Quaternion rotation;
	};

	[[nodiscard]] LocalTransform
	sampleTransform(const TransformAnimationClip &clip,
	                float timeSeconds) const;
	[[nodiscard]] LocalTransform
	interpolateTransform(const LocalTransform &from, const LocalTransform &to,
	                     float factor) const;
	void advanceSkeletalClock(std::size_t clipIndex, double &timeSeconds,
	                          double deltaSeconds) const;

	std::vector<TransformAnimationClip> transformClips_;
	std::size_t activeClip_ = 0;
	float activeClipStartSeconds_ = 0.0F;
	double activeClipTimeSeconds_ = 0.0;
	LocalTransform currentTransform_;
	// Crossfade captures the visible pose, so a rapid reversal has no jump.
	LocalTransform crossfadeSourceTransform_;
	double crossfadeElapsedSeconds_ = crossfadeDurationSeconds;
	std::optional<animation::SkeletalPoseSource> skeletalSource_;
	// Source and target clocks continue independently during a skeletal blend.
	std::size_t skeletalSourceClip_ = 0;
	std::size_t skeletalTargetClip_ = 0;
	double skeletalSourceTimeSeconds_ = 0.0;
	double skeletalTargetTimeSeconds_ = 0.0;
	float skeletalBlendStartFactor_ = 0.0F;
	float skeletalBlendTargetFactor_ = 0.0F;
	float skeletalBlendFactor_ = 0.0F;
};
} // namespace engine::graphics::detail
