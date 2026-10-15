/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   SkeletalPoseSource.cpp                            :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "SkeletalPose.hpp"

#include "assets/GlbLoader.hpp"

#include <algorithm>
#include <utility>

namespace
{
using namespace engine;

bool sourceContainsJoint(const animation::SkeletalPoseSource &source,
                         std::size_t nodeIndex)
{
	return std::find(source.jointNodeIndices.begin(),
	                 source.jointNodeIndices.end(),
	                 nodeIndex) != source.jointNodeIndices.end();
}

void copyRestHierarchy(const assets::GlbDocument &document,
                       animation::SkeletalPoseSource &destination)
{
	destination.nodes.reserve(document.nodes.size());
	for (const assets::GlbNode &node : document.nodes)
	{
		const assets::GlbNodeTransform &transform = node.localTransform;
		destination.nodes.push_back(
		    {node.parentNodeIndex, transform.translationX,
			 transform.translationY, transform.translationZ,
			 transform.rotationX, transform.rotationY, transform.rotationZ,
			 transform.rotationW, transform.scaleX, transform.scaleY,
			 transform.scaleZ});
	}
}

bool copySkinJointData(const assets::GlbSkin &skin,
                       animation::SkeletalPoseSource &destination,
                       std::string &error)
{
	if (skin.jointNodeIndices.empty() ||
	    skin.jointNodeIndices.size() != skin.inverseBindMatrices.size())
	{
		error = "skin source has mismatched joint data";
		return false;
	}
	for (std::size_t jointNodeIndex : skin.jointNodeIndices)
	{
		if (jointNodeIndex >= destination.nodes.size())
		{
			error = "skin source joint is invalid";
			return false;
		}
		destination.jointNodeIndices.push_back(jointNodeIndex);
	}
	for (const assets::GlbInverseBindMatrix &inverseBindMatrix :
	     skin.inverseBindMatrices)
	{
		destination.inverseBindMatrices.push_back({inverseBindMatrix.elements});
	}
	return true;
}

bool copyJointAnimationChannel(const assets::GlbAnimationClip &sourceClip,
                               const assets::GlbAnimationChannel &sourceChannel,
                               animation::SkeletalAnimationChannel &destination,
                               std::string &error)
{
	const assets::GlbAnimationSampler &sourceSampler =
	    sourceClip.samplers[sourceChannel.samplerIndex];
	if (sourceSampler.times.size() < 2 ||
	    sourceSampler.path != sourceChannel.path)
	{
		error = "skin source animation sampler is invalid";
		return false;
	}
	destination.nodeIndex = sourceChannel.nodeIndex;
	destination.path =
	    sourceChannel.path == assets::GlbAnimationPath::Translation
	        ? animation::SkeletalAnimationPath::Translation
	        : animation::SkeletalAnimationPath::Rotation;
	destination.times = sourceSampler.times;
	if (sourceChannel.path == assets::GlbAnimationPath::Translation)
	{
		if (sourceSampler.translations.size() != sourceSampler.times.size())
		{
			error = "skin source translation keys are invalid";
			return false;
		}
		for (const assets::GlbAnimationTranslation &translation :
		     sourceSampler.translations)
		{
			destination.values.push_back(
			    {translation.x, translation.y, translation.z, 0});
		}
	}
	else
	{
		if (sourceSampler.rotations.size() != sourceSampler.times.size())
		{
			error = "skin source rotation keys are invalid";
			return false;
		}
		for (const assets::GlbAnimationRotation &rotation :
		     sourceSampler.rotations)
		{
			destination.values.push_back(
			    {rotation.x, rotation.y, rotation.z, rotation.w});
		}
	}
	return true;
}

bool copyJointAnimationClips(const assets::GlbDocument &document,
                             animation::SkeletalPoseSource &destination,
                             std::string &error)
{
	for (const assets::GlbAnimationClip &sourceClip : document.animations)
	{
		animation::SkeletalPoseClip destinationClip{sourceClip.name, {}};
		for (const assets::GlbAnimationChannel &sourceChannel :
		     sourceClip.channels)
		{
			if (sourceChannel.samplerIndex >= sourceClip.samplers.size() ||
			    !sourceContainsJoint(destination, sourceChannel.nodeIndex))
			{
				continue;
			}
			animation::SkeletalAnimationChannel destinationChannel;
			if (!copyJointAnimationChannel(sourceClip, sourceChannel,
			                               destinationChannel, error))
			{
				return false;
			}
			destinationClip.channels.push_back(std::move(destinationChannel));
		}
		destination.clips.push_back(std::move(destinationClip));
	}
	return true;
}
} // namespace

namespace engine::animation
{
SkeletalPoseSourceResult
buildSkeletalPoseSource(const assets::GlbDocument &document,
                        const assets::GlbSkin &skin)
{
	SkeletalPoseSource source;
	copyRestHierarchy(document, source);
	std::string error;
	if (!copySkinJointData(skin, source, error) ||
	    !copyJointAnimationClips(document, source, error))
	{
		return {std::nullopt, std::move(error)};
	}
	return {std::move(source), {}};
}
} // namespace engine::animation
