/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   EnemyRoster.hpp                                  :+:      :+:    :+: :+:      */
/*                                                    +:+ +:+   +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+ +#+#+#+#+#+   +#+      */
/*                                                +#+#+#+#+#+   +#+           +#+#+#+#+#+   */
/*   Created: 2026/10/09          by flaviobc      #+#    #+#             */
/*   Updated: 2026/10/09          by flaviobc     ###   ########.fr       */
/*                                                                                    */
/* ******************************************************************************** */
/*                                                                                    */
/* **********************************************************************************
 */

#pragma once

#include "actors/EnemyState.hpp"
#include "config/GameplayTuning.hpp"

#include <array>
#include <cstddef>
#include <span>

namespace game
{
class Arena;

// Owns the fixed enemy slots and their lifecycle, but not their decisions.
class EnemyRoster
{
  public:
	static constexpr std::size_t slotCount = 3;
	explicit EnemyRoster(
	    double damageInvulnerabilitySeconds =
	        GameplayTuning::baseDamageInvulnerabilitySeconds,
	    std::array<EnemyPersonalityProfile, slotCount> personalities = {
	        EnemyPersonalityProfile::Strategist,
	        EnemyPersonalityProfile::Strategist,
	        EnemyPersonalityProfile::Strategist});

	void configureCollisionHalfExtent(float collisionHalfExtent);
	void reset(const Arena &arena, bool enabled);
	void advanceInvulnerability(double deltaSeconds);
	void applyDamage(std::size_t index);

	bool hasActiveEnemy() const;
	std::size_t activeEnemyCount() const;
	bool isActive(std::size_t index) const;

	// Borrowed views remain valid for the lifetime of this fixed roster.
	std::span<EnemyState> view();
	std::span<const EnemyState> view() const;

  private:
	std::array<EnemyState, slotCount> enemies_{};
	std::array<EnemyPersonalityProfile, slotCount> personalities_;
	double damageInvulnerabilitySeconds_;
};
} // namespace game
