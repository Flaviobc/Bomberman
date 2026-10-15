/* ********************************************************************************
 */
/*                                                                                    */
/*                                                      :::      ::::::::   :::
 */
/*   PickupSystem.cpp                                 :+:      :+:    :+:   :+:
 */
/*                                                    +:+ +:+         +:+ +:+ */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+       +#+ */
/*                                                +#+#+#+#+#+   +#+ +#+#+#+#+#+
 */
/*   Created: 2026/10/07          by flaviobc      #+#    #+#             */
/*   Updated: 2026/10/07          by flaviobc     ###   ########.fr       */
/*                                                                                    */
/* ********************************************************************************
 */

#include "pickups/PickupSystem.hpp"

#include "world/Arena.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <random>
#include <stdexcept>
#include <utility>

namespace game
{
void PickupSystem::reset(const Arena &arena, bool enabled)
{
	const int centerColumn = arena.width() / 2;
	const int centerRow = arena.height() / 2;
	std::array<std::vector<std::array<int, 2>>, 5> candidates;
	for (int row = 1; row < arena.height() - 1; ++row)
	{
		for (int column = 1; column < arena.width() - 1; ++column)
		{
			if (arena.cellType(column, row) != CellType::DestructibleWall)
				continue;
			const std::size_t region =
			    column == centerColumn || row == centerRow
			        ? 4
			        : (row > centerRow ? 2 : 0) +
			              (column > centerColumn ? 1 : 0);
			candidates[region].push_back({column, row});
		}
	}

	std::uint32_t seed = std::random_device{}();
	if (seed_ && seed == *seed_)
		++seed;
	seed_ = seed;
	std::mt19937 generator(seed);
	const auto previousPowerUps = std::move(powerUps_);
	powerUps_.clear();
	std::size_t totalPickupCount = 0;
	for (std::size_t region = 0; region < candidates.size(); ++region)
	{
		const double baselineWalls = region == 4 ? 24.0 : 16.0;
		totalPickupCount += std::max<std::size_t>(
		    1, static_cast<std::size_t>(std::lround(candidates[region].size() *
			                                        5.0 / baselineWalls)));
	}
	powerUps_.reserve(totalPickupCount);
	constexpr std::array<const char *, 5> regionNames{
	    {"A", "B", "C", "D", "cross"}};
	for (std::size_t region = 0; region < candidates.size(); ++region)
	{
		auto &walls = candidates[region];
		const double baselineWalls = region == 4 ? 24.0 : 16.0;
		const std::size_t pickupsPerRegion = std::min(
		    walls.size(),
		    std::max<std::size_t>(1, static_cast<std::size_t>(std::lround(
		                                 walls.size() * 5.0 / baselineWalls))));
		const std::size_t capacityPerRegion =
		    pickupsPerRegion / 2 +
		    (region % 2 == 0 && pickupsPerRegion % 2 != 0 ? 1 : 0);
		if (walls.empty())
		{
			throw std::logic_error("Arena region has no pickup walls");
		}
		std::shuffle(walls.begin(), walls.end(), generator);
		const bool samePositions =
		    !previousPowerUps.empty() &&
		    std::all_of(walls.begin(), walls.begin() + pickupsPerRegion,
			            [&previousPowerUps](const auto &cell)
			            {
			                return std::any_of(previousPowerUps.begin(),
							                   previousPowerUps.end(),
							                   [&cell](const PickupState &item)
							                   {
				                                   return item.column ==
												              cell[0] &&
												          item.row == cell[1];
			                                   });
		                });
		if (samePositions && walls.size() > pickupsPerRegion)
		{
			// Preserve quotas while changing at least one wall in this region.
			std::swap(walls.front(), walls[pickupsPerRegion]);
			std::fprintf(
			    stderr,
			    "Pickup region reroll: name=%s repeated positions replaced.\n",
			    regionNames[region]);
		}
		for (std::size_t index = 0; index < pickupsPerRegion; ++index)
		{
			const PickupEffect effect = index < capacityPerRegion
			                                ? PickupEffect::BombCapacity
			                                : PickupEffect::ExplosionRange;
			powerUps_.push_back({walls[index][0], walls[index][1], effect});
			std::fprintf(stderr,
			             "Pickup assigned: region=%s cell=(%d,%d) effect=%s "
			             "hidden=yes.\n",
			             regionNames[region], walls[index][0], walls[index][1],
			             effect == PickupEffect::BombCapacity
			                 ? "bomb_capacity"
							 : "explosion_range");
		}
		std::fprintf(stderr,
		             "Pickup region: name=%s walls=%zu hidden=%zu capacity=%zu "
		             "range=%zu.\n",
		             regionNames[region], walls.size(), pickupsPerRegion,
		             capacityPerRegion, pickupsPerRegion - capacityPerRegion);
	}
	lifePickup_ = {centerColumn, centerRow, enabled};
	const std::size_t capacityCount = static_cast<std::size_t>(std::count_if(
	    powerUps_.begin(), powerUps_.end(), [](const PickupState &pickup)
	    { return pickup.effect == PickupEffect::BombCapacity; }));
	std::fprintf(stderr,
	             "Pickups reset: seed=0x%08X map_seed=0x%08X hidden=%zu "
	             "capacity=%zu range=%zu life=(%d,%d) visible=%s "
	             "life_collector=all progression_collectors=all "
	             "player_capacity_limit=none enemy_capacity_limit=none "
	             "range_limit=none lives_limit=%d.\n",
	             seed, arena.generationSeed(), powerUps_.size(), capacityCount,
	             powerUps_.size() - capacityCount, centerColumn, centerRow,
	             enabled ? "yes" : "no", maximumLives);
}

bool PickupSystem::revealAt(int column, int row)
{
	for (PickupState &powerUp : powerUps_)
	{
		if (column != powerUp.column || row != powerUp.row ||
		    powerUp.revealed || powerUp.collected)
			continue;
		powerUp.revealed = true;
		std::fprintf(stderr, "Power-up revealed: cell=(%d,%d) effect=%s.\n",
		             powerUp.column, powerUp.row,
		             powerUp.effect == PickupEffect::BombCapacity
		                 ? "bomb_capacity"
						 : "explosion_range");
		return true;
	}
	return false;
}

std::optional<PickupCollection> PickupSystem::collectAt(const Arena &arena,
                                                        ActorState &collector)
{
	const int column = arena.columnAt(collector.x);
	const int row = arena.rowAt(collector.y);
	for (PickupState &powerUp : powerUps_)
	{
		if (!powerUp.revealed || powerUp.collected ||
		    powerUp.column != column || powerUp.row != row)
			continue;
		powerUp.collected = true;
		if (powerUp.effect == PickupEffect::BombCapacity)
		{
			if (collector.maximumActiveBombs !=
			    std::numeric_limits<std::size_t>::max())
				++collector.maximumActiveBombs;
		}
		else
		{
			collector.explosionRange += explosionRangeIncrease;
		}
		return PickupCollection{powerUp.column, powerUp.row, powerUp.effect};
	}
	return std::nullopt;
}

std::optional<LifePickupCollection>
PickupSystem::collectLifeAt(const Arena &arena, ActorState &collector)
{
	if (!lifePickup_.active ||
	    arena.columnAt(collector.x) != lifePickup_.column ||
	    arena.rowAt(collector.y) != lifePickup_.row)
	{
		return std::nullopt;
	}
	lifePickup_.active = false;
	const int previousLives = collector.health.lives;
	collector.health.lives = std::min(collector.health.lives + 1, maximumLives);
	return LifePickupCollection{lifePickup_.column, lifePickup_.row,
	                            previousLives};
}

std::span<const PickupState> PickupSystem::powerUps() const
{
	return powerUps_;
}

const LifePickupState &PickupSystem::lifePickup() const
{
	return lifePickup_;
}
} // namespace game
