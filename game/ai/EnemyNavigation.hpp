/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   EnemyNavigation.hpp                              :+:      :+:    :+:   :+:      */
/*                                                    +:+ +:+         +:+     +:+    */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           +#+#+#+#+#+   */
/*   Created: 2026/10/07          by flaviobc      #+#    #+#             */
/*   Updated: 2026/10/07          by flaviobc     ###   ########.fr       */
/*                                                                                    */
/* ******************************************************************************** */

#pragma once

#include "actors/ActorState.hpp"
#include "actors/EnemyState.hpp"
#include "combat/Explosion.hpp"

#include <array>
#include <optional>
#include <span>
#include <vector>

namespace game
{
class Arena;

// Non-owning navigation view. Arena cells are (column,row); routes own their
// ordered cells, while all referenced runtime state must outlive this object.
class EnemyNavigation final
{
  public:
	using Route = std::vector<ExplosionCell>;

	EnemyNavigation(const Arena &arena, const EnemyBrain &brain,
	                double ownFuseSeconds,
	                const GameplayTuningMultipliers &weights);

	bool contains(ExplosionCell cell) const;
	bool hasLineOfSight(ExplosionCell source, ExplosionCell target,
	                    int range) const;
	bool isCellDangerous(const EnemyState &enemy, float x, float y,
	                     bool ignoreOwnBomb = false) const;
	bool routeStepClear(const EnemyState &enemy, float fromX, float fromY,
	                    float toX, float toY) const;
	bool terrainOrBombBlocked(const EnemyState &enemy, float x, float y) const;
	bool actorBlocked(const EnemyState &enemy, float x, float y) const;
	bool movementStepBlocked(const EnemyState &enemy, float x, float y,
	                         bool allowPlayerOverlap) const;

	std::optional<Route>
	findEscapeRoute(const EnemyState &enemy, bool placingBomb, float actorSpeed,
	                bool preferAdversarySeparation = false) const;
	std::optional<Route>
	findRouteToAnyCandidate(const EnemyState &enemy,
	                        std::span<const ExplosionCell> candidates) const;
	std::optional<Route> findRouteToCell(const EnemyState &enemy,
	                                     ExplosionCell goal) const;
	std::optional<Route> findApproachRoute(const EnemyState &enemy,
	                                       ExplosionCell target) const;
	std::optional<Route> findWallRoute(const EnemyState &enemy,
	                                   ExplosionCell target,
	                                   ExplosionCell &wall) const;
	std::optional<ExplosionCell>
	selectPatrolNeighbor(const EnemyState &enemy,
	                     std::span<const unsigned> visits) const;

  private:
	std::array<ExplosionCell, 4>
	forwardBiasedDirections(const EnemyState &enemy) const;
	Route reconstructRoute(const std::vector<int> &parent, int start,
	                       int destination) const;

	const Arena &arena_;
	bool overlapsActor(float x, float y, float halfExtent,
	                   bool playerOnly) const;
	const EnemyBrain &brain_;
	double ownFuseSeconds_;
	const GameplayTuningMultipliers &weights_;
};
} // namespace game
