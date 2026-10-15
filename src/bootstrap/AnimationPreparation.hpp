/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   AnimationPreparation.hpp                         :+:      :+:    :+: :+:      */
/*                                                    +:+ +:+   +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+ +#+#+#+#+#+   +#+      */
/*                                                +#+#+#+#+#+   +#+           +#+#+#+#+#+   */
/*   Created: 2026/10/08          by flaviobc      #+#    #+#             */
/*                                                                                    */
/* ******************************************************************************** */

#pragma once

#include "animation/SkeletalPose.hpp"
#include "assets/GlbLoader.hpp"
#include "graphics/Renderer.hpp"

#include <vector>

namespace bootstrap
{
struct PreparedAnimations
{
	engine::animation::SkeletalPoseSource skeletalPose;
	std::vector<engine::graphics::TransformAnimationClip> transformClips;
};

bool prepareSkeletalPoseSource(
    const engine::assets::GlbDocument &document,
    engine::animation::SkeletalPoseSource &resolvedSource);
bool prepareTransformAnimations(
    const engine::assets::GlbDocument &document,
    std::vector<engine::graphics::TransformAnimationClip> &animations);
} // namespace bootstrap
