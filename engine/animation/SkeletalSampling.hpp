/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   SkeletalSampling.hpp                              :+:        :+:    :+: :+:    : */
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
[[nodiscard]] bool
sampleLocalTransforms(const SkeletalPoseSource &source,
                      const SkeletalPoseClip *clip, float timeSeconds,
                      std::vector<LocalTransform> &localTransforms,
                      std::string &error);
} // namespace engine::animation::detail
