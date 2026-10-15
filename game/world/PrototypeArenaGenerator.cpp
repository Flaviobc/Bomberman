#include "world/PrototypeArenaGenerator.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <queue>
#include <random>
#include <stdexcept>
#include <utility>

namespace
{
constexpr int kBaselineEligibleCellsPerSector = 27;
constexpr int kBaselineWallsPerSector = 16;

struct Sector
{
	int minimumColumn;
	int maximumColumn;
	int minimumRow;
	int maximumRow;
	int respawnColumn;
	int respawnRow;
	int centralAccessColumn;
	int centralAccessRow;
	int variationColumn;
	int variationRow;
};

std::size_t cellIndex(int width, int column, int row)
{
	return static_cast<std::size_t>(row * width + column);
}

bool isFree(const std::vector<game::CellType> &cells, int width, int column,
            int row)
{
	return cells[cellIndex(width, column, row)] == game::CellType::Free;
}

bool isInside(const Sector &sector, int column, int row)
{
	return column >= sector.minimumColumn && column <= sector.maximumColumn &&
	       row >= sector.minimumRow && row <= sector.maximumRow;
}

// Ensures every reserved respawn can leave its cell before deeper validation.
bool hasImmediateExit(const std::vector<game::CellType> &cells, int width,
                      const Sector &sector)
{
	static constexpr std::array<std::array<int, 2>, 4> directions{{
	    {{1, 0}},
	    {{-1, 0}},
	    {{0, 1}},
	    {{0, -1}},
	}};
	for (const auto &direction : directions)
	{
		const int column = sector.respawnColumn + direction[0];
		const int row = sector.respawnRow + direction[1];
		if (isInside(sector, column, row) && isFree(cells, width, column, row))
		{
			return true;
		}
	}
	return false;
}

// BFS verifies that no final free cell is isolated from its sector respawn.
bool isSectorConnected(const std::vector<game::CellType> &cells, int width,
                       const Sector &sector)
{
	if (!isFree(cells, width, sector.respawnColumn, sector.respawnRow) ||
	    !isFree(cells, width, sector.centralAccessColumn,
	            sector.centralAccessRow) ||
	    !hasImmediateExit(cells, width, sector))
	{
		return false;
	}
	std::vector<bool> reached(cells.size(), false);
	std::queue<std::array<int, 2>> pending;
	pending.push({sector.respawnColumn, sector.respawnRow});
	reached[cellIndex(width, sector.respawnColumn, sector.respawnRow)] = true;
	static constexpr std::array<std::array<int, 2>, 4> directions{{
	    {{1, 0}},
	    {{-1, 0}},
	    {{0, 1}},
	    {{0, -1}},
	}};
	while (!pending.empty())
	{
		const std::array<int, 2> cell = pending.front();
		pending.pop();
		for (const auto &direction : directions)
		{
			const int column = cell[0] + direction[0];
			const int row = cell[1] + direction[1];
			if (!isInside(sector, column, row) ||
			    reached[cellIndex(width, column, row)] ||
			    !isFree(cells, width, column, row))
			{
				continue;
			}
			reached[cellIndex(width, column, row)] = true;
			pending.push({column, row});
		}
	}
	for (int row = sector.minimumRow; row <= sector.maximumRow; ++row)
	{
		for (int column = sector.minimumColumn; column <= sector.maximumColumn;
		     ++column)
		{
			if (isFree(cells, width, column, row) &&
			    !reached[cellIndex(width, column, row)])
			{
				return false;
			}
		}
	}
	return true;
}

std::size_t countCells(const std::vector<game::CellType> &cells, int width,
                       const Sector &sector, game::CellType type);

std::uint64_t wallSignature(const std::vector<game::CellType> &cells, int width,
                            const Sector &sector)
{
	std::uint64_t signature = 0;
	int bit = 0;
	for (int row = sector.minimumRow; row <= sector.maximumRow; ++row)
	{
		for (int column = sector.minimumColumn; column <= sector.maximumColumn;
		     ++column, ++bit)
		{
			if (cells[cellIndex(width, column, row)] ==
			    game::CellType::DestructibleWall)
			{
				if (bit < 64)
					signature |= std::uint64_t{1} << bit;
				else
					signature =
					    (signature ^ static_cast<std::uint64_t>(bit + 1)) *
					    1099511628211ULL;
			}
		}
	}
	return signature;
}

// Retains one shuffled path so random walls cannot sever required access.
bool appendRandomPath(const std::vector<game::CellType> &cells, int width,
                      const Sector &sector, int startColumn, int startRow,
                      int targetColumn, int targetRow, std::mt19937 &generator,
                      std::vector<bool> &retained)
{
	std::vector<int> predecessor(cells.size(), -1);
	const std::size_t startIndex = cellIndex(width, startColumn, startRow);
	const std::size_t targetIndex = cellIndex(width, targetColumn, targetRow);
	std::queue<std::array<int, 2>> pending;
	pending.push({startColumn, startRow});
	predecessor[startIndex] = static_cast<int>(startIndex);
	static constexpr std::array<std::array<int, 2>, 4> baseDirections{{
	    {{1, 0}},
	    {{-1, 0}},
	    {{0, 1}},
	    {{0, -1}},
	}};
	while (!pending.empty() && predecessor[targetIndex] == -1)
	{
		const std::array<int, 2> cell = pending.front();
		pending.pop();
		auto directions = baseDirections;
		std::shuffle(directions.begin(), directions.end(), generator);
		for (const auto &direction : directions)
		{
			const int column = cell[0] + direction[0];
			const int row = cell[1] + direction[1];
			if (!isInside(sector, column, row) ||
			    !isFree(cells, width, column, row))
			{
				continue;
			}
			const std::size_t index = cellIndex(width, column, row);
			if (predecessor[index] != -1)
			{
				continue;
			}
			predecessor[index] =
			    static_cast<int>(cellIndex(width, cell[0], cell[1]));
			pending.push({column, row});
		}
	}
	if (predecessor[targetIndex] == -1)
	{
		return false;
	}
	for (std::size_t index = targetIndex;;
	     index = static_cast<std::size_t>(predecessor[index]))
	{
		retained[index] = true;
		if (index == startIndex)
		{
			return true;
		}
	}
}

// Accepts only layouts with the exact wall count, connectivity and unique
// signature.
bool fillSector(std::vector<game::CellType> &cells, const Sector &sector,
                int width, int wallCount, std::mt19937 &generator,
                const std::vector<std::uint64_t> &acceptedSignatures)
{
	std::vector<std::array<int, 2>> candidates;
	for (int row = sector.minimumRow; row <= sector.maximumRow; ++row)
	{
		for (int column = sector.minimumColumn; column <= sector.maximumColumn;
		     ++column)
		{
			if (isFree(cells, width, column, row))
			{
				candidates.push_back({column, row});
			}
		}
	}
	if (wallCount <= 0 ||
	    static_cast<std::size_t>(wallCount) >= candidates.size())
	{
		return false;
	}
	for (int attempt = 0; attempt < 1024; ++attempt)
	{
		for (const std::array<int, 2> &candidate : candidates)
		{
			cells[cellIndex(width, candidate[0], candidate[1])] =
			    game::CellType::Free;
		}
		static constexpr std::array<std::array<int, 2>, 4> baseDirections{{
		    {{1, 0}},
		    {{-1, 0}},
		    {{0, 1}},
		    {{0, -1}},
		}};
		std::vector<bool> retained(cells.size(), false);
		if (!appendRandomPath(cells, width, sector, sector.respawnColumn,
		                      sector.respawnRow, sector.variationColumn,
		                      sector.variationRow, generator, retained) ||
		    !appendRandomPath(cells, width, sector, sector.variationColumn,
		                      sector.variationRow, sector.centralAccessColumn,
		                      sector.centralAccessRow, generator, retained))
		{
			continue;
		}
		const auto retainedCount = [&retained]()
		{
			return static_cast<std::size_t>(
			    std::count(retained.begin(), retained.end(), true));
		};
		if (retainedCount() >
		    candidates.size() - static_cast<std::size_t>(wallCount))
		{
			continue;
		}
		while (retainedCount() <
		       candidates.size() - static_cast<std::size_t>(wallCount))
		{
			std::vector<std::array<int, 2>> frontier;
			for (const std::array<int, 2> &candidate : candidates)
			{
				const std::size_t index =
				    cellIndex(width, candidate[0], candidate[1]);
				if (retained[index])
				{
					continue;
				}
				for (const auto &direction : baseDirections)
				{
					const int column = candidate[0] + direction[0];
					const int row = candidate[1] + direction[1];
					if (isInside(sector, column, row) &&
					    retained[cellIndex(width, column, row)])
					{
						frontier.push_back(candidate);
						break;
					}
				}
			}
			if (frontier.empty())
			{
				break;
			}
			std::shuffle(frontier.begin(), frontier.end(), generator);
			const std::array<int, 2> selected = frontier.front();
			retained[cellIndex(width, selected[0], selected[1])] = true;
		}
		if (retainedCount() !=
		    candidates.size() - static_cast<std::size_t>(wallCount))
		{
			continue;
		}
		for (const std::array<int, 2> &candidate : candidates)
		{
			if (!retained[cellIndex(width, candidate[0], candidate[1])])
			{
				cells[cellIndex(width, candidate[0], candidate[1])] =
				    game::CellType::DestructibleWall;
			}
		}
		if (isSectorConnected(cells, width, sector) &&
		    countCells(cells, width, sector,
		               game::CellType::DestructibleWall) ==
		        static_cast<std::size_t>(wallCount) &&
		    std::find(acceptedSignatures.begin(), acceptedSignatures.end(),
		              wallSignature(cells, width, sector)) ==
		        acceptedSignatures.end())
		{
			return true;
		}
	}
	for (const std::array<int, 2> &candidate : candidates)
		cells[cellIndex(width, candidate[0], candidate[1])] =
		    game::CellType::Free;
	return false;
}

std::size_t countCells(const std::vector<game::CellType> &cells, int width,
                       const Sector &sector, game::CellType type)
{
	std::size_t count = 0;
	for (int row = sector.minimumRow; row <= sector.maximumRow; ++row)
	{
		for (int column = sector.minimumColumn; column <= sector.maximumColumn;
		     ++column)
		{
			if (cells[cellIndex(width, column, row)] == type)
			{
				++count;
			}
		}
	}
	return count;
}

std::array<int, 2> chooseVariationCell(const std::vector<game::CellType> &cells,
                                       int width, const Sector &sector,
                                       std::size_t freeCellTarget,
                                       bool preserveBaseline,
                                       std::mt19937 &generator)
{
	if (preserveBaseline)
		return {sector.variationColumn, sector.variationRow};
	std::vector<std::array<int, 2>> candidates;
	for (int row = sector.minimumRow; row <= sector.maximumRow; ++row)
	{
		for (int column = sector.minimumColumn; column <= sector.maximumColumn;
		     ++column)
		{
			if (!isFree(cells, width, column, row))
				continue;
			const std::size_t pathUpperBound = static_cast<std::size_t>(
			    std::abs(column - sector.respawnColumn) +
			    std::abs(row - sector.respawnRow) +
			    std::abs(column - sector.centralAccessColumn) +
			    std::abs(row - sector.centralAccessRow) + 1);
			if (pathUpperBound <= freeCellTarget)
				candidates.push_back({column, row});
		}
	}
	if (candidates.empty())
		return {sector.respawnColumn, sector.respawnRow};
	std::shuffle(candidates.begin(), candidates.end(), generator);
	return candidates.front();
}

std::array<int, 2>
chooseCentralAccessCell(const std::vector<game::CellType> &cells, int width,
                        const Sector &sector, std::size_t freeCellTarget,
                        bool preserveBaseline, std::mt19937 &generator)
{
	if (preserveBaseline)
		return {sector.centralAccessColumn, sector.centralAccessRow};
	const bool leftSector = sector.respawnColumn == sector.minimumColumn;
	const bool topSector = sector.respawnRow == sector.minimumRow;
	const int facingColumn =
	    leftSector ? sector.maximumColumn : sector.minimumColumn;
	const int facingRow = topSector ? sector.maximumRow : sector.minimumRow;
	std::vector<std::array<int, 2>> candidates;
	for (int row = sector.minimumRow; row <= sector.maximumRow; ++row)
		if (isFree(cells, width, facingColumn, row) &&
		    static_cast<std::size_t>(
		        std::abs(facingColumn - sector.respawnColumn) +
		        std::abs(row - sector.respawnRow) + 1) <= freeCellTarget)
			candidates.push_back({facingColumn, row});
	for (int column = sector.minimumColumn; column <= sector.maximumColumn;
	     ++column)
	{
		if (!isFree(cells, width, column, facingRow) ||
		    static_cast<std::size_t>(std::abs(column - sector.respawnColumn) +
		                             std::abs(facingRow - sector.respawnRow) +
		                             1) > freeCellTarget ||
		    (column == facingColumn && facingRow >= sector.minimumRow &&
		     facingRow <= sector.maximumRow))
			continue;
		candidates.push_back({column, facingRow});
	}
	if (candidates.empty())
		return {sector.centralAccessColumn, sector.centralAccessRow};
	std::shuffle(candidates.begin(), candidates.end(), generator);
	return candidates.front();
}
} // namespace

