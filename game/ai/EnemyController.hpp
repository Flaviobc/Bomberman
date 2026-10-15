/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   EnemyController.hpp                              :+:      :+:    :+:   :+:      */
/*                                                    +:+ +:+         +:+     +:+    */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07          by flaviobc      #+#    #+#             */
/*   Updated: 2026/10/07          by flaviobc     ###   ########.fr       */
/*                                                                                    */
/* ******************************************************************************** */

#pragma once

#include "actors/ActorState.hpp"
#include "actors/EnemyState.hpp"
#include "ai/EnemyNavigation.hpp"
#include "ai/EnemyObjectivePlanner.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace game
{
class Arena;
class BombSystem;
class PickupSystem;

// Synchronous effects keep collection/audio and actual bomb creation in the
// session. Physics resolves effects immediately, but other brains observe them
// only in their next pre-action snapshot.
class EnemyControllerEffects
{
  public:
	virtual ~EnemyControllerEffects() = default;
	virtual void collectEnemyPickups(std::size_t enemyIndex,
	                                 ActorState &actor) = 0;
	virtual std::optional<std::size_t> placeEnemyBomb(std::size_t enemyIndex,
	                                                  ActorState &actor) = 0;
};

// Per-update borrowed view that executes plans. Each EnemyState owns its brain;
// optional navigation/planner views live only during this synchronous update.
class EnemyController final
{
  public:
	EnemyController(const Arena &arena, BombSystem &bombs,
	                const PickupSystem &pickups, const ActorState &player,
	                std::span<EnemyState> enemies,
	                EnemyControllerEffects &effects, float actorSpeed,
	                GameplayTuningMultipliers weights = {});

	void update(double deltaSeconds, bool pickupsEnabled);

  private:
	bool objectiveValid(const EnemyState &enemy) const;
	bool applyObjectiveProposal(std::size_t index,
	                            EnemyObjectiveProposal proposal);
	void setObjective(std::size_t index, EnemyObjective objective,
	                  ExplosionCell cell, std::size_t actor,
	                  std::vector<ExplosionCell> route, const char *reason);
	void clearObjective(std::size_t index, const char *reason);
	bool chooseObjective(std::size_t index);
	void transition(std::size_t index, EnemyActionState state,
	                const char *reason);
	bool tryPlaceBomb(std::size_t index, EnemyState &enemy, const char *reason);
	bool commitBomb(std::size_t index, EnemyState &enemy, const char *reason,
	                std::vector<ExplosionCell> escapeRoute);
	void movePlan(std::size_t index, double deltaSeconds);
	void updateEnemy(std::size_t index, double deltaSeconds,
	                 bool pickupsEnabled);
	bool destructibleWallBlocksAdvance(const EnemyState &enemy) const;

	const Arena &arena_;
	BombSystem &bombs_;
	const PickupSystem &pickups_;
	const ActorState &player_;
	std::span<EnemyState> enemies_;
	EnemyControllerEffects &effects_;
	float actorSpeed_;
	GameplayTuningMultipliers weights_;
	std::optional<EnemyNavigation> navigation_;
	std::optional<EnemyObjectivePlanner> planner_;
};
} // namespace game
