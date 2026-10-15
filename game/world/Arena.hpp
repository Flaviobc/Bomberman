/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   Arena.hpp                                         :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/09/25          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/09/25          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

#include "config/GameplayTuning.hpp"

#include <cstdint>
#include <vector>

namespace game
{
enum class CellType
{
	Free,
	IndestructibleWall,
	DestructibleWall,
};

class Arena
{
  public:
	static Arena
	makePrototype(int width = 15, int height = 15,
	              std::uint32_t generationSeed = GameplayTuning::baseArenaSeed);
	static std::uint32_t prototypeSeed();

	Arena(int width, int height, float cellSize, float originX, float originY,
	      std::vector<CellType> cells,
	      std::uint32_t generationSeed = GameplayTuning::baseArenaSeed);

	CellType cellType(int column, int row) const;
	bool isOccupied(int column, int row) const;
	bool destroyDestructibleWall(int column, int row);
	bool intersectsOccupied(float minimumX, float minimumY, float maximumX,
	                        float maximumY) const;
	int columnAt(float x) const;
	int rowAt(float y) const;
	float cellCenterX(int column) const;
	float cellCenterY(int row) const;
	int width() const;
	int height() const;
	float cellSize() const;
	float originX() const;
	float originY() const;
	std::uint32_t generationSeed() const;

  private:
	int width_;
	int height_;
	float cellSize_;
	float originX_;
	float originY_;
	std::vector<CellType> cells_;
	std::uint32_t generationSeed_;
};
} // namespace game
