/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   SkeletalComposition.hpp                           :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

#include "SkeletalMath.hpp"

#include <string>
#include <vector>

namespace engine::animation::detail
{
[[nodiscard]] bool validateSkeletalPoseSource(const SkeletalPoseSource &source,
                                              std::string &error);

[[nodiscard]] SkeletalPoseResult
composeJointPalette(const SkeletalPoseSource &source,
                    const std::vector<LocalTransform> &localTransforms);
} // namespace engine::animation::detail
