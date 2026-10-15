/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   ExplosionDamage.hpp                              :+:      :+:    :+:   :+:      */
/*                                                    +:+ +:+         +:+     +:+    */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           +#+#+#+#+#+   */
/*   Created: 2026/10/07          by flaviobc      #+#    #+#             */
/*   Updated: 2026/10/07          by flaviobc     ###   ########.fr       */
/*                                                                                    */
/* ******************************************************************************** */

#pragma once

#include "actors/ActorState.hpp"
#include "combat/Explosion.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace game
{
struct ExplosionActorIdentity
{
	ActorKind kind = ActorKind::Player;
	std::size_t index = 0;

	bool operator==(const ExplosionActorIdentity &) const = default;
};

struct ExplosionDamageTarget
{
	ActorKind kind = ActorKind::Player;
	std::size_t index = 0;
	// Borrowed only for the current evaluation; no health is changed here.
	const ActorState *actor = nullptr;
	bool active = false;
};

struct ExplosionDamageResult
{
	bool playerHit = false;
	std::vector<std::size_t> enemiesHit;
};

// Detects and records hits. GameSession alone converts this result into lives,
// feedback and terminal state changes.
class ExplosionDamage final
{
  public:
	static void collectHits(std::span<const ExplosionProjectionCell> projection,
	                        std::span<const ExplosionDamageTarget> targets,
	                        std::vector<ExplosionActorIdentity> &alreadyHit,
	                        ExplosionDamageResult &result);

  private:
	static bool
	targetIsEligible(const ExplosionDamageTarget &target,
	                 const std::vector<ExplosionActorIdentity> &alreadyHit);
	static bool intersects(const ExplosionDamageTarget &target,
	                       const ExplosionProjectionCell &cell);
	static void appendAggregate(const ExplosionDamageTarget &target,
	                            ExplosionDamageResult &result);
};
} // namespace game
