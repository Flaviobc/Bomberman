/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   PlayerController.hpp                             :+:      :+:    :+:   :+:      */
/*                                                    +:+ +:+         +:+     +:+    */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/09          by flaviobc      #+#    #+#             */
/*   Updated: 2026/10/09          by flaviobc     ###   ########.fr       */
/*                                                                                    */
/* ******************************************************************************** */

#pragma once

#include "actors/EnemyState.hpp"
#include "actors/PlayerState.hpp"
#include "input/PlayerInput.hpp"

#include <span>

namespace game
{
class Arena;
class BombSystem;
class ActorCollision;

// Owns only control history. Player, terrain, bombs and enemies are borrowed
// for one synchronous update and remain authoritative in GameSession.
class PlayerController final
{
  public:
	void update(PlayerState &player, MoveAction action, double deltaSeconds,
	            float actorSpeed, const Arena &arena, const BombSystem &bombs,
	            std::span<const EnemyState> enemies);
	void reset(PlayerState &player);

  private:
	struct MovementResult
	{
		bool blockedHorizontally = false;
		bool blockedVertically = false;
	};

	MovementResult move(PlayerState &player, MoveAction action,
	                    double deltaSeconds, float actorSpeed,
	                    const Arena &arena,
	                    const ActorCollision &collision) const;

	MovementResult previousMovement_;
};
} // namespace game
