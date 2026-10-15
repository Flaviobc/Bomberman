/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   SkeletalComposition.cpp                           :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "SkeletalComposition.hpp"

#include <cstdint>

namespace engine::animation::detail
{
bool validateSkeletalPoseSource(const SkeletalPoseSource &source,
                                std::string &error)
{
	if (source.nodes.empty() || source.jointNodeIndices.empty() ||
	    source.jointNodeIndices.size() != source.inverseBindMatrices.size())
	{
		error = "skeletal source is invalid";
		return false;
	}
	return true;
}

SkeletalPoseResult
composeJointPalette(const SkeletalPoseSource &source,
                    const std::vector<LocalTransform> &localTransforms)
{
	SkeletalPoseResult result;
	if (localTransforms.size() != source.nodes.size())
	{
		result.error = "skeletal local pose count is invalid";
		return result;
	}

	std::vector<JointMatrix> globalNodeMatrices(source.nodes.size());
	std::vector<std::uint8_t> traversalState(source.nodes.size());
	const auto composeGlobalMatrix = [&](auto &&composeParentFirst,
	                                     std::size_t nodeIndex) -> bool
	{
		if (nodeIndex >= source.nodes.size() || traversalState[nodeIndex] == 1)
		{
			result.error = "skeletal source hierarchy is invalid";
			return false;
		}
		if (traversalState[nodeIndex] == 2)
		{
			return true;
		}
		traversalState[nodeIndex] = 1;
		globalNodeMatrices[nodeIndex] =
		    makeLocalTransformMatrix(localTransforms[nodeIndex]);
		if (source.nodes[nodeIndex].parentNodeIndex)
		{
			const std::size_t parentNodeIndex =
			    *source.nodes[nodeIndex].parentNodeIndex;
			if (!composeParentFirst(composeParentFirst, parentNodeIndex))
			{
				return false;
			}
			// With column vectors, local acts first: global = parentGlobal *
			// local.
			globalNodeMatrices[nodeIndex] =
			    multiplyMatrices(globalNodeMatrices[parentNodeIndex],
				                 globalNodeMatrices[nodeIndex]);
		}
		traversalState[nodeIndex] = 2;
		return true;
	};

	for (std::size_t jointNodeIndex : source.jointNodeIndices)
	{
		if (!composeGlobalMatrix(composeGlobalMatrix, jointNodeIndex))
		{
			return result;
		}
	}

	result.jointPalette.reserve(source.jointNodeIndices.size());
	for (std::size_t jointIndex = 0;
	     jointIndex < source.jointNodeIndices.size(); ++jointIndex)
	{
		// inverseBind removes the mesh bind-space transform. The product maps a
		// bind-space vertex into the joint's current model-space pose.
		result.jointPalette.push_back(multiplyMatrices(
		    globalNodeMatrices[source.jointNodeIndices[jointIndex]],
		    source.inverseBindMatrices[jointIndex]));
	}
	return result;
}
} // namespace engine::animation::detail
