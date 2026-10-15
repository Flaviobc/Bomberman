/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   ActorCollision.cpp                               :+:      :+:    :+:   :+:      */
/*                                                    +:+ +:+         +:+     +:+    */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           +#+#+#+#+#+   */
/*   Created: 2026/10/07          by flaviobc      #+#    #+#             */
/*   Updated: 2026/10/07          by flaviobc     ###   ########.fr       */
/*                                                                                    */
/* ******************************************************************************** */

#include "collision/ActorCollision.hpp"

#include "combat/BombSystem.hpp"
#include "world/Arena.hpp"

#include <algorithm>
#include <cmath>

namespace game
{
ActorCollision::ActorCollision(const Arena &arena, const BombSystem &bombs,
                               const ActorState &player,
                               std::span<const EnemyState> enemies)
    : arena_(arena), bombs_(bombs), player_(player), enemies_(enemies)
{
}

bool ActorCollision::blockedByTerrainOrBombs(float centerX, float centerY,
                                             const ActorState &actor) const
{
	const float halfExtent = actor.collisionHalfExtent;
	if (arena_.intersectsOccupied(centerX - halfExtent, centerY - halfExtent,
	                              centerX + halfExtent, centerY + halfExtent))
	{
		return true;
	}
	return bombs_.blocksActor(arena_, centerX, centerY, actor);
}

bool ActorCollision::overlapsActiveEnemy(float centerX, float centerY,
                                         float halfExtent) const
{
	return std::any_of(enemies_.begin(), enemies_.end(),
	                   [centerX, centerY, halfExtent](const EnemyState &enemy)
	                   {
		                   return enemy.active &&
						          overlaps(centerX, centerY, halfExtent,
						                   enemy.actor.x, enemy.actor.y,
						                   enemy.actor.collisionHalfExtent);
	                   });
}

bool ActorCollision::overlapsPlayer(float centerX, float centerY,
                                    float halfExtent) const
{
	return overlaps(centerX, centerY, halfExtent, player_.x, player_.y,
	                player_.collisionHalfExtent);
}

bool ActorCollision::overlapsOtherEnemy(const EnemyState &enemy, float centerX,
                                        float centerY) const
{
	return std::any_of(
	    enemies_.begin(), enemies_.end(),
	    [&enemy, centerX, centerY](const EnemyState &other)
	    {
		    const float separation = enemy.actor.collisionHalfExtent +
			                         other.actor.collisionHalfExtent;
		    // Preserve the original inclusive enemy separation comparison.
		    return &enemy != &other && other.active &&
			       std::abs(centerX - other.actor.x) <= separation &&
			       std::abs(centerY - other.actor.y) <= separation;
	    });
}

bool ActorCollision::canPlayerFollowActiveEnemy(float centerX, float centerY,
                                                float halfExtent,
                                                float movementX,
                                                float movementY) const
{
	for (const EnemyState &enemy : enemies_)
	{
		if ((enemy.brain.action != EnemyActionState::Patrol &&
		     enemy.brain.action != EnemyActionState::EscapeToSafeCell) ||
		    enemy.brain.routeStep >= enemy.brain.route.size())
		{
			continue;
		}
		const float targetX =
		    arena_.cellCenterX(enemy.brain.route[enemy.brain.routeStep].column);
		const float targetY =
		    arena_.cellCenterY(enemy.brain.route[enemy.brain.routeStep].row);
		const float deltaX = targetX - enemy.actor.x;
		const float deltaY = targetY - enemy.actor.y;
		if (deltaX * movementX + deltaY * movementY <= 0.0F ||
		    (movementX != 0.0F && std::abs(deltaX) < std::abs(movementX)) ||
		    (movementY != 0.0F && (std::abs(deltaX) > 0.00001F ||
		                           std::abs(deltaY) < std::abs(movementY))) ||
		    blockedByTerrainOrBombs(enemy.actor.x + movementX,
		                            enemy.actor.y + movementY, enemy.actor) ||
		    bombs_.intersectsActiveExplosion(enemy.actor.x + movementX,
		                                     enemy.actor.y + movementY,
		                                     enemy.actor.collisionHalfExtent))
		{
			continue;
		}
		// Rear contact may follow an enemy that is already moving away. Front
		// contact remains blocking, which prevents the Player from pushing it.
		const bool playerIsBehind = (centerX - enemy.actor.x) * movementX +
		                                (centerY - enemy.actor.y) * movementY <
		                            0.0F;
		if (enemy.active && playerIsBehind &&
		    overlaps(centerX, centerY, halfExtent, enemy.actor.x, enemy.actor.y,
		             enemy.actor.collisionHalfExtent))
		{
			return true;
		}
	}
	return false;
}

bool ActorCollision::overlaps(float firstX, float firstY, float firstHalfExtent,
                              float secondX, float secondY,
                              float secondHalfExtent)
{
	// Touching faces count as collision, matching the existing inclusive AABBs.
	return firstX + firstHalfExtent >= secondX - secondHalfExtent &&
	       firstX - firstHalfExtent <= secondX + secondHalfExtent &&
	       firstY + firstHalfExtent >= secondY - secondHalfExtent &&
	       firstY - firstHalfExtent <= secondY + secondHalfExtent;
}
} // namespace game
