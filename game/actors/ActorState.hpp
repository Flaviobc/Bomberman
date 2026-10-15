/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   ActorState.hpp                                    :+:      :+:    :+: :+:      */
/*                                                    +:+ +:+   +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+ +#+#+#+#+#+   +#+      */
/*                                                +#+#+#+#+#+   +#+           +#+#+#+#+#+   */
/*   Created: 2026/10/06          by flaviobc      #+#    #+#             */
/*   Updated: 2026/10/06          by flaviobc     ###   ########.fr       */
/*                                                                                    */
/* ******************************************************************************** */
/*                                                                                    */
/* **********************************************************************************
 */

#pragma once

#include <cstddef>
#include <vector>

namespace game
{
enum class ActorKind
{
	Player,
	Enemy,
};

// Damage state shared by every actor. Durations are measured in seconds.
struct HealthState
{
	int lives = 1;
	double invulnerabilityRemainingSeconds = 0.0;
};

// Temporary permission to leave a bomb cell, stored in world-space X/Y.
struct BombPassThrough
{
	float x = 0.0F;
	float y = 0.0F;
};

// Common logical state owned by Gameplay; position and extents use world units.
struct ActorState
{
	float x = 0.0F;
	float y = 0.0F;
	float collisionHalfExtent = 0.0F;
	std::size_t maximumActiveBombs = 1;
	int explosionRange = 1;
	HealthState health;
	std::vector<BombPassThrough> bombPassThrough;
};
} // namespace game
