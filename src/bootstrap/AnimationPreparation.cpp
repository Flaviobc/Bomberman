/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   AnimationPreparation.cpp                         :+:      :+:    :+: :+:      */
/*                                                    +:+ +:+   +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+ +#+#+#+#+#+   +#+      */
/*                                                +#+#+#+#+#+   +#+           +#+#+#+#+#+   */
/*   Created: 2026/10/08          by flaviobc      #+#    #+#             */
/*                                                                                    */
/* ******************************************************************************** */

#include "AnimationPreparation.hpp"

#include <cmath>
#include <cstdio>
#include <optional>
#include <utility>

bool bootstrap::prepareSkeletalPoseSource(
    const engine::assets::GlbDocument &document,
    engine::animation::SkeletalPoseSource &resolvedSource)
{
	const engine::assets::GlbSkin *playerSkin = nullptr;
	for (const engine::assets::GlbNode &node : document.nodes)
	{
		if (node.name == "Player" && node.skinIndex &&
		    *node.skinIndex < document.skins.size())
		{
			playerSkin = &document.skins[*node.skinIndex];
			break;
		}
	}
	if (!playerSkin)
	{
		return false;
	}
	engine::animation::SkeletalPoseSourceResult sourceResult =
	    engine::animation::buildSkeletalPoseSource(document, *playerSkin);
	if (!sourceResult)
	{
		std::fprintf(stderr, "GLB skeletal pose source failed: %s\n",
		             sourceResult.error.c_str());
		return false;
	}
	engine::animation::SkeletalPoseSource source =
	    std::move(*sourceResult.source);
	const engine::animation::SkeletalPoseClip *idle =
	    engine::animation::findSkeletalPoseClip(source, "Idle");
	const engine::animation::SkeletalPoseClip *walk =
	    engine::animation::findSkeletalPoseClip(source, "Walk");
	if (!idle || !walk)
	{
		return false;
	}
	std::fprintf(
	    stderr,
	    "GLB skeletal pose source: ownership=value nodes=%zu joints=%zu "
	    "inverse_binds=%zu clips=%zu\n",
	    source.nodes.size(), source.jointNodeIndices.size(),
	    source.inverseBindMatrices.size(), source.clips.size());
	const engine::animation::SkeletalPoseResult bindPose =
	    engine::animation::evaluateSkeletalPose(source, nullptr, 0.0F);
	const engine::animation::SkeletalPoseResult idlePose =
	    engine::animation::evaluateSkeletalPose(source, idle, 0.5F);
	const engine::animation::SkeletalPoseResult walkPose =
	    engine::animation::evaluateSkeletalPose(source, walk, 0.5F);
	const engine::animation::SkeletalPoseResult blendStart =
	    engine::animation::evaluateBlendedSkeletalPose(source, *idle, 0.5F,
		                                               *walk, 0.5F, 0.0F);
	const engine::animation::SkeletalPoseResult blendBeforeMiddle =
	    engine::animation::evaluateBlendedSkeletalPose(source, *idle, 0.5F,
		                                               *walk, 0.5F, 0.499F);
	const engine::animation::SkeletalPoseResult blendAfterMiddle =
	    engine::animation::evaluateBlendedSkeletalPose(source, *idle, 0.5F,
		                                               *walk, 0.5F, 0.501F);
	const engine::animation::SkeletalPoseResult blendEnd =
	    engine::animation::evaluateBlendedSkeletalPose(source, *idle, 0.5F,
		                                               *walk, 0.5F, 1.0F);
	if (!bindPose || !idlePose || !walkPose || !blendStart ||
	    !blendBeforeMiddle || !blendAfterMiddle || !blendEnd ||
	    bindPose.jointPalette.size() != 2 ||
	    idlePose.jointPalette.size() != 2 ||
	    walkPose.jointPalette.size() != 2 ||
	    blendStart.jointPalette.size() != 2 ||
	    blendBeforeMiddle.jointPalette.size() != 2 ||
	    blendAfterMiddle.jointPalette.size() != 2 ||
	    blendEnd.jointPalette.size() != 2)
	{
		std::fprintf(stderr,
		             "GLB skeletal pose failed: bind=%s idle=%s walk=%s "
		             "blend_start=%s blend_before=%s blend_after=%s "
		             "blend_end=%s\n",
		             bindPose.error.c_str(), idlePose.error.c_str(),
		             walkPose.error.c_str(), blendStart.error.c_str(),
		             blendBeforeMiddle.error.c_str(),
		             blendAfterMiddle.error.c_str(), blendEnd.error.c_str());
		return false;
	}
	const auto paletteDifference = [](const auto &first, const auto &second)
	{
		float difference = 0.0F;
		for (std::size_t jointIndex = 0; jointIndex < first.size();
		     ++jointIndex)
		{
			for (std::size_t elementIndex = 0;
			     elementIndex < first[jointIndex].elements.size();
			     ++elementIndex)
			{
				difference +=
				    std::fabs(first[jointIndex].elements[elementIndex] -
					          second[jointIndex].elements[elementIndex]);
			}
		}
		return difference;
	};
	const float upperDifference =
	    paletteDifference(bindPose.jointPalette, walkPose.jointPalette);
	const float blendStartError =
	    paletteDifference(idlePose.jointPalette, blendStart.jointPalette);
	const float blendEndError =
	    paletteDifference(walkPose.jointPalette, blendEnd.jointPalette);
	const float blendMiddleStep = paletteDifference(
	    blendBeforeMiddle.jointPalette, blendAfterMiddle.jointPalette);
	std::fprintf(
	    stderr,
	    "GLB joint palette: pose=bind joints=%zu upper_translation=(%.3f, "
	    "%.3f, %.3f)\n",
	    bindPose.jointPalette.size(), bindPose.jointPalette[1].elements[12],
	    bindPose.jointPalette[1].elements[13],
	    bindPose.jointPalette[1].elements[14]);
	std::fprintf(stderr,
	             "GLB joint palette: pose=Idle time=0.500 joints=%zu "
	             "upper_translation=(%.3f, %.3f, %.3f)\n",
	             idlePose.jointPalette.size(),
	             idlePose.jointPalette[1].elements[12],
	             idlePose.jointPalette[1].elements[13],
	             idlePose.jointPalette[1].elements[14]);
	std::fprintf(stderr,
	             "GLB joint palette: pose=Walk time=0.500 joints=%zu "
	             "upper_translation=(%.3f, %.3f, %.3f) difference=%.6f\n",
	             walkPose.jointPalette.size(),
	             walkPose.jointPalette[1].elements[12],
	             walkPose.jointPalette[1].elements[13],
	             walkPose.jointPalette[1].elements[14], upperDifference);
	std::fprintf(stderr,
	             "GLB skeletal blend: source=Idle target=Walk "
	             "source_time=0.500 target_time=0.500 "
	             "start_error=%.9f end_error=%.9f "
	             "middle_step_0.499_0.501=%.9f\n",
	             blendStartError, blendEndError, blendMiddleStep);
	if (upperDifference <= 0.001F || blendStartError > 0.000001F ||
	    blendEndError > 0.000001F || blendMiddleStep <= 0.0F ||
	    blendMiddleStep >= upperDifference * 0.01F)
	{
		return false;
	}
	const engine::animation::SkeletalPoseClip idleClip = *idle;
	const engine::animation::SkeletalPoseClip walkClip = *walk;
	source.clips = {idleClip, walkClip};
	resolvedSource = std::move(source);
	return true;
}

