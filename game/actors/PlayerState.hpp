/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   PlayerState.hpp                                   :+:      :+:    :+: :+:      */
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

#include <cstddef>

namespace game
{
// Player-only presentation state composed with the shared logical actor state.
struct PlayerState
{
	ActorState actor;
	float facingRadians = 0.0F;
	std::size_t visualAnimationIndex = 0;
};
} // namespace game
