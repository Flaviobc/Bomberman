/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   EnemyController.cpp                              :+:      :+:    :+:   :+:      */
/*                                                    +:+ +:+         +:+     +:+    */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07          by flaviobc      #+#    #+#             */
/*   Updated: 2026/10/07          by flaviobc     ###   ########.fr       */
/*                                                                                    */
/* ******************************************************************************** */

#include "ai/EnemyController.hpp"

#include "collision/ActorCollision.hpp"
#include "combat/BombSystem.hpp"
#include "pickups/PickupSystem.hpp"
#include "world/Arena.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <utility>

namespace game
{
EnemyController::EnemyController(const Arena &arena, BombSystem &bombs,
                                 const PickupSystem &pickups,
                                 const ActorState &player,
                                 std::span<EnemyState> enemies,
                                 EnemyControllerEffects &effects,
                                 float actorSpeed,
                                 GameplayTuningMultipliers weights)
    : arena_(arena), bombs_(bombs), pickups_(pickups), player_(player),
      enemies_(enemies), effects_(effects), actorSpeed_(actorSpeed),
      weights_(weights)
{
}

void EnemyController::update(double deltaSeconds, bool pickupsEnabled)
{
	// Freeze every perception before effects. Later slots cannot observe a
	// bomb or pickup produced by an earlier slot until the following frame.
	for (std::size_t index = 0; index < enemies_.size(); ++index)
		if (enemies_[index].active)
			enemies_[index].brain.capture(arena_, bombs_, pickups_, player_,
			                              enemies_, index, deltaSeconds,
			                              pickupsEnabled);
	for (std::size_t index = 0; index < enemies_.size(); ++index)
		if (enemies_[index].active)
		{
			const GameplayTuningMultipliers profileWeights =
			    applyEnemyPersonalityProfile(weights_,
				                             enemies_[index].brain.personality);
			navigation_.emplace(arena_, enemies_[index].brain,
			                    bombs_.fuseDurationSeconds(), profileWeights);
			planner_.emplace(arena_, *navigation_, actorSpeed_, profileWeights);
			updateEnemy(index, deltaSeconds, pickupsEnabled);
		}
}

bool EnemyController::objectiveValid(const EnemyState &enemy) const
{
	return planner_->objectiveValid(enemy);
}

bool EnemyController::applyObjectiveProposal(std::size_t index,
                                             EnemyObjectiveProposal proposal)
{
	EnemyState &enemy = enemies_[index];
	if (proposal.objective == EnemyObjective::AttackPlayer ||
	    proposal.objective == EnemyObjective::AttackEnemy ||
	    proposal.objective == EnemyObjective::DefensiveBomb)
	{
		const char *placementReason = "defensive_pursuit_block";
		if (proposal.objective == EnemyObjective::AttackPlayer)
			placementReason = "player_line_of_sight";
		else if (proposal.objective == EnemyObjective::AttackEnemy)
			placementReason = "enemy_line_of_sight";
		if (!commitBomb(index, enemy, placementReason,
		                std::move(proposal.route)))
			return false;
		proposal.route = std::move(enemy.brain.route);
	}
	else if (proposal.objective == EnemyObjective::Patrol &&
	         !proposal.route.empty())
	{
		const ExplosionCell current{arena_.columnAt(enemy.actor.x),
		                            arena_.rowAt(enemy.actor.y)};
		if (proposal.cell.column != current.column ||
		    proposal.cell.row != current.row)
		{
			enemy.direction = {proposal.cell.column - current.column,
			                   proposal.cell.row - current.row};
		}
	}

	setObjective(index, proposal.objective, proposal.cell, proposal.actorIndex,
	             std::move(proposal.route),
	             enemyObjectiveReasonText(proposal.reason));
	switch (proposal.objective)
	{
	case EnemyObjective::AttackPlayer:
	case EnemyObjective::AttackEnemy:
	case EnemyObjective::DefensiveBomb:
		return true;
	case EnemyObjective::LifePickup:
		transition(index, EnemyActionState::Patrol, "life pickup selected");
		return true;
	case EnemyObjective::PowerUp:
		transition(index, EnemyActionState::Patrol, "power-up selected");
		return true;
	case EnemyObjective::DestroyWall:
		transition(index, EnemyActionState::Patrol, "blocking wall selected");
		return true;
	case EnemyObjective::ApproachPlayer:
	case EnemyObjective::ApproachEnemy:
		transition(index, EnemyActionState::Patrol,
		           "adversary approach selected");
		return true;
	case EnemyObjective::Patrol:
		transition(index, EnemyActionState::Patrol,
		           enemy.brain.route.empty() ? "no free patrol cell; holding"
		                                     : "patrol target selected");
		return !enemy.brain.route.empty();
	case EnemyObjective::None:
		return false;
	case EnemyObjective::Survive:
		transition(index, EnemyActionState::EscapeToSafeCell,
		           "mandatory survival");
		return true;
	case EnemyObjective::EvadeThreat:
		transition(index, EnemyActionState::Patrol, "observed threat response");
		return true;
	}
	return false;
}

void EnemyController::setObjective(std::size_t index, EnemyObjective objective,
                                   ExplosionCell cell, std::size_t actor,
                                   std::vector<ExplosionCell> route,
                                   const char *reason)
{
	EnemyState &enemy = enemies_[index];
	constexpr const char *names[] = {
	    "none",     "attack_player", "attack_enemy",    "life_pickup",
	    "power_up", "destroy_wall",  "approach_player", "approach_enemy",
	    "patrol",   "survive",       "evade_threat",    "defensive_bomb"};
	enemy.brain.objective = objective;
	enemy.brain.objectiveCell = cell;
	enemy.brain.objectiveActor = actor;
	enemy.brain.route = std::move(route);
	enemy.brain.routeStep = 0;
	std::fprintf(
	    stderr,
	    "Enemy objective selected: index=%zu type=%s cell=(%d,%d) "
	    "actor=%zu steps=%zu reason=%s active_bombs=%zu capacity=%zu.\n",
	    index, names[static_cast<std::size_t>(objective)], cell.column,
	    cell.row, actor,
	    enemy.brain.route.empty() ? 0 : enemy.brain.route.size() - 1, reason,
	    enemy.brain.ownBombCount(), enemy.actor.maximumActiveBombs);
}

void EnemyController::clearObjective(std::size_t index, const char *reason)
{
	EnemyState &enemy = enemies_[index];
	if (enemy.brain.objective == EnemyObjective::None)
		return;
	std::fprintf(stderr,
	             "Enemy objective ended: index=%zu cell=(%d,%d) reason=%s.\n",
	             index, enemy.brain.objectiveCell.column,
	             enemy.brain.objectiveCell.row, reason);
	enemy.brain.objective = EnemyObjective::None;
	enemy.brain.objectiveCell = {};
	enemy.brain.objectiveActor = 0;
}

bool EnemyController::chooseObjective(std::size_t index)
{
	EnemyState &enemy = enemies_[index];
	enemy.brain.threatAtDecision = enemy.brain.threat;
	enemy.brain.retrySeconds = 0.35;
	// A reached wall objective stays committed until the real placement window
	// opens; destruction is never anticipated by the tactical plan.
	if (enemy.brain.objective == EnemyObjective::DestroyWall &&
	    objectiveValid(enemy))
	{
		const int column = arena_.columnAt(enemy.actor.x);
		const int row = arena_.rowAt(enemy.actor.y);
		const int deltaColumn = enemy.brain.objectiveCell.column - column;
		const int deltaRow = enemy.brain.objectiveCell.row - row;
		if (std::abs(deltaColumn) + std::abs(deltaRow) == 1)
		{
			enemy.direction = {deltaColumn, deltaRow};
			if (enemy.brain.ownBombCount() >= enemy.actor.maximumActiveBombs)
			{
				enemy.brain.waitingForPlacementWindow = true;
				transition(index, EnemyActionState::WaitForExplosion,
				           "wall objective waits for bomb capacity");
				return true;
			}
			if (tryPlaceBomb(index, enemy, "blocking_destructible_wall"))
				return true;
			clearObjective(index, "wall attack has no safe escape");
			if (auto patrol = planner_->proposePatrol(enemy))
				return applyObjectiveProposal(index, std::move(*patrol));
			return false;
		}
	}
	if (enemy.brain.objective != EnemyObjective::None)
		clearObjective(index, objectiveValid(enemy)
		                          ? "objective position reached"
								  : "objective invalidated");
	std::vector<EnemyUtilityCandidate> candidates;
	if (auto proposal = planner_->propose(enemy, candidates))
	{
		std::fprintf(stderr,
		             "Enemy utility: index=%zu threat=%.3f candidates=", index,
		             enemy.brain.threat);
		for (const auto &candidate : candidates)
			std::fprintf(stderr, " %s:%.2f",
			             enemyObjectiveName(candidate.objective),
			             candidate.score);
		std::fprintf(stderr, " winner=%s score=%.2f reason=%s.\n",
		             enemyObjectiveName(proposal->objective), proposal->utility,
		             enemyObjectiveReasonText(proposal->reason));
		return applyObjectiveProposal(index, std::move(*proposal));
	}
	transition(index, EnemyActionState::Patrol,
	           "no safe utility candidate; holding");
	return false;
}

void EnemyController::transition(std::size_t index, EnemyActionState state,
                                 const char *reason)
{
	EnemyState &enemy = enemies_[index];
	if (enemy.brain.action == state)
		return;
	constexpr const char *names[] = {"Patrol",           "ChooseAction",
	                                 "PlaceBomb",        "EscapeToSafeCell",
	                                 "WaitForExplosion", "InspectResult"};
	std::fprintf(stderr, "Enemy state: index=%zu %s -> %s reason=%s.\n", index,
	             names[static_cast<std::size_t>(enemy.brain.action)],
	             names[static_cast<std::size_t>(state)], reason);
	enemy.brain.action = state;
}

bool EnemyController::tryPlaceBomb(std::size_t index, EnemyState &enemy,
                                   const char *reason)
{
	if (navigation_->isCellDangerous(enemy, enemy.actor.x, enemy.actor.y))
		return false;
	if (enemy.brain.ownBombCount() >= enemy.actor.maximumActiveBombs)
		return false;
	auto route = navigation_->findEscapeRoute(enemy, true, actorSpeed_);
	if (!route)
	{
		if (!enemy.brain.safetyRefusalLogged)
		{
			std::fprintf(
			    stderr,
			    "Enemy bomb refused: index=%zu reason=%s no safe escape.\n",
			    index, reason);
			enemy.brain.safetyRefusalLogged = true;
		}
		return false;
	}
	return commitBomb(index, enemy, reason, std::move(*route));
}

bool EnemyController::commitBomb(std::size_t index, EnemyState &enemy,
                                 const char *reason,
                                 std::vector<ExplosionCell> escapeRoute)
{
	transition(index, EnemyActionState::PlaceBomb, reason);
	const std::optional<std::size_t> bombId =
	    effects_.placeEnemyBomb(index, enemy.actor);
	if (!bombId)
	{
		transition(index, EnemyActionState::ChooseAction, "placement rejected");
		return false;
	}
	enemy.brain.rememberOwnBomb(
	    *bombId, arena_.cellCenterX(arena_.columnAt(enemy.actor.x)),
	    arena_.cellCenterY(arena_.rowAt(enemy.actor.y)),
	    enemy.actor.explosionRange, bombs_.fuseDurationSeconds());
	enemy.brain.escapingBombId = *bombId;
	OwnedBombPlan plan{*bombId, std::nullopt, enemy.direction};
	if (destructibleWallBlocksAdvance(enemy))
		plan.inspectedWall = ExplosionCell{
		    arena_.columnAt(enemy.actor.x) + enemy.direction.column,
		    arena_.rowAt(enemy.actor.y) + enemy.direction.row};
	enemy.brain.ownedBombPlans.push_back(plan);
	enemy.brain.route = std::move(escapeRoute);
	enemy.brain.routeStep = 0;
	enemy.brain.observedDangerRevision = enemy.brain.perceptionRevision;
	enemy.brain.safetyRefusalLogged = false;
	std::fprintf(stderr, "Enemy plan: index=%zu bomb=%zu safe_cell=(%d,%d).\n",
	             index, *enemy.brain.escapingBombId,
	             enemy.brain.route.back().column, enemy.brain.route.back().row);
	transition(index, EnemyActionState::EscapeToSafeCell, "bomb placed");
	return true;
}

void EnemyController::movePlan(std::size_t index, double deltaSeconds)
{
	EnemyState &enemy = enemies_[index];
	float remaining =
	    actorSpeed_ * static_cast<float>(std::max(0.0, deltaSeconds));
	while (enemy.brain.routeStep < enemy.brain.route.size())
	{
		const ExplosionCell cell = enemy.brain.route[enemy.brain.routeStep];
		const float targetX = arena_.cellCenterX(cell.column);
		const float targetY = arena_.cellCenterY(cell.row);
		const float differenceX = targetX - enemy.actor.x;
		const float differenceY = targetY - enemy.actor.y;
		if (std::abs(differenceX) <= 0.00001F &&
		    std::abs(differenceY) <= 0.00001F)
		{
			enemy.actor.x = targetX;
			enemy.actor.y = targetY;
			++enemy.brain.patrolVisits[cell.row * arena_.width() + cell.column];
			++enemy.brain.routeStep;
			continue;
		}
		if (remaining <= 0.0F)
			return;
		const bool horizontal = std::abs(differenceX) > 0.00001F;
		const float difference = horizontal ? differenceX : differenceY;
		const int sign = difference > 0.0F ? 1 : -1;
		enemy.direction =
		    horizontal ? ExplosionCell{sign, 0} : ExplosionCell{0, sign};
		const float step = std::min(
		    {std::abs(difference), remaining, arena_.cellSize() * 0.25F});
		const float candidateX =
		    enemy.actor.x + static_cast<float>(enemy.direction.column) * step;
		const float candidateY =
		    enemy.actor.y + static_cast<float>(enemy.direction.row) * step;
		const bool playerBehind =
		    (player_.x - enemy.actor.x) *
		            static_cast<float>(enemy.direction.column) +
		        (player_.y - enemy.actor.y) *
		            static_cast<float>(enemy.direction.row) <
		    0.0F;
		// Planning reads snapshots; execution uses authoritative collision to
		// reject same-frame conflicts without exposing them to other brains.
		const ActorCollision collision(arena_, bombs_, player_, enemies_);
		const bool separating =
		    collision.overlapsPlayer(enemy.actor.x, enemy.actor.y,
			                         enemy.actor.collisionHalfExtent) &&
		    std::hypot(candidateX - player_.x, candidateY - player_.y) >
		        std::hypot(enemy.actor.x - player_.x,
				           enemy.actor.y - player_.y);
		if (navigation_->movementStepBlocked(enemy, candidateX, candidateY,
		                                     playerBehind) ||
		    collision.blockedByTerrainOrBombs(candidateX, candidateY,
		                                      enemy.actor) ||
		    collision.overlapsOtherEnemy(enemy, candidateX, candidateY) ||
		    bombs_.intersectsActiveExplosion(candidateX, candidateY,
		                                     enemy.actor.collisionHalfExtent) ||
		    (!playerBehind && !separating &&
		     collision.overlapsPlayer(candidateX, candidateY,
		                              enemy.actor.collisionHalfExtent)))
		{
			enemy.brain.blockedSeconds += std::max(0.0, deltaSeconds);
			if (enemy.brain.blockedSeconds >= 0.35)
			{
				enemy.brain.route.clear();
				enemy.brain.routeStep = 0;
				enemy.brain.blockedSeconds = 0.0;
				transition(index, EnemyActionState::ChooseAction,
				           "persistent physical blocker");
			}
			return;
		}
		enemy.brain.blockedSeconds = 0.0;
		enemy.actor.x = candidateX;
		enemy.actor.y = candidateY;
		remaining -= step;
	}
	enemy.brain.route.clear();
	enemy.brain.routeStep = 0;
	if (enemy.brain.action == EnemyActionState::EscapeToSafeCell)
	{
		std::fprintf(
		    stderr, "Enemy safe cell reached: index=%zu cell=(%d,%d).\n", index,
		    arena_.columnAt(enemy.actor.x), arena_.rowAt(enemy.actor.y));
		enemy.brain.escapingBombId.reset();
		transition(index, EnemyActionState::ChooseAction,
		           "safe cell reached; capacity may remain");
	}
	else
		transition(index, EnemyActionState::ChooseAction,
		           "patrol target reached");
}

void EnemyController::updateEnemy(std::size_t index, double deltaSeconds,
                                  bool pickupsEnabled)
{
	EnemyState &enemy = enemies_[index];
	enemy.brain.retrySeconds =
	    std::max(0.0, enemy.brain.retrySeconds - deltaSeconds);
	if (pickupsEnabled)
		effects_.collectEnemyPickups(index, enemy.actor);

	// dangerRevision changes only when bomb/explosion topology changes. Routes
	// remain committed between revisions instead of being reconsidered per
	// frame.
	const bool dangerChanged =
	    enemy.brain.observedDangerRevision != enemy.brain.perceptionRevision;
	enemy.brain.observedDangerRevision = enemy.brain.perceptionRevision;
	if (enemy.brain.action == EnemyActionState::Patrol &&
	    enemy.brain.objective != EnemyObjective::None &&
	    (!objectiveValid(enemy) ||
	     enemy.brain.threat > enemy.brain.threatAtDecision + 0.75))
	{
		enemy.brain.route.clear();
		enemy.brain.routeStep = 0;
		clearObjective(index, "objective invalidated");
		transition(index, EnemyActionState::ChooseAction,
		           "objective invalidated");
	}
	for (auto plan = enemy.brain.ownedBombPlans.begin();
	     plan != enemy.brain.ownedBombPlans.end();)
	{
		if (bombs_.bombCycleActive(plan->id))
		{
			++plan;
			continue;
		}
		std::fprintf(stderr, "Enemy own explosion ended: index=%zu bomb=%zu.\n",
		             index, plan->id);
		if (enemy.brain.escapingBombId == plan->id)
			enemy.brain.escapingBombId.reset();
		enemy.brain.completedBombPlans.push_back(*plan);
		plan = enemy.brain.ownedBombPlans.erase(plan);
	}
	if (!enemy.brain.completedBombPlans.empty() &&
	    enemy.brain.action != EnemyActionState::EscapeToSafeCell &&
	    enemy.brain.action != EnemyActionState::WaitForExplosion)
		transition(index, EnemyActionState::InspectResult,
		           "owned explosion finished");
	bool routeInvalid = false;
	if (dangerChanged)
	{
		routeInvalid = navigation_->isCellDangerous(
		    enemy, enemy.actor.x, enemy.actor.y,
		    enemy.brain.action == EnemyActionState::EscapeToSafeCell);
		for (std::size_t step = enemy.brain.routeStep;
		     step < enemy.brain.route.size() && !routeInvalid; ++step)
		{
			const float x = arena_.cellCenterX(enemy.brain.route[step].column);
			const float y = arena_.cellCenterY(enemy.brain.route[step].row);
			routeInvalid =
			    navigation_->terrainOrBombBlocked(enemy, x, y) ||
			    navigation_->isCellDangerous(
			        enemy, x, y,
			        enemy.brain.action == EnemyActionState::EscapeToSafeCell);
		}
	}
	bool routeBlocked = false;
	if (enemy.brain.action == EnemyActionState::Patrol &&
	    !enemy.brain.route.empty())
		for (std::size_t step = enemy.brain.routeStep;
		     step < enemy.brain.route.size(); ++step)
		{
			const float x = arena_.cellCenterX(enemy.brain.route[step].column);
			const float y = arena_.cellCenterY(enemy.brain.route[step].row);
			routeBlocked =
			    routeBlocked || navigation_->actorBlocked(enemy, x, y);
		}
	if (routeBlocked)
	{
		enemy.brain.route.clear();
		enemy.brain.routeStep = 0;
		clearObjective(index, "route blocked by actor");
		transition(index, EnemyActionState::ChooseAction,
		           "route invalidated by actor");
	}
	const bool standingInDanger =
	    navigation_->isCellDangerous(enemy, enemy.actor.x, enemy.actor.y);
	if ((dangerChanged && routeInvalid) ||
	    (standingInDanger &&
	     enemy.brain.action != EnemyActionState::EscapeToSafeCell &&
	     (enemy.brain.retrySeconds <= 0.0 ||
	      enemy.brain.action == EnemyActionState::ChooseAction)))
	{
		enemy.brain.retrySeconds = 0.35;
		if (enemy.brain.action == EnemyActionState::Patrol)
			clearObjective(index, "route invalidated by danger");
		auto route = navigation_->findEscapeRoute(enemy, false, actorSpeed_);
		if (dangerChanged ||
		    (enemy.brain.action != EnemyActionState::EscapeToSafeCell &&
		     enemy.brain.action != EnemyActionState::WaitForExplosion))
			std::fprintf(
			    stderr,
			    "Enemy utility: index=%zu winner=survive constraint=mandatory "
			    "route=%s alternatives=attack,pickup,wall,explore "
			    "rejected_by_danger.\n",
			    index, route ? "available" : "unavailable");
		enemy.brain.route.clear();
		enemy.brain.routeStep = 0;
		if (route)
		{
			enemy.brain.route = std::move(*route);
			transition(index, EnemyActionState::EscapeToSafeCell,
			           "new danger; escape replanned");
		}
		else
			transition(index, EnemyActionState::WaitForExplosion,
			           "new danger; no safe route");
	}
	if (enemy.brain.action == EnemyActionState::WaitForExplosion)
	{
		if (enemy.brain.waitingForPlacementWindow)
		{
			if (enemy.brain.ownBombCount() < enemy.actor.maximumActiveBombs &&
			    !navigation_->isCellDangerous(enemy, enemy.actor.x,
			                                  enemy.actor.y))
			{
				enemy.brain.waitingForPlacementWindow = false;
				transition(index, EnemyActionState::ChooseAction,
				           "placement window available");
			}
			else
				return;
		}
		else if (!navigation_->isCellDangerous(enemy, enemy.actor.x,
		                                       enemy.actor.y))
		{
			transition(index,
			           !enemy.brain.completedBombPlans.empty()
			               ? EnemyActionState::InspectResult
						   : EnemyActionState::ChooseAction,
			           "local position safe; resume decisions");
		}
		else
			return;
	}
	if (enemy.brain.action == EnemyActionState::InspectResult)
	{
		// Inspection occurs after the owned explosion disappears, so the Arena
		// queried here is the real post-destruction state.
		for (const OwnedBombPlan &plan : enemy.brain.completedBombPlans)
		{
			if (plan.inspectedWall)
			{
				const ExplosionCell cell = *plan.inspectedWall;
				std::fprintf(stderr,
				             "Enemy inspected result: index=%zu bomb=%zu "
				             "cell=(%d,%d) occupied=%s.\n",
				             index, plan.id, cell.column, cell.row,
				             arena_.isOccupied(cell.column, cell.row) ? "yes"
				                                                      : "no");
			}
			else
				std::fprintf(stderr,
				             "Enemy inspected result: index=%zu bomb=%zu arena "
				             "checked after attack.\n",
				             index, plan.id);
			enemy.direction = plan.placementDirection;
		}
		enemy.brain.completedBombPlans.clear();
		transition(index, EnemyActionState::ChooseAction,
		           "inspection complete");
	}
	if (enemy.brain.action == EnemyActionState::Patrol &&
	    enemy.brain.route.empty() &&
	    (dangerChanged || enemy.brain.retrySeconds <= 0.0))
	{
		constexpr std::array<ExplosionCell, 4> directions{
		    {{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
		for (const ExplosionCell direction : directions)
		{
			const ExplosionCell cell{
			    arena_.columnAt(enemy.actor.x) + direction.column,
			    arena_.rowAt(enemy.actor.y) + direction.row};
			const float x = arena_.cellCenterX(cell.column);
			const float y = arena_.cellCenterY(cell.row);
			if (navigation_->contains(cell) &&
			    !navigation_->terrainOrBombBlocked(enemy, x, y) &&
			    !navigation_->isCellDangerous(enemy, x, y))
			{
				transition(index, EnemyActionState::ChooseAction,
				           "patrol cell became available");
				break;
			}
		}
	}
	if (enemy.brain.action == EnemyActionState::ChooseAction)
		chooseObjective(index);
	if (!enemy.brain.route.empty() &&
	    (enemy.brain.action == EnemyActionState::Patrol ||
	     enemy.brain.action == EnemyActionState::EscapeToSafeCell))
	{
		movePlan(index, deltaSeconds);
	}
	bombs_.updatePassThrough(arena_, enemy.actor);
	if (pickupsEnabled)
		effects_.collectEnemyPickups(index, enemy.actor);
}

bool EnemyController::destructibleWallBlocksAdvance(
    const EnemyState &enemy) const
{
	const int column = arena_.columnAt(enemy.actor.x) + enemy.direction.column;
	const int row = arena_.rowAt(enemy.actor.y) + enemy.direction.row;
	return arena_.cellType(column, row) == CellType::DestructibleWall;
}
} // namespace game
