/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   AssetDiagnostics.hpp                             :+:      :+:    :+: :+:      */
/*                                                    +:+ +:+   +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+ +#+#+#+#+#+   +#+      */
/*                                                +#+#+#+#+#+   +#+           +#+#+#+#+#+   */
/*   Created: 2026/10/08          by flaviobc      #+#    #+#             */
/*                                                                                    */
/* ******************************************************************************** */

#pragma once

#include "animation/SkeletalPose.hpp"
#include "assets/GlbLoader.hpp"

#include <string>

namespace bootstrap
{
bool diagnosePrototypeArena(
    const engine::assets::GlbDocument &document, const std::string &path,
    engine::animation::SkeletalPoseSource &skeletalSource);
} // namespace bootstrap
