/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   ActorCollision.hpp                               :+:      :+:    :+:   :+:      */
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

#include <span>

namespace game
{
class Arena;
class BombSystem;

// Non-owning collision view. All positions and AABBs use Arena world-space X/Y;
// referenced objects must outlive this short-lived query object.
class ActorCollision final
{
  public:
	ActorCollision(const Arena &arena, const BombSystem &bombs,
	               const ActorState &player,
	               std::span<const EnemyState> enemies);

	bool blockedByTerrainOrBombs(float centerX, float centerY,
	                             const ActorState &actor) const;
	bool overlapsActiveEnemy(float centerX, float centerY,
	                         float halfExtent) const;
	bool overlapsPlayer(float centerX, float centerY, float halfExtent) const;
	bool overlapsOtherEnemy(const EnemyState &enemy, float centerX,
	                        float centerY) const;
	bool canPlayerFollowActiveEnemy(float centerX, float centerY,
	                                float halfExtent, float movementX,
	                                float movementY) const;

  private:
	static bool overlaps(float firstX, float firstY, float firstHalfExtent,
	                     float secondX, float secondY, float secondHalfExtent);

	const Arena &arena_;
	const BombSystem &bombs_;
	const ActorState &player_;
	std::span<const EnemyState> enemies_;
};
} // namespace game
