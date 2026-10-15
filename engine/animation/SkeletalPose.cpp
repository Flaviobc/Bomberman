/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   SkeletalPose.cpp                                  :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/01          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/10/04          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "SkeletalPose.hpp"

#include "SkeletalComposition.hpp"
#include "SkeletalMath.hpp"
#include "SkeletalSampling.hpp"

#include <cmath>

namespace engine::animation
{
SkeletalPoseSourceResult::operator bool() const
{
	return source.has_value();
}

SkeletalPoseResult::operator bool() const
{
	return error.empty();
}

const SkeletalPoseClip *findSkeletalPoseClip(const SkeletalPoseSource &source,
                                             const char *name)
{
	for (const SkeletalPoseClip &clip : source.clips)
	{
		if (clip.name == name)
		{
			return &clip;
		}
	}
	return nullptr;
}

SkeletalPoseResult evaluateSkeletalPose(const SkeletalPoseSource &source,
                                        const SkeletalPoseClip *clip,
                                        float timeSeconds)
{
	SkeletalPoseResult result;
	if (!detail::validateSkeletalPoseSource(source, result.error))
	{
		return result;
	}
	std::vector<detail::LocalTransform> localTransforms;
	if (!detail::sampleLocalTransforms(source, clip, timeSeconds,
	                                   localTransforms, result.error))
	{
		return result;
	}
	return detail::composeJointPalette(source, localTransforms);
}

SkeletalPoseResult evaluateBlendedSkeletalPose(
    const SkeletalPoseSource &source, const SkeletalPoseClip &sourceClip,
    float sourceTimeSeconds, const SkeletalPoseClip &targetClip,
    float targetTimeSeconds, float blendFactor)
{
	SkeletalPoseResult result;
	if (!detail::validateSkeletalPoseSource(source, result.error) ||
	    !std::isfinite(blendFactor) || blendFactor < 0.0F || blendFactor > 1.0F)
	{
		if (result.error.empty())
		{
			result.error = "skeletal blend factor is invalid";
		}
		return result;
	}

	std::vector<detail::LocalTransform> sourceLocalTransforms;
	std::vector<detail::LocalTransform> targetLocalTransforms;
	if (!detail::sampleLocalTransforms(source, &sourceClip, sourceTimeSeconds,
	                                   sourceLocalTransforms, result.error) ||
	    !detail::sampleLocalTransforms(source, &targetClip, targetTimeSeconds,
	                                   targetLocalTransforms, result.error))
	{
		return result;
	}

	// Both clips are sampled in parent-local space. Their local transforms are
	// blended first; hierarchy composition then runs once on the blended pose.
	std::vector<detail::LocalTransform> blendedLocalTransforms;
	blendedLocalTransforms.reserve(sourceLocalTransforms.size());
	for (std::size_t nodeIndex = 0; nodeIndex < sourceLocalTransforms.size();
	     ++nodeIndex)
	{
		blendedLocalTransforms.push_back(detail::blendLocalTransforms(
		    sourceLocalTransforms[nodeIndex], targetLocalTransforms[nodeIndex],
		    blendFactor));
	}
	return detail::composeJointPalette(source, blendedLocalTransforms);
}
} // namespace engine::animation
