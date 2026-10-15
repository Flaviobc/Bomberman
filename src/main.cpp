/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   main.cpp                                          :+:      :+:    :+: :+:      */
/*                                                    +:+ +:+   +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+ +#+#+#+#+#+   +#+      */
/*                                                +#+#+#+#+#+   +#+           +#+#+#+#+#+   */
/*   Created: 2026/09/25          by flaviobc      #+#    #+#             */
/*   Updated: 2026/10/03          by flaviobc     ###   ########.fr       */
/*                                                                                    */
/* ******************************************************************************** */
/*                                                                                    */
/* **********************************************************************************
 */

#include "bootstrap/AnimationPreparation.hpp"
#include "bootstrap/ApplicationComposition.hpp"
#include "bootstrap/AssetDiagnostics.hpp"
#include "bootstrap/AssetPreparation.hpp"
#include "bootstrap/BootstrapPaths.hpp"
#include "bootstrap/ModelPreparation.hpp"

#include <SDL3/SDL_version.h>

#include <cstdio>

int main()
{
	if (SDL_GetVersion() != SDL_VERSION)
	{
		std::fputs("SDL version mismatch.\n", stderr);
		return 1;
	}

	const bootstrap::BootstrapPaths paths = bootstrap::resolveBootstrapPaths();
	// This owner is never moved after loading because prepared mesh inputs
	// borrow storage from its GLB document and decoded images.
	bootstrap::OwnedAssets assets;
	if (!bootstrap::loadOwnedAssets(paths, assets))
	{
		return 1;
	}

	bootstrap::PreparedAnimations animations;
	if (!bootstrap::diagnosePrototypeArena(assets.document, paths.glb,
	                                       animations.skeletalPose) ||
	    !bootstrap::prepareTransformAnimations(assets.document,
	                                           animations.transformClips))
	{
		return 1;
	}

	bootstrap::PreparedModels models;
	if (!bootstrap::prepareModels(assets.document, models))
	{
		return 1;
	}
	return bootstrap::runGameApplication(paths, assets, models, animations);
}
