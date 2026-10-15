/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   SkeletalSampling.cpp                              :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "SkeletalSampling.hpp"

#include <algorithm>
#include <cmath>

namespace
{
using engine::animation::SkeletalAnimationChannel;
using engine::animation::SkeletalNodeSource;
using engine::animation::SkeletalPoseSource;
using engine::animation::detail::LocalTransform;

LocalTransform restLocalTransform(const SkeletalNodeSource &node)
{
	return {node.translationX, node.translationY, node.translationZ,
	        node.rotationX,    node.rotationY,    node.rotationZ,
	        node.rotationW,    node.scaleX,       node.scaleY,
	        node.scaleZ};
}

bool targetsJoint(const SkeletalPoseSource &source, std::size_t nodeIndex)
{
	return std::find(source.jointNodeIndices.begin(),
	                 source.jointNodeIndices.end(),
	                 nodeIndex) != source.jointNodeIndices.end();
}

struct KeyframeSegment
{
	std::size_t firstKeyIndex;
	float interpolationFactor;
};

KeyframeSegment findKeyframeSegment(const SkeletalAnimationChannel &channel,
                                    float timeSeconds)
{
	const float firstTime = channel.times.front();
	const float clipDuration = channel.times.back() - firstTime;
	float wrappedTime =
	    clipDuration > 0 ? std::fmod(timeSeconds, clipDuration) : 0;
	if (wrappedTime < 0)
	{
		wrappedTime += clipDuration;
	}
	wrappedTime += firstTime;

	const auto upperKey = std::upper_bound(channel.times.begin(),
	                                       channel.times.end(), wrappedTime);
	if (upperKey == channel.times.begin())
	{
		return {0, 0};
	}
	if (upperKey == channel.times.end())
	{
		return {channel.times.size() - 2, 1};
	}
	const std::size_t firstKeyIndex =
	    static_cast<std::size_t>(upperKey - channel.times.begin() - 1);
	const float segmentDuration =
	    channel.times[firstKeyIndex + 1] - channel.times[firstKeyIndex];
	return {firstKeyIndex,
	        segmentDuration > 0
	            ? (wrappedTime - channel.times[firstKeyIndex]) / segmentDuration
				: 0};
}
} // namespace

namespace engine::animation::detail
{
bool sampleLocalTransforms(const SkeletalPoseSource &source,
                           const SkeletalPoseClip *clip, float timeSeconds,
                           std::vector<LocalTransform> &localTransforms,
                           std::string &error)
{
	if (!std::isfinite(timeSeconds))
	{
		error = "skeletal pose time is invalid";
		return false;
	}
	localTransforms.clear();
	localTransforms.reserve(source.nodes.size());
	for (const SkeletalNodeSource &node : source.nodes)
	{
		localTransforms.push_back(restLocalTransform(node));
	}
	if (!clip)
	{
		return true;
	}

	for (const SkeletalAnimationChannel &channel : clip->channels)
	{
		if (channel.nodeIndex >= localTransforms.size() ||
		    !targetsJoint(source, channel.nodeIndex))
		{
			continue;
		}
		if (channel.times.size() < 2 ||
		    channel.values.size() != channel.times.size())
		{
			error = "skeletal source animation channel is invalid";
			return false;
		}

		const KeyframeSegment keyframes =
		    findKeyframeSegment(channel, timeSeconds);
		LocalTransform &localTransform = localTransforms[channel.nodeIndex];
		const std::array<float, 4> &firstValue =
		    channel.values[keyframes.firstKeyIndex];
		const std::array<float, 4> &secondValue =
		    channel.values[keyframes.firstKeyIndex + 1];
		if (channel.path == SkeletalAnimationPath::Translation)
		{
			localTransform.translationX =
			    firstValue[0] + keyframes.interpolationFactor *
			                        (secondValue[0] - firstValue[0]);
			localTransform.translationY =
			    firstValue[1] + keyframes.interpolationFactor *
			                        (secondValue[1] - firstValue[1]);
			localTransform.translationZ =
			    firstValue[2] + keyframes.interpolationFactor *
			                        (secondValue[2] - firstValue[2]);
		}
		else
		{
			interpolateQuaternionShortestArc(localTransform, firstValue,
			                                 secondValue,
			                                 keyframes.interpolationFactor);
		}
	}
	return true;
}
} // namespace engine::animation::detail