namespace game
{
PrototypeArenaLayout
PrototypeArenaGenerator::generate(int width, int height,
                                  std::uint32_t generationSeed)
{
	constexpr float cellSize = 0.25F;
	std::vector<CellType> cells(static_cast<std::size_t>(width * height),
	                            CellType::Free);
	for (int row = 0; row < height; ++row)
	{
		for (int column = 0; column < width; ++column)
		{
			if (column == 0 || column == width - 1 || row == 0 ||
			    row == height - 1 || (row % 2 == 0 && column % 2 == 0))
			{
				cells[cellIndex(width, column, row)] =
				    CellType::IndestructibleWall;
			}
		}
	}
	const int centerColumn = width / 2;
	const int centerRow = height / 2;
	for (int column = 1; column < width - 1; ++column)
	{
		if (column != centerColumn)
			cells[cellIndex(width, column, centerRow)] =
			    CellType::DestructibleWall;
	}
	for (int row = 1; row < height - 1; ++row)
	{
		if (row != centerRow)
			cells[cellIndex(width, centerColumn, row)] =
			    CellType::DestructibleWall;
	}
	cells[cellIndex(width, centerColumn, centerRow)] = CellType::Free;
	std::array<Sector, 4> sectors{{
	    {1, centerColumn - 1, 1, centerRow - 1, 1, 1, centerColumn - 1, 1, 3,
		 3},
	    {centerColumn + 1, width - 2, 1, centerRow - 1, width - 2, 1,
		 centerColumn + 1, 1, width - 3, 3},
	    {1, centerColumn - 1, centerRow + 1, height - 2, 1, height - 2,
		 centerColumn - 1, height - 2, 3, height - 4},
	    {centerColumn + 1, width - 2, centerRow + 1, height - 2, width - 2,
		 height - 2, centerColumn + 1, height - 2, width - 3, height - 4},
	}};
	std::mt19937 generator(generationSeed);
	const bool preserveBaseline =
	    width == 15 && height == 15 && generationSeed == seed;
	std::vector<std::uint64_t> acceptedSignatures;
	acceptedSignatures.reserve(sectors.size());
	for (Sector &sector : sectors)
	{
		const std::size_t eligible =
		    countCells(cells, width, sector, CellType::Free);
		const int densityWallCount = static_cast<int>(std::lround(
		    static_cast<double>(eligible) * kBaselineWallsPerSector /
		    kBaselineEligibleCellsPerSector));
		// Cap density so a narrow sector can retain a connected path spanning
		// both dimensions. Otherwise some inner cells can never vary by seed.
		const std::size_t minimumFreeCellCount = static_cast<std::size_t>(
		    (sector.maximumColumn - sector.minimumColumn + 1) +
		    (sector.maximumRow - sector.minimumRow + 1) - 1);
		const int wallCount =
		    std::min(densityWallCount,
			         static_cast<int>(eligible - minimumFreeCellCount));
		const std::size_t freeCellTarget =
		    eligible - static_cast<std::size_t>(wallCount);
		bool generated = false;
		const int planningAttempts = preserveBaseline ? 1 : 128;
		for (int attempt = 0; attempt < planningAttempts && !generated;
		     ++attempt)
		{
			const auto access =
			    chooseCentralAccessCell(cells, width, sector, freeCellTarget,
				                        preserveBaseline, generator);
			sector.centralAccessColumn = access[0];
			sector.centralAccessRow = access[1];
			const auto variation =
			    chooseVariationCell(cells, width, sector, freeCellTarget,
				                    preserveBaseline, generator);
			sector.variationColumn = variation[0];
			sector.variationRow = variation[1];
			generated = fillSector(cells, sector, width, wallCount, generator,
			                       acceptedSignatures);
		}
		if (!generated)
		{
			throw std::logic_error(
			    "Arena generation failed to preserve sector connectivity");
		}
		acceptedSignatures.push_back(wallSignature(cells, width, sector));
	}
	const std::size_t crossWalls =
	    static_cast<std::size_t>((width - 3) + (height - 3));
	std::fprintf(stderr,
	             "Arena generation: dimensions=%dx%d seed=0x%08X "
	             "cross_walls=%zu center=(%d,%d) free "
	             "respawns=(1,1),(%d,1),(1,%d),(%d,%d).\n",
	             width, height, generationSeed, crossWalls, centerColumn,
	             centerRow, width - 2, height - 2, width - 2, height - 2);
	for (std::size_t index = 0; index < sectors.size(); ++index)
	{
		const Sector &sector = sectors[index];
		std::fprintf(
		    stderr,
		    "Arena sector: index=%zu respawn=(%d,%d) access=(%d,%d) "
		    "variation=(%d,%d) eligible=%zu destructible=%zu free=%zu "
		    "bfs=pass "
		    "signature=0x%09llX.\n",
		    index, sector.respawnColumn, sector.respawnRow,
		    sector.centralAccessColumn, sector.centralAccessRow,
		    sector.variationColumn, sector.variationRow,
		    countCells(cells, width, sector, CellType::DestructibleWall) +
		        countCells(cells, width, sector, CellType::Free),
		    countCells(cells, width, sector, CellType::DestructibleWall),
		    countCells(cells, width, sector, CellType::Free),
		    static_cast<unsigned long long>(acceptedSignatures[index]));
	}
	const float originX = -static_cast<float>(width) * cellSize * 0.5F;
	const float originY = -static_cast<float>(height) * cellSize * 0.5F;
	return {width, height, cellSize, originX, originY, std::move(cells)};
}
} // namespace game
