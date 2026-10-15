/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   ApplicationComposition.hpp                       :+:      :+:    :+: :+:      */
/*                                                    +:+ +:+   +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+ +#+#+#+#+#+   +#+      */
/*                                                +#+#+#+#+#+   +#+           +#+#+#+#+#+   */
/*   Created: 2026/10/08          by flaviobc      #+#    #+#             */
/*                                                                                    */
/* ******************************************************************************** */

#pragma once

#include "AnimationPreparation.hpp"
#include "AssetPreparation.hpp"
#include "BootstrapPaths.hpp"
#include "ModelPreparation.hpp"

namespace bootstrap
{
int runGameApplication(const BootstrapPaths &paths, OwnedAssets &assets,
                       PreparedModels &models, PreparedAnimations &animations);
} // namespace bootstrap
