/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   EnemyNavigation.cpp                              :+:      :+:    :+:   :+:      */
/*                                                    +:+ +:+         +:+     +:+    */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           +#+#+#+#+#+   */
/*   Created: 2026/10/07          by flaviobc      #+#    #+#             */
/*   Updated: 2026/10/07          by flaviobc     ###   ########.fr       */
/*                                                                                    */
/* ******************************************************************************** */

#include "ai/EnemyNavigation.hpp"

#include "world/Arena.hpp"

#include <algorithm>
#include <cmath>

namespace
{
constexpr std::array<game::ExplosionCell, 4> kCardinalDirections{
    {{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
}

namespace game
{
EnemyNavigation::EnemyNavigation(const Arena &arena, const EnemyBrain &brain,
                                 double ownFuseSeconds,
                                 const GameplayTuningMultipliers &weights)
    : arena_(arena), brain_(brain), ownFuseSeconds_(ownFuseSeconds),
      weights_(weights)
{
}

bool EnemyNavigation::contains(ExplosionCell cell) const
{
	return cell.column > 0 && cell.column < arena_.width() - 1 &&
	       cell.row > 0 && cell.row < arena_.height() - 1;
}

bool EnemyNavigation::hasLineOfSight(ExplosionCell source, ExplosionCell target,
                                     int range) const
{
	const int columnDistance = std::abs(target.column - source.column);
	const int rowDistance = std::abs(target.row - source.row);
	if ((columnDistance == 0) == (rowDistance == 0) ||
	    columnDistance + rowDistance > range)
		return false;
	const int columnStep =
	    (target.column > source.column) - (target.column < source.column);
	const int rowStep = (target.row > source.row) - (target.row < source.row);
	for (int step = 1; step <= columnDistance + rowDistance; ++step)
	{
		const int column = source.column + columnStep * step;
		const int row = source.row + rowStep * step;
		if (arena_.isOccupied(column, row) ||
		    brain_.hasBombAt(arena_, column, row))
			return false;
	}
	return true;
}

bool EnemyNavigation::isCellDangerous(const EnemyState &enemy, float x, float y,
                                      bool ignoreOwnBomb) const
{
	if (brain_.intersectsExplosion(x, y, enemy.actor.collisionHalfExtent))
		return true;
	for (const ObservedBomb &bomb : brain_.bombs)
	{
		if (ignoreOwnBomb && enemy.brain.escapingBombId == bomb.id)
			continue;
		if (brain_.bombThreatens(arena_, x, y, enemy.actor.collisionHalfExtent,
		                         bomb))
			return true;
	}
	return false;
}

bool EnemyNavigation::routeStepClear(const EnemyState &enemy, float fromX,
                                     float fromY, float toX, float toY) const
{
	// Cardinal route segments are sampled in quarter-cell increments so actors
	// cannot tunnel through a blocker between two cell centers.
	const float distance = std::abs(toX - fromX) + std::abs(toY - fromY);
	const int steps = std::max(
	    1, static_cast<int>(std::ceil(distance / (arena_.cellSize() * 0.25F))));
	const auto &player = brain_.actors.front();
	bool maySeparateFromPlayer =
	    overlapsActor(fromX, fromY, enemy.actor.collisionHalfExtent, true);
	float previousPlayerDistanceSquared =
	    (fromX - player.x) * (fromX - player.x) +
	    (fromY - player.y) * (fromY - player.y);
	for (int step = 1; step <= steps; ++step)
	{
		const float t = static_cast<float>(step) / static_cast<float>(steps);
		const float x = fromX + (toX - fromX) * t;
		const float y = fromY + (toY - fromY) * t;
		const bool overlapsPlayer =
		    overlapsActor(x, y, enemy.actor.collisionHalfExtent, true);
		if (overlapsPlayer)
		{
			const float playerDistanceSquared =
			    (x - player.x) * (x - player.x) +
			    (y - player.y) * (y - player.y);
			if (!maySeparateFromPlayer ||
			    playerDistanceSquared <= previousPlayerDistanceSquared)
				return false;
			previousPlayerDistanceSquared = playerDistanceSquared;
		}
		else
			maySeparateFromPlayer = false;
		if (terrainOrBombBlocked(enemy, x, y) ||
		    brain_.intersectsExplosion(x, y, enemy.actor.collisionHalfExtent) ||
		    overlapsActor(x, y, enemy.actor.collisionHalfExtent, false))
			return false;
	}
	return true;
}

bool EnemyNavigation::terrainOrBombBlocked(const EnemyState &enemy, float x,
                                           float y) const
{
	const float half = enemy.actor.collisionHalfExtent;
	if (arena_.intersectsOccupied(x - half, y - half, x + half, y + half))
		return true;
	for (const auto &bomb : brain_.bombs)
	{
		const bool mayLeave =
		    std::any_of(enemy.actor.bombPassThrough.begin(),
			            enemy.actor.bombPassThrough.end(), [&](const auto &pass)
			            { return pass.x == bomb.x && pass.y == bomb.y; });
		if (!mayLeave &&
		    std::abs(x - bomb.x) <= half + arena_.cellSize() * 0.5F &&
		    std::abs(y - bomb.y) <= half + arena_.cellSize() * 0.5F)
			return true;
	}
	return false;
}

bool EnemyNavigation::overlapsActor(float x, float y, float halfExtent,
                                    bool playerOnly) const
{
	return std::any_of(
	    brain_.actors.begin(), brain_.actors.end(),
	    [&](const auto &actor)
	    {
		    return (actor.identity == 0) == playerOnly &&
			       std::abs(x - actor.x) <= halfExtent + actor.halfExtent &&
			       std::abs(y - actor.y) <= halfExtent + actor.halfExtent;
	    });
}

bool EnemyNavigation::actorBlocked(const EnemyState &enemy, float x,
                                   float y) const
{
	return overlapsActor(x, y, enemy.actor.collisionHalfExtent, false) ||
	       overlapsActor(x, y, enemy.actor.collisionHalfExtent, true);
}

bool EnemyNavigation::movementStepBlocked(const EnemyState &enemy, float x,
                                          float y,
                                          bool allowPlayerOverlap) const
{
	const auto &player_ = brain_.actors.front();
	const bool overlapsPlayer =
	    overlapsActor(x, y, enemy.actor.collisionHalfExtent, true);
	const bool currentlyOverlapsPlayer = overlapsActor(
	    enemy.actor.x, enemy.actor.y, enemy.actor.collisionHalfExtent, true);
	const float currentDistanceSquared =
	    (enemy.actor.x - player_.x) * (enemy.actor.x - player_.x) +
	    (enemy.actor.y - player_.y) * (enemy.actor.y - player_.y);
	const float candidateDistanceSquared =
	    (x - player_.x) * (x - player_.x) + (y - player_.y) * (y - player_.y);
	const bool separatesExistingOverlap =
	    currentlyOverlapsPlayer &&
	    candidateDistanceSquared > currentDistanceSquared;
	return terrainOrBombBlocked(enemy, x, y) ||
	       overlapsActor(x, y, enemy.actor.collisionHalfExtent, false) ||
	       brain_.intersectsExplosion(x, y, enemy.actor.collisionHalfExtent) ||
	       (!allowPlayerOverlap && !separatesExistingOverlap && overlapsPlayer);
}

std::optional<EnemyNavigation::Route>
EnemyNavigation::findEscapeRoute(const EnemyState &enemy, bool placingBomb,
                                 float actorSpeed,
                                 bool preferAdversarySeparation) const
{
	const ExplosionCell startCell{arena_.columnAt(enemy.actor.x),
	                              arena_.rowAt(enemy.actor.y)};
	const int width = arena_.width();
	const int start = startCell.row * width + startCell.column;
	const float startX = arena_.cellCenterX(startCell.column);
	const float startY = arena_.cellCenterY(startCell.row);
	const ObservedBomb proposed{
	    0, startX, startY, brain_.timeSeconds,
	    KnownBombTiming{enemy.actor.explosionRange, ownFuseSeconds_}};
	const bool alreadyThreatened =
	    isCellDangerous(enemy, enemy.actor.x, enemy.actor.y);
	double deadline = ownFuseSeconds_;
	for (const ObservedBomb &bomb : brain_.bombs)
		if (brain_.bombThreatens(arena_, enemy.actor.x, enemy.actor.y,
		                         enemy.actor.collisionHalfExtent, bomb))
			deadline = std::min(
			    deadline, bomb.own ? bomb.own->remainingSeconds
				                   : brain_.unknownDeadline(bomb, weights_));
	if (!contains(startCell))
		return std::nullopt;
	const bool startCenterClear =
	    routeStepClear(enemy, enemy.actor.x, enemy.actor.y, startX, startY);

	// BFS stores cells as row-major linear indices. parent is both the visited
	// set and the predecessor chain used to reconstruct the chosen route.
	std::vector<int> parent(static_cast<std::size_t>(width * arena_.height()),
	                        -1);
	std::vector<int> frontier{start};
	parent[start] = start;
	const auto directions = forwardBiasedDirections(enemy);
	std::optional<Route> bestEscape;
	double bestCost = 0.0;
	for (std::size_t head = 0; head < frontier.size(); ++head)
	{
		const int current = frontier[head];
		const ExplosionCell cell{current % width, current / width};
		const float x = arena_.cellCenterX(cell.column);
		const float y = arena_.cellCenterY(cell.row);
		if (!isCellDangerous(enemy, x, y) &&
		    (!placingBomb ||
		     !brain_.bombThreatens(arena_, x, y,
		                           enemy.actor.collisionHalfExtent, proposed)))
		{
			Route route = reconstructRoute(parent, start, current);
			if (!startCenterClear)
				route.erase(route.begin());
			float distance = 0.0F;
			float routeX = enemy.actor.x;
			float routeY = enemy.actor.y;
			for (const ExplosionCell routeCell : route)
			{
				const float nextX = arena_.cellCenterX(routeCell.column);
				const float nextY = arena_.cellCenterY(routeCell.row);
				distance += std::abs(nextX - routeX) + std::abs(nextY - routeY);
				routeX = nextX;
				routeY = nextY;
			}
			// When already threatened, an uncertain deadline must not paralyse
			// escape. Placement still requires a strictly feasible known
			// deadline.
			if (!route.empty() &&
			    ((!placingBomb && alreadyThreatened) ||
			     distance + enemy.actor.collisionHalfExtent <
			         actorSpeed * static_cast<float>(deadline)))
			{
				int safeExits = 0;
				for (auto direction : kCardinalDirections)
				{
					const float exitX =
					    arena_.cellCenterX(cell.column + direction.column);
					const float exitY =
					    arena_.cellCenterY(cell.row + direction.row);
					if (!terrainOrBombBlocked(enemy, exitX, exitY) &&
					    !isCellDangerous(enemy, exitX, exitY) &&
					    (!placingBomb ||
					     !brain_.bombThreatens(arena_, exitX, exitY,
					                           enemy.actor.collisionHalfExtent,
					                           proposed)))
						++safeExits;
				}
				// Older/less certain bombs increase time pressure. Safe exits
				// are a secondary preference; all candidates obey physical
				// constraints.
				double adversarySeparation = 0.0;
				if (preferAdversarySeparation && !brain_.actors.empty())
				{
					adversarySeparation = arena_.width() + arena_.height();
					for (const ObservedActor &actor : brain_.actors)
						adversarySeparation = std::min(
						    adversarySeparation,
						    static_cast<double>(
						        std::abs(cell.column -
								         arena_.columnAt(actor.x)) +
						        std::abs(cell.row - arena_.rowAt(actor.y))));
				}
				const double cost =
				    distance / std::max(0.001, actorSpeed * deadline) +
				    0.1 * std::min(100.0, weights_.aiTrapAvoidance) /
				        (safeExits + 1.0) -
				    0.2 * adversarySeparation;
				if (!bestEscape || cost < bestCost)
				{
					bestCost = cost;
					bestEscape = std::move(route);
				}
			}
		}
		for (const ExplosionCell direction : directions)
		{
			const ExplosionCell nextCell{cell.column + direction.column,
			                             cell.row + direction.row};
			if (!contains(nextCell))
				continue;
			const int next = nextCell.row * width + nextCell.column;
			const float nextX = arena_.cellCenterX(nextCell.column);
			const float nextY = arena_.cellCenterY(nextCell.row);
			const float fromX =
			    current == start && !startCenterClear ? enemy.actor.x : x;
			const float fromY =
			    current == start && !startCenterClear ? enemy.actor.y : y;
			if (parent[next] != -1 ||
			    !routeStepClear(enemy, fromX, fromY, nextX, nextY))
				continue;
			// A threatened actor may cross forecast cells to escape. A plan
			// that starts safe must not enter another bomb's future blast.
			if (!alreadyThreatened &&
			    isCellDangerous(enemy, nextX, nextY, true))
				continue;
			parent[next] = current;
			frontier.push_back(next);
		}
	}
	return bestEscape;
}

std::optional<EnemyNavigation::Route> EnemyNavigation::findRouteToAnyCandidate(
    const EnemyState &enemy, std::span<const ExplosionCell> candidates) const
{
	const int width = arena_.width();
	const ExplosionCell startCell{arena_.columnAt(enemy.actor.x),
	                              arena_.rowAt(enemy.actor.y)};
	const float startX = arena_.cellCenterX(startCell.column);
	const float startY = arena_.cellCenterY(startCell.row);
	if (!contains(startCell) || isCellDangerous(enemy, startX, startY) ||
	    !routeStepClear(enemy, enemy.actor.x, enemy.actor.y, startX, startY))
		return std::nullopt;
	const int start = startCell.row * width + startCell.column;
	std::vector<int> parent(static_cast<std::size_t>(width * arena_.height()),
	                        -1);
	std::vector<int> frontier{start};
	parent[start] = start;
	const auto directions = forwardBiasedDirections(enemy);
	for (std::size_t head = 0; head < frontier.size(); ++head)
	{
		const int current = frontier[head];
		const ExplosionCell cell{current % width, current / width};
		if (std::any_of(candidates.begin(), candidates.end(),
		                [cell](ExplosionCell candidate)
		                {
			                return candidate.column == cell.column &&
							       candidate.row == cell.row;
		                }))
			return reconstructRoute(parent, start, current);
		for (const ExplosionCell direction : directions)
		{
			const ExplosionCell next{cell.column + direction.column,
			                         cell.row + direction.row};
			if (!contains(next))
				continue;
			const int node = next.row * width + next.column;
			const float x = arena_.cellCenterX(next.column);
			const float y = arena_.cellCenterY(next.row);
			if (parent[node] != -1 || isCellDangerous(enemy, x, y) ||
			    !routeStepClear(enemy, arena_.cellCenterX(cell.column),
			                    arena_.cellCenterY(cell.row), x, y))
				continue;
			parent[node] = current;
			frontier.push_back(node);
		}
	}
	return std::nullopt;
}

std::optional<EnemyNavigation::Route>
EnemyNavigation::findRouteToCell(const EnemyState &enemy,
                                 ExplosionCell goal) const
{
	const int width = arena_.width();
	const ExplosionCell start{arena_.columnAt(enemy.actor.x),
	                          arena_.rowAt(enemy.actor.y)};
	if (!contains(start) || !contains(goal))
		return std::nullopt;
	const int startNode = start.row * width + start.column;
	const int goalNode = goal.row * width + goal.column;
	std::vector<int> parent(static_cast<std::size_t>(width * arena_.height()),
	                        -1);
	std::vector<int> frontier{startNode};
	parent[startNode] = startNode;
	const auto directions = forwardBiasedDirections(enemy);
	for (std::size_t head = 0; head < frontier.size(); ++head)
	{
		const int current = frontier[head];
		if (current == goalNode)
			return reconstructRoute(parent, startNode, current);
		const ExplosionCell cell{current % width, current / width};
		for (const ExplosionCell direction : directions)
		{
			const ExplosionCell next{cell.column + direction.column,
			                         cell.row + direction.row};
			if (!contains(next))
				continue;
			const int node = next.row * width + next.column;
			const float x = arena_.cellCenterX(next.column);
			const float y = arena_.cellCenterY(next.row);
			if (parent[node] != -1 || isCellDangerous(enemy, x, y) ||
			    !routeStepClear(enemy, arena_.cellCenterX(cell.column),
			                    arena_.cellCenterY(cell.row), x, y))
				continue;
			parent[node] = current;
			frontier.push_back(node);
		}
	}
	return std::nullopt;
}

std::optional<EnemyNavigation::Route>
EnemyNavigation::findApproachRoute(const EnemyState &enemy,
                                   ExplosionCell target) const
{
	const int width = arena_.width();
	const ExplosionCell start{arena_.columnAt(enemy.actor.x),
	                          arena_.rowAt(enemy.actor.y)};
	const int startNode = start.row * width + start.column;
	std::vector<int> parent(static_cast<std::size_t>(width * arena_.height()),
	                        -1);
	std::vector<int> frontier{startNode};
	parent[startNode] = startNode;
	for (std::size_t head = 0; head < frontier.size(); ++head)
	{
		const int current = frontier[head];
		const ExplosionCell cell{current % width, current / width};
		if (current != startNode &&
		    hasLineOfSight(cell, target, enemy.actor.explosionRange))
			return reconstructRoute(parent, startNode, current);
		for (const ExplosionCell direction : kCardinalDirections)
		{
			const ExplosionCell next{cell.column + direction.column,
			                         cell.row + direction.row};
			if (!contains(next))
				continue;
			const int node = next.row * width + next.column;
			const float x = arena_.cellCenterX(next.column);
			const float y = arena_.cellCenterY(next.row);
			if (parent[node] != -1 || isCellDangerous(enemy, x, y) ||
			    !routeStepClear(enemy, arena_.cellCenterX(cell.column),
			                    arena_.cellCenterY(cell.row), x, y))
				continue;
			parent[node] = current;
			frontier.push_back(node);
		}
	}
	return std::nullopt;
}

std::optional<EnemyNavigation::Route>
EnemyNavigation::findWallRoute(const EnemyState &enemy, ExplosionCell target,
                               ExplosionCell &wall) const
{
	const int width = arena_.width();
	const ExplosionCell start{arena_.columnAt(enemy.actor.x),
	                          arena_.rowAt(enemy.actor.y)};
	const int startNode = start.row * width + start.column;
	std::vector<int> parent(static_cast<std::size_t>(width * arena_.height()),
	                        -1);
	std::vector<int> depth(parent.size(), 0);
	std::vector<int> frontier{startNode};
	parent[startNode] = startNode;
	int bestNode = -1;
	int bestScore = arena_.width() * arena_.height() * 2;
	for (std::size_t head = 0; head < frontier.size(); ++head)
	{
		const int current = frontier[head];
		const ExplosionCell cell{current % width, current / width};
		for (const ExplosionCell direction : kCardinalDirections)
		{
			const ExplosionCell next{cell.column + direction.column,
			                         cell.row + direction.row};
			if (!contains(next))
				continue;
			if (arena_.cellType(next.column, next.row) ==
			    CellType::DestructibleWall)
			{
				const int score = depth[current] +
				                  std::abs(next.column - target.column) +
				                  std::abs(next.row - target.row);
				if (score < bestScore)
				{
					bestScore = score;
					bestNode = current;
					wall = next;
				}
				continue;
			}
			const int node = next.row * width + next.column;
			const float x = arena_.cellCenterX(next.column);
			const float y = arena_.cellCenterY(next.row);
			if (parent[node] != -1 || isCellDangerous(enemy, x, y) ||
			    !routeStepClear(enemy, arena_.cellCenterX(cell.column),
			                    arena_.cellCenterY(cell.row), x, y))
				continue;
			parent[node] = current;
			depth[node] = depth[current] + 1;
			frontier.push_back(node);
		}
	}
	if (bestNode == -1)
		return std::nullopt;
	return reconstructRoute(parent, startNode, bestNode);
}

std::optional<ExplosionCell>
EnemyNavigation::selectPatrolNeighbor(const EnemyState &enemy,
                                      std::span<const unsigned> visits) const
{
	const int column = arena_.columnAt(enemy.actor.x);
	const int row = arena_.rowAt(enemy.actor.y);
	std::optional<ExplosionCell> selected;
	unsigned fewestVisits = 0;
	for (const ExplosionCell direction : forwardBiasedDirections(enemy))
	{
		const ExplosionCell next{column + direction.column,
		                         row + direction.row};
		const float x = arena_.cellCenterX(next.column);
		const float y = arena_.cellCenterY(next.row);
		if (!contains(next) || terrainOrBombBlocked(enemy, x, y) ||
		    actorBlocked(enemy, x, y) || isCellDangerous(enemy, x, y))
			continue;
		const unsigned visitCount = visits[static_cast<std::size_t>(
		    next.row * arena_.width() + next.column)];
		if (!selected || visitCount < fewestVisits)
		{
			selected = next;
			fewestVisits = visitCount;
		}
	}
	return selected;
}

std::array<ExplosionCell, 4>
EnemyNavigation::forwardBiasedDirections(const EnemyState &enemy) const
{
	// Forward, left, right, back is the historical deterministic tie-break.
	return {enemy.direction,
	        {-enemy.direction.row, enemy.direction.column},
	        {enemy.direction.row, -enemy.direction.column},
	        {-enemy.direction.column, -enemy.direction.row}};
}

EnemyNavigation::Route
EnemyNavigation::reconstructRoute(const std::vector<int> &parent, int start,
                                  int destination) const
{
	Route route;
	for (int node = destination; node != start; node = parent[node])
		route.push_back({node % arena_.width(), node / arena_.width()});
	route.push_back({start % arena_.width(), start / arena_.width()});
	std::reverse(route.begin(), route.end());
	return route;
}
} // namespace game
