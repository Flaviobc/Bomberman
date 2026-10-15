/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   EnemyObjectivePlanner.hpp                        :+:      :+:    :+:   :+:      */
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

#include <cstddef>
#include <optional>
#include <span>

namespace game
{
class Arena;

enum class EnemyObjectiveReason
{
	ViableAttack,
	LifeBelowMaximum,
	UsefulRevealedPickup,
	WallBlocksAdversaryRoute,
	SafeObservedAdversaryPosition,
	LeastVisitedFreeNeighbor,
	NoFreePatrolCell,
	MandatorySurvival,
	ObservedThreat,
	DefensivePursuit,
};

// A proposal owns its route but never changes actors. EnemyController decides
// whether and when to apply it, request a bomb, transition state, and emit
// diagnostics.
struct EnemyObjectiveProposal
{
	EnemyObjective objective = EnemyObjective::None;
	ExplosionCell cell{};
	std::size_t actorIndex = 0;
	EnemyNavigation::Route route;
	EnemyObjectiveReason reason = EnemyObjectiveReason::NoFreePatrolCell;
	double utility = 0.0;
};

struct EnemyUtilityCandidate
{
	EnemyObjective objective;
	double score;
};

const char *enemyObjectiveReasonText(EnemyObjectiveReason reason);
const char *enemyObjectiveName(EnemyObjective objective);

// Stateless decision view of one brain's observations. Feasibility precedes
// utility. Only the requesting brain advances its RNG to break close ties.
class EnemyObjectivePlanner final
{
  public:
	EnemyObjectivePlanner(const Arena &arena, const EnemyNavigation &navigation,
	                      float actorSpeed,
	                      const GameplayTuningMultipliers &weights);

	bool objectiveValid(const EnemyState &enemy) const;
	std::optional<EnemyObjectiveProposal>
	propose(EnemyState &enemy,
	        std::vector<EnemyUtilityCandidate> &scores) const;
	std::optional<EnemyObjectiveProposal>
	proposePatrol(const EnemyState &enemy) const;

  private:
	std::optional<EnemyNavigation::Route>
	findSafePlacementEscape(const EnemyState &enemy,
	                        bool preferAdversarySeparation = false) const;
	bool pickupUseful(const EnemyState &enemy, int column, int row) const;
	ExplosionCell actorCell(const ActorState &actor) const;

	const Arena &arena_;
	const EnemyNavigation &navigation_;
	float actorSpeed_;
	const GameplayTuningMultipliers &weights_;
};
} // namespace game
