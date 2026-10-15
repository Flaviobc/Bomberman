/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   PlayerController.cpp                             :+:      :+:    :+:   :+:      */
/*                                                    +:+ +:+         +:+     +:+    */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/09          by flaviobc      #+#    #+#             */
/*   Updated: 2026/10/09          by flaviobc     ###   ########.fr       */
/*                                                                                    */
/* ******************************************************************************** */

#include "actors/PlayerController.hpp"

#include "collision/ActorCollision.hpp"
#include "world/Arena.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>

namespace
{
constexpr std::size_t kStationaryVisualState = 0;
constexpr std::size_t kMovingVisualState = 1;
} // namespace

namespace game
{
void PlayerController::update(PlayerState &player, MoveAction action,
                              double deltaSeconds, float actorSpeed,
                              const Arena &arena, const BombSystem &bombs,
                              std::span<const EnemyState> enemies)
{
	if (!std::isfinite(action.horizontal) || !std::isfinite(action.vertical))
		action = {};
	if (action.horizontal != 0.0F || action.vertical != 0.0F)
		player.facingRadians = std::atan2(-action.horizontal, action.vertical);

	const float previousX = player.actor.x;
	const float previousY = player.actor.y;
	const ActorCollision collision(arena, bombs, player.actor, enemies);
	const MovementResult movement =
	    move(player, action, deltaSeconds, actorSpeed, arena, collision);
	player.visualAnimationIndex =
	    player.actor.x != previousX || player.actor.y != previousY
	        ? kMovingVisualState
	        : kStationaryVisualState;

	if ((movement.blockedHorizontally &&
	     !previousMovement_.blockedHorizontally) ||
	    (movement.blockedVertically && !previousMovement_.blockedVertically))
	{
		std::fprintf(stderr,
		             "Arena collision: position=(%.3f, %.3f) x=%s y=%s.\n",
		             player.actor.x, player.actor.y,
		             movement.blockedHorizontally ? "blocked" : "free",
		             movement.blockedVertically ? "blocked" : "free");
	}
	previousMovement_ = movement;
}

void PlayerController::reset(PlayerState &player)
{
	previousMovement_ = {};
	player.visualAnimationIndex = kStationaryVisualState;
}

PlayerController::MovementResult PlayerController::move(
    PlayerState &player, MoveAction action, double deltaSeconds,
    float actorSpeed, const Arena &arena, const ActorCollision &collision) const
{
	MovementResult result{};
	if (!std::isfinite(actorSpeed) || actorSpeed <= 0.0F ||
	    !std::isfinite(deltaSeconds) || deltaSeconds <= 0.0)
		return result;
	const float magnitudeSquared = action.horizontal * action.horizontal +
	                               action.vertical * action.vertical;
	if (magnitudeSquared > 1.0F)
	{
		const float inverseMagnitude =
		    std::isfinite(magnitudeSquared)
		        ? 1.0F / std::sqrt(magnitudeSquared)
		        : static_cast<float>(
		              1.0 / std::hypot(static_cast<double>(action.horizontal),
					                   static_cast<double>(action.vertical)));
		action.horizontal *= inverseMagnitude;
		action.vertical *= inverseMagnitude;
	}

	const float maximumStep = arena.cellSize() * 0.25F;
	// One arena span per update bounds collision work, even after a long stall
	// or with extreme tuning. Excess travel is discarded, never enlarged steps.
	const int maximumSteps = 4 * std::max(arena.width(), arena.height());
	const float maximumDistance = maximumStep * maximumSteps;
	const float distance =
	    deltaSeconds >= static_cast<double>(maximumDistance) / actorSpeed
	        ? maximumDistance
	        : std::min(maximumDistance,
	                   deltaSeconds > std::numeric_limits<float>::max()
	                       ? static_cast<float>(actorSpeed * deltaSeconds)
	                       : actorSpeed * static_cast<float>(deltaSeconds));
	const float movementX = action.horizontal * distance;
	const float movementY = action.vertical * distance;
	const int steps = std::max(
	    1,
	    static_cast<int>(std::ceil(
	        std::max(std::abs(movementX), std::abs(movementY)) / maximumStep)));
	const float stepX = movementX / static_cast<float>(steps);
	const float stepY = movementY / static_cast<float>(steps);
	const float enemyFollowDistance = distance / static_cast<float>(steps);
	for (int step = 0; step < steps; ++step)
	{
		const float candidateX = player.actor.x + stepX;
		if (!collision.blockedByTerrainOrBombs(candidateX, player.actor.y,
		                                       player.actor) &&
		    !collision.overlapsActiveEnemy(candidateX, player.actor.y,
		                                   player.actor.collisionHalfExtent))
		{
			player.actor.x += stepX;
		}
		else if (stepX != 0.0F &&
		         collision.canPlayerFollowActiveEnemy(
		             candidateX, player.actor.y,
		             player.actor.collisionHalfExtent, stepX, 0.0F))
		{
			const float followStep = std::copysign(
			    std::min(std::abs(stepX), enemyFollowDistance), stepX);
			if (!collision.blockedByTerrainOrBombs(
			        player.actor.x + followStep, player.actor.y, player.actor))
			{
				player.actor.x += followStep;
			}
			else
			{
				result.blockedHorizontally = true;
			}
		}
		else if (stepX != 0.0F)
		{
			result.blockedHorizontally = true;
		}

		if (!collision.blockedByTerrainOrBombs(
		        player.actor.x, player.actor.y + stepY, player.actor) &&
		    !collision.overlapsActiveEnemy(player.actor.x,
		                                   player.actor.y + stepY,
		                                   player.actor.collisionHalfExtent))
		{
			player.actor.y += stepY;
		}
		else if (stepY != 0.0F &&
		         collision.canPlayerFollowActiveEnemy(
		             player.actor.x, player.actor.y + stepY,
		             player.actor.collisionHalfExtent, 0.0F, stepY))
		{
			const float followStep = std::copysign(
			    std::min(std::abs(stepY), enemyFollowDistance), stepY);
			if (!collision.blockedByTerrainOrBombs(
			        player.actor.x, player.actor.y + followStep, player.actor))
				player.actor.y += followStep;
			else
				result.blockedVertically = true;
		}
		else if (stepY != 0.0F)
		{
			result.blockedVertically = true;
		}
	}
	return result;
}
} // namespace game