namespace
{
bool resolveTransformAnimation(const engine::assets::GlbDocument &document,
                               const char *clipName,
                               engine::graphics::TransformAnimationClip &result)
{
	std::optional<std::size_t> playerNodeIndex;
	for (std::size_t nodeIndex = 0; nodeIndex < document.nodes.size();
	     ++nodeIndex)
	{
		if (document.nodes[nodeIndex].name == "Player")
		{
			playerNodeIndex = nodeIndex;
			break;
		}
	}
	if (!playerNodeIndex)
	{
		return false;
	}
	for (const engine::assets::GlbAnimationClip &clip : document.animations)
	{
		if (clip.name != clipName)
		{
			continue;
		}
		result.durationSeconds = clip.durationSeconds;
		for (const engine::assets::GlbAnimationChannel &channel : clip.channels)
		{
			if (channel.samplerIndex >= clip.samplers.size() ||
			    channel.nodeIndex != *playerNodeIndex)
			{
				continue;
			}
			const engine::assets::GlbAnimationSampler &sampler =
			    clip.samplers[channel.samplerIndex];
			if (channel.path == engine::assets::GlbAnimationPath::Translation)
			{
				result.translations.reserve(sampler.times.size());
				for (std::size_t index = 0; index < sampler.times.size();
				     ++index)
				{
					const engine::assets::GlbAnimationTranslation &value =
					    sampler.translations[index];
					result.translations.push_back(
					    {sampler.times[index], value.x, value.y, value.z});
				}
			}
			else
			{
				result.rotations.reserve(sampler.times.size());
				for (std::size_t index = 0; index < sampler.times.size();
				     ++index)
				{
					const engine::assets::GlbAnimationRotation &value =
					    sampler.rotations[index];
					result.rotations.push_back({sampler.times[index], value.x,
					                            value.y, value.z, value.w});
				}
			}
		}
		if (result.durationSeconds <= 0.0F || result.translations.empty() ||
		    result.rotations.empty())
		{
			return false;
		}
		std::fprintf(stderr,
		             "Animation playback selected: clip=%s duration=%.3f "
		             "translations=%zu rotations=%zu.\n",
		             clipName, result.durationSeconds,
		             result.translations.size(), result.rotations.size());
		return true;
	}
	return false;
}
} // namespace

bool bootstrap::prepareTransformAnimations(
    const engine::assets::GlbDocument &document,
    std::vector<engine::graphics::TransformAnimationClip> &animations)
{
	animations.resize(2);
	if (!resolveTransformAnimation(document, "Idle", animations[0]))
	{
		std::fputs("GLB animation selection failed: Idle is incomplete.\n",
		           stderr);
		return false;
	}
	if (!resolveTransformAnimation(document, "Walk", animations[1]))
	{
		std::fputs("GLB animation selection failed: Walk is incomplete.\n",
		           stderr);
		return false;
	}
	return true;
}
