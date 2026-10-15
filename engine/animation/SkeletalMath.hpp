/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   SkeletalMath.hpp                                  :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

#include "SkeletalPose.hpp"

#include <array>

namespace engine::animation::detail
{
// Translation, rotation and scale of one node relative to its parent.
struct LocalTransform
{
	float translationX;
	float translationY;
	float translationZ;
	float rotationX;
	float rotationY;
	float rotationZ;
	float rotationW;
	float scaleX;
	float scaleY;
	float scaleZ;
};

[[nodiscard]] JointMatrix identityMatrix();
[[nodiscard]] JointMatrix multiplyMatrices(const JointMatrix &left,
                                           const JointMatrix &right);
[[nodiscard]] JointMatrix
makeLocalTransformMatrix(LocalTransform localTransform);

void interpolateQuaternionShortestArc(
    LocalTransform &destination, const std::array<float, 4> &firstRotation,
    const std::array<float, 4> &secondRotation, float interpolationFactor);

[[nodiscard]] LocalTransform
blendLocalTransforms(const LocalTransform &sourceTransform,
                     const LocalTransform &targetTransform, float blendFactor);
} // namespace engine::animation::detail
