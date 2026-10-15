/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   ApplicationComposition.cpp                       :+:      :+:    :+: :+:      */
/*                                                    +:+ +:+   +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+ +#+#+#+#+#+   +#+      */
/*                                                +#+#+#+#+#+   +#+           +#+#+#+#+#+   */
/*   Created: 2026/10/08          by flaviobc      #+#    #+#             */
/*                                                                                    */
/* ******************************************************************************** */

#include "ApplicationComposition.hpp"

#include "GameSession.hpp"
#include "config/GameplayTuning.hpp"
#include "core/Application.hpp"

#include <cstddef>
#include <cstdio>
#include <string>
#include <utility>

namespace
{
constexpr std::size_t kBombPlacementAudioClipSlot = 0;
constexpr std::size_t kBombDetonationAudioClipSlot = 1;
constexpr std::size_t kPickupAudioClipSlot = 2;
constexpr std::size_t kDamageAudioClipSlot = 3;
constexpr std::size_t kDefeatAudioClipSlot = 4;
constexpr std::size_t kVictoryAudioClipSlot = 5;
} // namespace

int bootstrap::runGameApplication(const BootstrapPaths &paths,
                                  OwnedAssets &assets, PreparedModels &models,
                                  PreparedAnimations &animations)
{
	std::string tuningError;
	const std::optional<game::GameplayTuningLoadResult> tuning =
	    game::loadGameplayTuning(paths.gameplayTuning, tuningError);
	if (!tuning)
	{
		std::fprintf(stderr, "Gameplay tuning error: %s\n",
		             tuningError.c_str());
		return 1;
	}
	game::logGameplayTuning(*tuning, paths.gameplayTuning);
	const ModelIds &ids = models.ids;
	game::GameSession session(
	    tuning->tuning, ids.indestructibleWall, ids.labyrinthPillar,
	    ids.destructibleWall, ids.bomb, ids.floor, ids.powerUpBombCapacity,
	    ids.powerUpAura, ids.powerUpBlastRange, ids.powerUpLife, ids.enemyBasic,
	    models.bounds,
	    {kBombPlacementAudioClipSlot, kBombDetonationAudioClipSlot,
		 kPickupAudioClipSlot, kDamageAudioClipSlot, kDefeatAudioClipSlot,
		 kVictoryAudioClipSlot});
	// The synchronous application run ends before assets is destroyed. This
	// keeps every mesh and decoded-image pointer borrowed by models.inputs
	// valid.
	return engine::runApplication(
	    session, models.inputs, animations.transformClips,
	    std::move(animations.skeletalPose), assets.hudIconAtlas,
	    assets.hudFontAtlas,
	    {assets.explosionSmokeImage, assets.explosionFireballImage},
	    paths.audioClips);
}
