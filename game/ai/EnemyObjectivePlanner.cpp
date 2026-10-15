/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   EnemyObjectivePlanner.cpp                        :+:      :+:    :+:   :+:      */
/*                                                    +:+ +:+         +:+     +:+    */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07          by flaviobc      #+#    #+#             */
/*   Updated: 2026/10/07          by flaviobc     ###   ########.fr       */
/*                                                                                    */
/* ******************************************************************************** */

#include "ai/EnemyObjectivePlanner.hpp"

#include "pickups/PickupSystem.hpp"
#include "world/Arena.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace game
{
namespace
{
// Code-owned utility bases; multipliers change preference, never feasibility.
double scaled(double base, double multiplier)
{
	return static_cast<double>(std::min(
	    static_cast<long double>(base) * multiplier,
	    static_cast<long double>(std::numeric_limits<double>::max() / 16.0)));
}
} // namespace

const char *enemyObjectiveName(EnemyObjective objective)
{
	constexpr const char *names[] = {
	    "none",     "attack_player", "attack_enemy",    "life_pickup",
	    "power_up", "destroy_wall",  "approach_player", "approach_enemy",
	    "patrol",   "survive",       "evade_threat",    "defensive_bomb"};
	return names[static_cast<std::size_t>(objective)];
}

const char *enemyObjectiveReasonText(EnemyObjectiveReason reason)
{
	switch (reason)
	{
	case EnemyObjectiveReason::ViableAttack:
		return "safe observed attack";
	case EnemyObjectiveReason::LifeBelowMaximum:
		return "visible heart useful to self";
	case EnemyObjectiveReason::UsefulRevealedPickup:
		return "visible useful pickup";
	case EnemyObjectiveReason::WallBlocksAdversaryRoute:
		return "wall opens observed route";
	case EnemyObjectiveReason::SafeObservedAdversaryPosition:
		return "observed adversary approach";
	case EnemyObjectiveReason::LeastVisitedFreeNeighbor:
		return "exploration";
	case EnemyObjectiveReason::NoFreePatrolCell:
		return "no safe candidate";
	case EnemyObjectiveReason::MandatorySurvival:
		return "mandatory survival before utility";
	case EnemyObjectiveReason::ObservedThreat:
		return "inferred pursuit or trap";
	case EnemyObjectiveReason::DefensivePursuit:
		return "safe defensive bomb against pursuit";
	}
	return "unknown";
}

EnemyObjectivePlanner::EnemyObjectivePlanner(
    const Arena &arena, const EnemyNavigation &navigation, float actorSpeed,
    const GameplayTuningMultipliers &weights)
    : arena_(arena), navigation_(navigation), actorSpeed_(actorSpeed),
      weights_(weights)
{
}

bool EnemyObjectivePlanner::objectiveValid(const EnemyState &enemy) const
{
	const EnemyBrain &brain = enemy.brain;
	switch (brain.objective)
	{
	case EnemyObjective::AttackPlayer:
	case EnemyObjective::ApproachPlayer:
	case EnemyObjective::AttackEnemy:
	case EnemyObjective::ApproachEnemy:
		return std::any_of(
		    brain.actors.begin(), brain.actors.end(),
		    [&](const auto &actor)
		    {
			    const bool player =
			        brain.objective == EnemyObjective::AttackPlayer ||
			        brain.objective == EnemyObjective::ApproachPlayer;
			    return actor.identity ==
				           (player ? 0 : brain.objectiveActor + 1) &&
				       arena_.columnAt(actor.x) == brain.objectiveCell.column &&
				       arena_.rowAt(actor.y) == brain.objectiveCell.row;
		    });
	case EnemyObjective::LifePickup:
		return enemy.actor.health.lives < PickupSystem::maximumLives &&
		       std::any_of(
		           brain.pickups.begin(), brain.pickups.end(),
		           [&](const auto &item)
		           {
			           return item.life &&
					          item.cell.column == brain.objectiveCell.column &&
					          item.cell.row == brain.objectiveCell.row;
		           });
	case EnemyObjective::PowerUp:
		return pickupUseful(enemy, brain.objectiveCell.column,
		                    brain.objectiveCell.row);
	case EnemyObjective::DestroyWall:
		return arena_.cellType(brain.objectiveCell.column,
		                       brain.objectiveCell.row) ==
		       CellType::DestructibleWall;
	case EnemyObjective::Patrol:
	case EnemyObjective::Survive:
	case EnemyObjective::EvadeThreat:
	case EnemyObjective::DefensiveBomb:
		return true;
	case EnemyObjective::None:
		return false;
	}
	return false;
}

std::optional<EnemyObjectiveProposal>
EnemyObjectivePlanner::propose(EnemyState &enemy,
                               std::vector<EnemyUtilityCandidate> &scores) const
{
	scores.clear();
	EnemyBrain &brain = enemy.brain;
	// Hard constraint evaluated before scores: no multiplier can buy an attack
	// while the actor is standing in forecast danger.
	if (navigation_.isCellDangerous(enemy, enemy.actor.x, enemy.actor.y))
	{
		auto route = navigation_.findEscapeRoute(enemy, false, actorSpeed_);
		if (!route)
			return std::nullopt;
		const auto cell = route->back();
		scores.push_back(
		    {EnemyObjective::Survive,
			 scaled(AiUtilityBases::survival, weights_.aiSurvival)});
		return EnemyObjectiveProposal{EnemyObjective::Survive,
		                              cell,
		                              0,
		                              std::move(*route),
		                              EnemyObjectiveReason::MandatorySurvival,
		                              scores.back().score};
	}

	std::optional<EnemyObjectiveProposal> best;
	auto consider =
	    [&](EnemyObjectiveProposal candidate, double base, double weight)
	{
		if (candidate.route.empty())
			return;
		const auto destination = candidate.route.back();
		int exits = 0;
		constexpr ExplosionCell directions[] = {
		    {1, 0}, {-1, 0}, {0, 1}, {0, -1}};
		for (const auto direction : directions)
		{
			const float x =
			    arena_.cellCenterX(destination.column + direction.column);
			const float y = arena_.cellCenterY(destination.row + direction.row);
			if (!navigation_.terrainOrBombBlocked(enemy, x, y) &&
			    !navigation_.isCellDangerous(enemy, x, y))
				++exits;
		}
		const double distanceFactor = 1.0 + 0.08 * (candidate.route.size() - 1);
		const double trapPenalty =
		    scaled(AiUtilityBases::trap, weights_.aiTrapAvoidance) *
		    std::min(3.0, brain.threat) / (exits + 1.0);
		candidate.utility = scaled(base, weight) / distanceFactor -
		                    trapPenalty + brain.randomTieBreak();
		scores.push_back({candidate.objective, candidate.utility});
		if (!best || candidate.utility > best->utility)
			best = std::move(candidate);
	};

	for (const auto &target : brain.actors)
	{
		const ExplosionCell cell{arena_.columnAt(target.x),
		                         arena_.rowAt(target.y)};
		const bool player = target.identity == 0;
		const std::size_t slot = player ? 0 : target.identity - 1;
		if (navigation_.hasLineOfSight(actorCell(enemy.actor), cell,
		                               enemy.actor.explosionRange))
			if (auto route = findSafePlacementEscape(enemy))
				consider({player ? EnemyObjective::AttackPlayer
				                 : EnemyObjective::AttackEnemy,
				          cell, slot, std::move(*route),
				          EnemyObjectiveReason::ViableAttack},
				         AiUtilityBases::attack, weights_.aiAggression);
		if (auto route = navigation_.findApproachRoute(enemy, cell))
			consider({player ? EnemyObjective::ApproachPlayer
			                 : EnemyObjective::ApproachEnemy,
			          cell, slot, std::move(*route),
			          EnemyObjectiveReason::SafeObservedAdversaryPosition},
			         AiUtilityBases::approach, weights_.aiAggression);
		ExplosionCell wall{};
		if (auto route = navigation_.findWallRoute(enemy, cell, wall))
			consider({EnemyObjective::DestroyWall, wall, slot,
			          std::move(*route),
			          EnemyObjectiveReason::WallBlocksAdversaryRoute},
			         AiUtilityBases::wall, weights_.aiWallDestruction);
	}
	for (const auto &item : brain.pickups)
	{
		const bool useful =
		    item.life ? enemy.actor.health.lives < PickupSystem::maximumLives
			          : pickupUseful(enemy, item.cell.column, item.cell.row);
		if (!useful)
			continue;
		if (auto route = navigation_.findRouteToCell(enemy, item.cell))
			consider({item.life ? EnemyObjective::LifePickup
			                    : EnemyObjective::PowerUp,
			          item.cell, 0, std::move(*route),
			          item.life ? EnemyObjectiveReason::LifeBelowMaximum
			                    : EnemyObjectiveReason::UsefulRevealedPickup},
			         item.life
			             ? AiUtilityBases::life * (PickupSystem::maximumLives -
						                           enemy.actor.health.lives)
						 : AiUtilityBases::pickup,
			         weights_.aiPickup);
	}
	// Inferred threat favours a reachable destination farther from observed
	// opponents with exits. No target intention or future player input is read.
	if (brain.threat > 0.75)
	{
		const auto start = actorCell(enemy.actor);
		if (auto route = findSafePlacementEscape(enemy, true))
		{
			const ExplosionCell destination = route->back();
			double startSeparation = arena_.width() + arena_.height();
			double destinationSeparation = startSeparation;
			for (const ObservedActor &actor : brain.actors)
			{
				const int actorColumn = arena_.columnAt(actor.x);
				const int actorRow = arena_.rowAt(actor.y);
				startSeparation = std::min(
				    startSeparation,
				    static_cast<double>(std::abs(start.column - actorColumn) +
					                    std::abs(start.row - actorRow)));
				destinationSeparation =
				    std::min(destinationSeparation,
					         static_cast<double>(
				                 std::abs(destination.column - actorColumn) +
				                 std::abs(destination.row - actorRow)));
			}
			if (destinationSeparation > startSeparation)
				consider({EnemyObjective::DefensiveBomb, start, 0,
				          std::move(*route),
				          EnemyObjectiveReason::DefensivePursuit},
				         AiUtilityBases::attack + AiUtilityBases::threat,
				         weights_.aiThreatResponse);
		}
		constexpr ExplosionCell directions[] = {
		    {1, 0}, {-1, 0}, {0, 1}, {0, -1}};
		auto separation = [&](ExplosionCell cell)
		{
			double nearest = arena_.width() + arena_.height();
			for (const auto &actor : brain.actors)
				nearest = std::min(
				    nearest,
				    static_cast<double>(
				        std::abs(cell.column - arena_.columnAt(actor.x)) +
				        std::abs(cell.row - arena_.rowAt(actor.y))));
			return nearest;
		};
		for (auto direction : directions)
		{
			ExplosionCell cell{start.column + direction.column,
			                   start.row + direction.row};
			if (separation(cell) <= separation(start))
				continue;
			if (auto route = navigation_.findRouteToCell(enemy, cell))
				consider({EnemyObjective::EvadeThreat, cell, 0,
				          std::move(*route),
				          EnemyObjectiveReason::ObservedThreat},
				         AiUtilityBases::threat * std::min(1.5, brain.threat) +
				             scaled(10.0, weights_.aiSurvival),
				         weights_.aiThreatResponse);
		}
	}
	if (auto patrol = proposePatrol(enemy))
		consider(std::move(*patrol), AiUtilityBases::exploration,
		         weights_.aiExploration);
	return best;
}

std::optional<EnemyObjectiveProposal>
EnemyObjectivePlanner::proposePatrol(const EnemyState &enemy) const
{
	const auto start = actorCell(enemy.actor);
	EnemyNavigation::Route route;
	if (std::abs(arena_.cellCenterX(start.column) - enemy.actor.x) > 0.00001F ||
	    std::abs(arena_.cellCenterY(start.row) - enemy.actor.y) > 0.00001F)
	{
		if (!navigation_.routeStepClear(enemy, enemy.actor.x, enemy.actor.y,
		                                arena_.cellCenterX(start.column),
		                                arena_.cellCenterY(start.row)))
			return std::nullopt;
		route.push_back(start);
	}
	else if (auto selected = navigation_.selectPatrolNeighbor(
	             enemy, enemy.brain.patrolVisits))
	{
		if (!navigation_.routeStepClear(enemy, enemy.actor.x, enemy.actor.y,
		                                arena_.cellCenterX(selected->column),
		                                arena_.cellCenterY(selected->row)))
			return std::nullopt;
		route.push_back(*selected);
	}
	if (route.empty())
		return std::nullopt;
	const auto target = route.back();
	return EnemyObjectiveProposal{
	    EnemyObjective::Patrol, target, 0, std::move(route),
	    EnemyObjectiveReason::LeastVisitedFreeNeighbor};
}

std::optional<EnemyNavigation::Route>
EnemyObjectivePlanner::findSafePlacementEscape(
    const EnemyState &enemy, bool preferAdversarySeparation) const
{
	if (enemy.brain.ownBombCount() >= enemy.actor.maximumActiveBombs)
		return std::nullopt;
	const auto cell = actorCell(enemy.actor);
	if (enemy.brain.hasBombAt(arena_, cell.column, cell.row))
		return std::nullopt;
	return navigation_.findEscapeRoute(enemy, true, actorSpeed_,
	                                   preferAdversarySeparation);
}

bool EnemyObjectivePlanner::pickupUseful(const EnemyState &enemy, int column,
                                         int row) const
{
	return std::any_of(enemy.brain.pickups.begin(), enemy.brain.pickups.end(),
	                   [&](const auto &item)
	                   {
		                   return !item.life && item.cell.column == column &&
						          item.cell.row == row;
	                   });
}

ExplosionCell EnemyObjectivePlanner::actorCell(const ActorState &actor) const
{
	return {arena_.columnAt(actor.x), arena_.rowAt(actor.y)};
}
} // namespace game
