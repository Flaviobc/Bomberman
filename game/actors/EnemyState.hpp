/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   EnemyState.hpp                                    :+:      :+:    :+: :+:      */
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

#include "actors/ActorState.hpp"
#include "ai/EnemyBrain.hpp"
#include "combat/Explosion.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace game
{
// Authoritative physical actor plus its exclusively owned decision/memory
// state.
struct EnemyState
{
	ActorState actor;
	ExplosionCell direction{1, 0};
	bool active = true;
	EnemyBrain brain;
};
} // namespace game
