/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   EnemyRoster.cpp                                  :+:      :+:    :+: :+:      */
/*                                                    +:+ +:+   +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+ +#+#+#+#+#+   +#+      */
/*                                                +#+#+#+#+#+   +#+           +#+#+#+#+#+   */
/*   Created: 2026/10/09          by flaviobc      #+#    #+#             */
/*   Updated: 2026/10/09          by flaviobc     ###   ########.fr       */
/*                                                                                    */
/* ******************************************************************************** */
/*                                                                                    */
/* **********************************************************************************
 */

#include "actors/EnemyRoster.hpp"

#include "pickups/PickupSystem.hpp"
#include "world/Arena.hpp"

#include <algorithm>
#include <array>
#include <cstdio>

namespace
{
struct EnemySpawn
{
	int column;
	int row;
	int direction;
};

constexpr std::size_t kInitialMaximumActiveBombs = 1;
constexpr int kInitialExplosionRange = 1;
constexpr int kInitialLives = 1;
} // namespace

namespace game
{
EnemyRoster::EnemyRoster(
    double damageInvulnerabilitySeconds,
    std::array<EnemyPersonalityProfile, slotCount> personalities)
    : personalities_(personalities),
      damageInvulnerabilitySeconds_(damageInvulnerabilitySeconds)
{
}

void EnemyRoster::configureCollisionHalfExtent(float collisionHalfExtent)
{
	for (EnemyState &enemy : enemies_)
		enemy.actor.collisionHalfExtent = collisionHalfExtent;
}

void EnemyRoster::reset(const Arena &arena, bool enabled)
{
	const std::array<EnemySpawn, slotCount> enemySpawns{{
	    {1, 1, 1},
	    {arena.width() - 2, 1, -1},
	    {arena.width() - 2, arena.height() - 2, -1},
	}};
	for (std::size_t index = 0; index < enemies_.size(); ++index)
	{
		EnemyState &enemy = enemies_[index];
		enemy.brain.reset(
		    arena.generationSeed() ^
		    (0x9E3779B9U * static_cast<std::uint32_t>(index + 1)));
		enemy.brain.personality = personalities_[index];
		const EnemySpawn &spawn = enemySpawns[index];
		enemy.actor.x = arena.cellCenterX(spawn.column);
		enemy.actor.y = arena.cellCenterY(spawn.row);
		enemy.actor.maximumActiveBombs = kInitialMaximumActiveBombs;
		enemy.actor.explosionRange = kInitialExplosionRange;
		enemy.actor.bombPassThrough.clear();
		enemy.actor.health.lives = kInitialLives;
		enemy.actor.health.invulnerabilityRemainingSeconds = 0.0;
		enemy.direction = {spawn.direction, 0};
		enemy.brain.patrolVisits.assign(
		    static_cast<std::size_t>(arena.width() * arena.height()), 0);
		enemy.brain.patrolVisits[spawn.row * arena.width() + spawn.column] = 1;
		enemy.active = enabled;
		std::fprintf(stderr,
		             "Enemy spawn: index=%zu profile_id=%u cell=(%d,%d) "
		             "navigation=global lives=%d/%d direction=%s.\n",
		             index, static_cast<unsigned>(enemy.brain.personality),
		             spawn.column, spawn.row, enemy.actor.health.lives,
		             PickupSystem::maximumLives,
		             spawn.direction > 0 ? "right" : "left");
	}
}

void EnemyRoster::advanceInvulnerability(double deltaSeconds)
{
	for (EnemyState &enemy : enemies_)
	{
		enemy.actor.health.invulnerabilityRemainingSeconds =
		    std::max(0.0, enemy.actor.health.invulnerabilityRemainingSeconds -
			                  deltaSeconds);
	}
}

void EnemyRoster::applyDamage(std::size_t index)
{
	if (index >= enemies_.size())
		return;
	EnemyState &enemy = enemies_[index];
	if (!enemy.active ||
	    enemy.actor.health.invulnerabilityRemainingSeconds > 0.0)
		return;
	--enemy.actor.health.lives;
	std::fprintf(
	    stderr, "Enemy damaged: index=%zu lives=%d/%d position=(%.3f, %.3f).\n",
	    index, enemy.actor.health.lives, PickupSystem::maximumLives,
	    enemy.actor.x, enemy.actor.y);
	if (enemy.actor.health.lives == 0)
	{
		enemy.active = false;
		enemy.brain.route.clear();
		std::fprintf(
		    stderr,
		    "Enemy defeated by explosion: index=%zu position=(%.3f, %.3f).\n",
		    index, enemy.actor.x, enemy.actor.y);
	}
	else
	{
		enemy.actor.health.invulnerabilityRemainingSeconds =
		    damageInvulnerabilitySeconds_;
		std::fprintf(stderr,
		             "Enemy damage invulnerability: index=%zu seconds=%.2f.\n",
		             index, enemy.actor.health.invulnerabilityRemainingSeconds);
	}
}

bool EnemyRoster::hasActiveEnemy() const
{
	return std::any_of(enemies_.begin(), enemies_.end(),
	                   [](const EnemyState &enemy) { return enemy.active; });
}

std::size_t EnemyRoster::activeEnemyCount() const
{
	return static_cast<std::size_t>(
	    std::count_if(enemies_.begin(), enemies_.end(),
		              [](const EnemyState &enemy) { return enemy.active; }));
}

bool EnemyRoster::isActive(std::size_t index) const
{
	return index < enemies_.size() && enemies_[index].active;
}

std::span<EnemyState> EnemyRoster::view()
{
	return enemies_;
}

std::span<const EnemyState> EnemyRoster::view() const
{
	return enemies_;
}
} // namespace game
