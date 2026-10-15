/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   ExplosionDamage.cpp                              :+:      :+:    :+:   :+:      */
/*                                                    +:+ +:+         +:+     +:+    */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           +#+#+#+#+#+   */
/*   Created: 2026/10/07          by flaviobc      #+#    #+#             */
/*   Updated: 2026/10/07          by flaviobc     ###   ########.fr       */
/*                                                                                    */
/* ******************************************************************************** */

#include "combat/ExplosionDamage.hpp"

#include <algorithm>

namespace game
{
void ExplosionDamage::collectHits(
    std::span<const ExplosionProjectionCell> projection,
    std::span<const ExplosionDamageTarget> targets,
    std::vector<ExplosionActorIdentity> &alreadyHit,
    ExplosionDamageResult &result)
{
	for (const ExplosionDamageTarget &target : targets)
	{
		if (!targetIsEligible(target, alreadyHit) ||
		    !std::any_of(projection.begin(), projection.end(),
		                 [&target](const ExplosionProjectionCell &cell)
		                 { return intersects(target, cell); }))
		{
			continue;
		}
		alreadyHit.push_back({target.kind, target.index});
		appendAggregate(target, result);
	}
}

bool ExplosionDamage::targetIsEligible(
    const ExplosionDamageTarget &target,
    const std::vector<ExplosionActorIdentity> &alreadyHit)
{
	if (!target.active || target.actor == nullptr ||
	    target.actor->health.invulnerabilityRemainingSeconds > 0.0)
	{
		return false;
	}
	const ExplosionActorIdentity identity{target.kind, target.index};
	return std::find(alreadyHit.begin(), alreadyHit.end(), identity) ==
	       alreadyHit.end();
}

bool ExplosionDamage::intersects(const ExplosionDamageTarget &target,
                                 const ExplosionProjectionCell &cell)
{
	const float halfExtent = target.actor->collisionHalfExtent;
	return target.actor->x + halfExtent >= cell.x &&
	       target.actor->x - halfExtent <= cell.x + cell.width &&
	       target.actor->y + halfExtent >= cell.y &&
	       target.actor->y - halfExtent <= cell.y + cell.height;
}

void ExplosionDamage::appendAggregate(const ExplosionDamageTarget &target,
                                      ExplosionDamageResult &result)
{
	if (target.kind == ActorKind::Player)
	{
		result.playerHit = true;
		return;
	}
	const auto position = std::lower_bound(
	    result.enemiesHit.begin(), result.enemiesHit.end(), target.index);
	if (position == result.enemiesHit.end() || *position != target.index)
		result.enemiesHit.insert(position, target.index);
}
} // namespace game
