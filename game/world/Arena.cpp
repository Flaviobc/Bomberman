/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   Arena.cpp                                         :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/09/25          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/09/25          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "world/Arena.hpp"
#include "world/PrototypeArenaGenerator.hpp"

#include <cmath>
#include <utility>

namespace game
{
Arena Arena::makePrototype(int width, int height, std::uint32_t generationSeed)
{
	PrototypeArenaLayout layout =
	    PrototypeArenaGenerator::generate(width, height, generationSeed);
	return {layout.width,   layout.height,  layout.cellSize,
	        layout.originX, layout.originY, std::move(layout.cells),
	        generationSeed};
}

std::uint32_t Arena::prototypeSeed()
{
	return PrototypeArenaGenerator::seed;
}

Arena::Arena(int width, int height, float cellSize, float originX,
             float originY, std::vector<CellType> cells,
             std::uint32_t generationSeed)
    : width_(width), height_(height), cellSize_(cellSize), originX_(originX),
      originY_(originY), cells_(std::move(cells)),
      generationSeed_(generationSeed)
{
}

CellType Arena::cellType(int column, int row) const
{
	if (column < 0 || column >= width_ || row < 0 || row >= height_)
	{
		return CellType::IndestructibleWall;
	}
	return cells_[static_cast<std::size_t>(row * width_ + column)];
}

bool Arena::isOccupied(int column, int row) const
{
	return cellType(column, row) != CellType::Free;
}

bool Arena::destroyDestructibleWall(int column, int row)
{
	if (column < 0 || column >= width_ || row < 0 || row >= height_)
	{
		return false;
	}
	CellType &cell = cells_[static_cast<std::size_t>(row * width_ + column)];
	if (cell != CellType::DestructibleWall)
	{
		return false;
	}
	cell = CellType::Free;
	return true;
}

bool Arena::intersectsOccupied(float minimumX, float minimumY, float maximumX,
                               float maximumY) const
{
	const int firstColumn =
	    static_cast<int>(std::floor((minimumX - originX_) / cellSize_));
	const int lastColumn =
	    static_cast<int>(std::floor((maximumX - originX_) / cellSize_));
	const int firstRow =
	    static_cast<int>(std::floor((minimumY - originY_) / cellSize_));
	const int lastRow =
	    static_cast<int>(std::floor((maximumY - originY_) / cellSize_));
	for (int row = firstRow; row <= lastRow; ++row)
	{
		for (int column = firstColumn; column <= lastColumn; ++column)
		{
			if (isOccupied(column, row))
			{
				return true;
			}
		}
	}
	return false;
}

int Arena::columnAt(float x) const
{
	return static_cast<int>(std::floor((x - originX_) / cellSize_));
}

int Arena::rowAt(float y) const
{
	return static_cast<int>(std::floor((y - originY_) / cellSize_));
}

float Arena::cellCenterX(int column) const
{
	return originX_ + (static_cast<float>(column) + 0.5F) * cellSize_;
}

float Arena::cellCenterY(int row) const
{
	return originY_ + (static_cast<float>(row) + 0.5F) * cellSize_;
}

int Arena::width() const
{
	return width_;
}

int Arena::height() const
{
	return height_;
}

float Arena::cellSize() const
{
	return cellSize_;
}

float Arena::originX() const
{
	return originX_;
}

float Arena::originY() const
{
	return originY_;
}

std::uint32_t Arena::generationSeed() const
{
	return generationSeed_;
}
} // namespace game
