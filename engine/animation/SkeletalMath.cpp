/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   SkeletalMath.cpp                                  :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "SkeletalMath.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
void normalizeRotation(engine::animation::detail::LocalTransform &transform)
{
	const float length = std::sqrt(transform.rotationX * transform.rotationX +
	                               transform.rotationY * transform.rotationY +
	                               transform.rotationZ * transform.rotationZ +
	                               transform.rotationW * transform.rotationW);
	if (length > std::numeric_limits<float>::epsilon())
	{
		transform.rotationX /= length;
		transform.rotationY /= length;
		transform.rotationZ /= length;
		transform.rotationW /= length;
	}
}
} // namespace

namespace engine::animation::detail
{
JointMatrix identityMatrix()
{
	return {{{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}}};
}

JointMatrix multiplyMatrices(const JointMatrix &left, const JointMatrix &right)
{
	JointMatrix product{};
	for (std::size_t column = 0; column < 4; ++column)
	{
		for (std::size_t row = 0; row < 4; ++row)
		{
			for (std::size_t sharedIndex = 0; sharedIndex < 4; ++sharedIndex)
			{
				product.elements[column * 4 + row] +=
				    left.elements[sharedIndex * 4 + row] *
				    right.elements[column * 4 + sharedIndex];
			}
		}
	}
	return product;
}

JointMatrix makeLocalTransformMatrix(LocalTransform localTransform)
{
	// glTF stores column-major matrices. This constructs T * R * S, so the
	// result moves from node-local space into the parent's coordinate space.
	normalizeRotation(localTransform);
	const float rotationXX =
	    localTransform.rotationX * localTransform.rotationX;
	const float rotationYY =
	    localTransform.rotationY * localTransform.rotationY;
	const float rotationZZ =
	    localTransform.rotationZ * localTransform.rotationZ;
	const float rotationXY =
	    localTransform.rotationX * localTransform.rotationY;
	const float rotationXZ =
	    localTransform.rotationX * localTransform.rotationZ;
	const float rotationYZ =
	    localTransform.rotationY * localTransform.rotationZ;
	const float rotationWX =
	    localTransform.rotationW * localTransform.rotationX;
	const float rotationWY =
	    localTransform.rotationW * localTransform.rotationY;
	const float rotationWZ =
	    localTransform.rotationW * localTransform.rotationZ;

	JointMatrix localMatrix = identityMatrix();
	localMatrix.elements[0] =
	    (1 - 2 * (rotationYY + rotationZZ)) * localTransform.scaleX;
	localMatrix.elements[1] =
	    2 * (rotationXY + rotationWZ) * localTransform.scaleX;
	localMatrix.elements[2] =
	    2 * (rotationXZ - rotationWY) * localTransform.scaleX;
	localMatrix.elements[4] =
	    2 * (rotationXY - rotationWZ) * localTransform.scaleY;
	localMatrix.elements[5] =
	    (1 - 2 * (rotationXX + rotationZZ)) * localTransform.scaleY;
	localMatrix.elements[6] =
	    2 * (rotationYZ + rotationWX) * localTransform.scaleY;
	localMatrix.elements[8] =
	    2 * (rotationXZ + rotationWY) * localTransform.scaleZ;
	localMatrix.elements[9] =
	    2 * (rotationYZ - rotationWX) * localTransform.scaleZ;
	localMatrix.elements[10] =
	    (1 - 2 * (rotationXX + rotationYY)) * localTransform.scaleZ;
	localMatrix.elements[12] = localTransform.translationX;
	localMatrix.elements[13] = localTransform.translationY;
	localMatrix.elements[14] = localTransform.translationZ;
	return localMatrix;
}

void interpolateQuaternionShortestArc(
    LocalTransform &destination, const std::array<float, 4> &firstRotation,
    const std::array<float, 4> &secondRotation, float interpolationFactor)
{
	std::array<float, 4> adjustedSecond = secondRotation;
	float dotProduct = firstRotation[0] * adjustedSecond[0] +
	                   firstRotation[1] * adjustedSecond[1] +
	                   firstRotation[2] * adjustedSecond[2] +
	                   firstRotation[3] * adjustedSecond[3];
	// q and -q describe the same rotation. Flipping the second quaternion when
	// needed makes spherical interpolation follow the shorter arc.
	if (dotProduct < 0)
	{
		for (float &component : adjustedSecond)
		{
			component = -component;
		}
		dotProduct = -dotProduct;
	}
	if (dotProduct > .9995F)
	{
		destination.rotationX =
		    firstRotation[0] +
		    interpolationFactor * (adjustedSecond[0] - firstRotation[0]);
		destination.rotationY =
		    firstRotation[1] +
		    interpolationFactor * (adjustedSecond[1] - firstRotation[1]);
		destination.rotationZ =
		    firstRotation[2] +
		    interpolationFactor * (adjustedSecond[2] - firstRotation[2]);
		destination.rotationW =
		    firstRotation[3] +
		    interpolationFactor * (adjustedSecond[3] - firstRotation[3]);
		normalizeRotation(destination);
		return;
	}

	const float angle = std::acos(std::clamp(dotProduct, -1.0F, 1.0F));
	const float angleSine = std::sin(angle);
	const float firstWeight =
	    std::sin((1 - interpolationFactor) * angle) / angleSine;
	const float secondWeight =
	    std::sin(interpolationFactor * angle) / angleSine;
	destination.rotationX =
	    firstRotation[0] * firstWeight + adjustedSecond[0] * secondWeight;
	destination.rotationY =
	    firstRotation[1] * firstWeight + adjustedSecond[1] * secondWeight;
	destination.rotationZ =
	    firstRotation[2] * firstWeight + adjustedSecond[2] * secondWeight;
	destination.rotationW =
	    firstRotation[3] * firstWeight + adjustedSecond[3] * secondWeight;
	normalizeRotation(destination);
}

LocalTransform blendLocalTransforms(const LocalTransform &sourceTransform,
                                    const LocalTransform &targetTransform,
                                    float blendFactor)
{
	// Both inputs are parent-local. Mixing happens before hierarchy
	// composition.
	LocalTransform blended{
	    sourceTransform.translationX +
	        blendFactor *
	            (targetTransform.translationX - sourceTransform.translationX),
	    sourceTransform.translationY +
	        blendFactor *
	            (targetTransform.translationY - sourceTransform.translationY),
	    sourceTransform.translationZ +
	        blendFactor *
	            (targetTransform.translationZ - sourceTransform.translationZ),
	    sourceTransform.rotationX,
	    sourceTransform.rotationY,
	    sourceTransform.rotationZ,
	    sourceTransform.rotationW,
	    sourceTransform.scaleX +
	        blendFactor * (targetTransform.scaleX - sourceTransform.scaleX),
	    sourceTransform.scaleY +
	        blendFactor * (targetTransform.scaleY - sourceTransform.scaleY),
	    sourceTransform.scaleZ +
	        blendFactor * (targetTransform.scaleZ - sourceTransform.scaleZ),
	};
	interpolateQuaternionShortestArc(
	    blended,
	    {sourceTransform.rotationX, sourceTransform.rotationY,
		 sourceTransform.rotationZ, sourceTransform.rotationW},
	    {targetTransform.rotationX, targetTransform.rotationY,
		 targetTransform.rotationZ, targetTransform.rotationW},
	    blendFactor);
	return blended;
}
} // namespace engine::animation::detail
