/* ********************************************************************************
 */
/*                                                                                    */
/*                                                      :::      ::::::::   :::
 */
/*   PickupSystem.hpp                                 :+:      :+:    :+:   :+:
 */
/*                                                    +:+ +:+         +:+ +:+ */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+       +#+ */
/*                                                +#+#+#+#+#+   +#+ +#+#+#+#+#+
 */
/*   Created: 2026/10/07          by flaviobc      #+#    #+#             */
/*   Updated: 2026/10/07          by flaviobc     ###   ########.fr       */
/*                                                                                    */
/* ********************************************************************************
 */

#pragma once

#include "actors/ActorState.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace game
{
class Arena;

enum class PickupEffect
{
	BombCapacity,
	ExplosionRange,
};

// Logical item state owned by PickupSystem; coordinates identify Arena cells.
struct PickupState
{
	int column = 0;
	int row = 0;
	PickupEffect effect = PickupEffect::BombCapacity;
	bool revealed = false;
	bool collected = false;
};

struct LifePickupState
{
	int column = 0;
	int row = 0;
	bool active = true;
};

struct PickupCollection
{
	int column = 0;
	int row = 0;
	PickupEffect effect = PickupEffect::BombCapacity;
};

struct LifePickupCollection
{
	int column = 0;
	int row = 0;
	int previousLives = 0;
};

// Owns item distribution and one-shot collection. Returned views remain valid
// until the next reset; callers retain ownership of the collecting ActorState.
class PickupSystem final
{
  public:
	static constexpr int maximumLives = 3;
	static constexpr int explosionRangeIncrease = 1;

	void reset(const Arena &arena, bool enabled);
	bool revealAt(int column, int row);
	std::optional<PickupCollection> collectAt(const Arena &arena,
	                                          ActorState &collector);
	std::optional<LifePickupCollection> collectLifeAt(const Arena &arena,
	                                                  ActorState &collector);
	std::span<const PickupState> powerUps() const;
	const LifePickupState &lifePickup() const;

  private:
	std::vector<PickupState> powerUps_;
	std::optional<std::uint32_t> seed_;
	LifePickupState lifePickup_;
};
} // namespace game
