#pragma once

#include "world/Arena.hpp"
#include "config/GameplayTuning.hpp"

#include <cstdint>
#include <vector>

namespace game
{
// Immutable construction result consumed immediately by Arena::makePrototype.
// Arena owns the moved cells afterwards and is the only runtime map authority.
struct PrototypeArenaLayout
{
	int width = 0;
	int height = 0;
	float cellSize = 0.0F;
	float originX = 0.0F;
	float originY = 0.0F;
	std::vector<CellType> cells;
};

class PrototypeArenaGenerator
{
  public:
	static constexpr std::uint32_t seed = GameplayTuning::baseArenaSeed;

	static PrototypeArenaLayout generate(int width, int height,
	                                     std::uint32_t generationSeed = seed);
};
} // namespace game
