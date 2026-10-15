/* ********************************************************************************
 */
/*                                                                                    */
/*                                                      :::      ::::::::   :::
 */
/*   BombSystem.cpp                                   :+:      :+:    :+:   :+:
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

#include "combat/BombSystem.hpp"

#include "world/Arena.hpp"

#include <algorithm>
#include <cstdio>
#include <utility>

namespace game
{
BombSystem::BombSystem(double fuseDurationSeconds,
                       double explosionDurationSeconds)
    : fuseDurationSeconds_(fuseDurationSeconds),
      explosionDurationSeconds_(explosionDurationSeconds)
{
}

double BombSystem::fuseDurationSeconds() const
{
	return fuseDurationSeconds_;
}

double BombSystem::explosionDurationSeconds() const
{
	return explosionDurationSeconds_;
}

std::optional<std::size_t>
BombSystem::placeBomb(const Arena &arena, ActorKind ownerKind,
                      std::size_t ownerIndex, ActorState &owner,
                      std::span<ActorState *> overlappingActors)
{
	const int column = arena.columnAt(owner.x);
	const int row = arena.rowAt(owner.y);
	if (hasBombAt(arena, column, row))
		return std::nullopt;
	bombs_.push_back({Bomb(arena.cellCenterX(column), arena.cellCenterY(row),
	                       fuseDurationSeconds_),
	                  ownerKind, ownerIndex, owner.explosionRange,
	                  nextBombId_++});
	++dangerRevision_;
	const ActiveBombState &activeBomb = bombs_.back();
	grantPassThrough(arena, activeBomb.bomb, overlappingActors);
	std::fprintf(stderr,
	             "Bomb placed: owner=%s index=%zu actor=(%.3f, %.3f) "
	             "cell=(%d,%d) bomb=(%.3f, %.3f) fuse=%.1f s range=%d "
	             "active=%zu/%zu.\n",
	             ownerKind == ActorKind::Player ? "player" : "enemy",
	             ownerIndex, owner.x, owner.y, column, row, activeBomb.bomb.x(),
	             activeBomb.bomb.y(), activeBomb.bomb.remainingSeconds(),
	             owner.explosionRange, activeBombCount(ownerKind, ownerIndex),
	             owner.maximumActiveBombs);
	return activeBomb.id;
}

void BombSystem::updateBombs(double deltaSeconds, Arena &arena,
                             std::span<ActorState *> actors,
                             BombSystemListener &listener)
{
	for (auto bomb = bombs_.begin(); bomb != bombs_.end();)
	{
		const double previousRemainingSeconds = bomb->bomb.remainingSeconds();
		if (!bomb->bomb.update(deltaSeconds))
		{
			if (previousRemainingSeconds > 1.0 &&
			    bomb->bomb.remainingSeconds() <= 1.0)
			{
				std::fprintf(
				    stderr, "Bomb fuse: position=(%.3f, %.3f) 1 s remaining.\n",
				    bomb->bomb.x(), bomb->bomb.y());
			}
			++bomb;
			continue;
		}
		std::fprintf(stderr, "Bomb expired: position=(%.3f, %.3f).\n",
		             bomb->bomb.x(), bomb->bomb.y());
		removePassThrough(bomb->bomb, actors);
		queueDetonation(*bomb, false);
		bomb = bombs_.erase(bomb);
	}
	processPendingDetonations(arena, actors, listener);
}

ExplosionDamageResult
BombSystem::updateExplosions(double deltaSeconds,
                             std::span<const ExplosionDamageTarget> targets,
                             bool damageEnabled)
{
	ExplosionDamageResult result;
	for (auto explosion = explosions_.begin(); explosion != explosions_.end();)
	{
		if (damageEnabled)
			ExplosionDamage::collectHits(explosion->projection, targets,
			                             explosion->damagedActors, result);
		if (!explosion->explosion.update(deltaSeconds))
		{
			++explosion;
			continue;
		}
		std::fputs("Explosion finished.\n", stderr);
		++dangerRevision_;
		explosion = explosions_.erase(explosion);
	}
	rebuildExplosionProjection();
	return result;
}

void BombSystem::reset()
{
	bombs_.clear();
	pendingDetonations_.clear();
	explosions_.clear();
	explosionProjection_.clear();
	nextBombId_ = 1;
	dangerRevision_ = 0;
}

void BombSystem::updatePassThrough(const Arena &arena, ActorState &actor) const
{
	actor.bombPassThrough.erase(
	    std::remove_if(
	        actor.bombPassThrough.begin(), actor.bombPassThrough.end(),
	        [this, &arena, &actor](const BombPassThrough &passThrough)
	        {
		        const Bomb bomb(passThrough.x, passThrough.y, 0.0);
		        return !intersectsBombCell(arena, actor.x, actor.y,
				                           actor.collisionHalfExtent, bomb);
	        }),
	    actor.bombPassThrough.end());
}

bool BombSystem::blocksActor(const Arena &arena, float centerX, float centerY,
                             const ActorState &actor) const
{
	return std::any_of(
	    bombs_.begin(), bombs_.end(),
	    [this, &arena, centerX, centerY, &actor](const ActiveBombState &active)
	    {
		    return !actorCanPassThrough(actor, active.bomb) &&
			       intersectsBombCell(arena, centerX, centerY,
			                          actor.collisionHalfExtent, active.bomb);
	    });
}

bool BombSystem::intersectsBombCell(const Arena &arena, float centerX,
                                    float centerY, float halfExtent,
                                    const Bomb &bomb) const
{
	const float cellHalfExtent = arena.cellSize() * 0.5F;
	return centerX + halfExtent >= bomb.x() - cellHalfExtent &&
	       centerX - halfExtent <= bomb.x() + cellHalfExtent &&
	       centerY + halfExtent >= bomb.y() - cellHalfExtent &&
	       centerY - halfExtent <= bomb.y() + cellHalfExtent;
}

bool BombSystem::intersectsActiveExplosion(float centerX, float centerY,
                                           float halfExtent) const
{
	const float minimumX = centerX - halfExtent;
	const float minimumY = centerY - halfExtent;
	const float maximumX = centerX + halfExtent;
	const float maximumY = centerY + halfExtent;
	return std::any_of(explosionProjection_.begin(), explosionProjection_.end(),
	                   [minimumX, minimumY, maximumX,
	                    maximumY](const ExplosionProjectionCell &cell)
	                   {
		                   return maximumX >= cell.x &&
						          minimumX <= cell.x + cell.width &&
						          maximumY >= cell.y &&
						          minimumY <= cell.y + cell.height;
	                   });
}

bool BombSystem::bombThreatens(const Arena &arena, float x, float y,
                               float halfExtent, const Bomb &bomb,
                               int range) const
{
	if (intersectsBombCell(arena, x, y, halfExtent, bomb))
		return true;
	const int column = arena.columnAt(bomb.x());
	const int row = arena.rowAt(bomb.y());
	constexpr int directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
	for (const auto &direction : directions)
	{
		for (int step = 1; step <= range; ++step)
		{
			const int nextColumn = column + direction[0] * step;
			const int nextRow = row + direction[1] * step;
			const CellType type = arena.cellType(nextColumn, nextRow);
			if (type == CellType::IndestructibleWall)
				break;
			const Bomb cell(arena.cellCenterX(nextColumn),
			                arena.cellCenterY(nextRow), 0.0);
			if (intersectsBombCell(arena, x, y, halfExtent, cell))
				return true;
			if (type == CellType::DestructibleWall)
				break;
		}
	}
	return false;
}

bool BombSystem::hasBombAt(const Arena &arena, int column, int row) const
{
	return std::any_of(bombs_.begin(), bombs_.end(),
	                   [&arena, column, row](const ActiveBombState &active)
	                   {
		                   return arena.columnAt(active.bomb.x()) == column &&
						          arena.rowAt(active.bomb.y()) == row;
	                   });
}

std::size_t BombSystem::activeBombCount(ActorKind ownerKind,
                                        std::size_t ownerIndex) const
{
	return static_cast<std::size_t>(std::count_if(
	    bombs_.begin(), bombs_.end(),
	    [ownerKind, ownerIndex](const ActiveBombState &bomb)
	    {
		    return bomb.ownerKind == ownerKind && bomb.ownerIndex == ownerIndex;
	    }));
}

bool BombSystem::bombCycleActive(std::size_t id) const
{
	const auto matches = [id](const ActiveBombState &bomb)
	{ return bomb.id == id; };
	return std::any_of(bombs_.begin(), bombs_.end(), matches) ||
	       std::any_of(pendingDetonations_.begin(), pendingDetonations_.end(),
	                   matches) ||
	       std::any_of(explosions_.begin(), explosions_.end(),
	                   [id](const ActiveExplosionState &explosion)
	                   { return explosion.bombId == id; });
}

bool BombSystem::hasActiveBombs() const
{
	return !bombs_.empty();
}

bool BombSystem::hasActiveExplosions() const
{
	return !explosions_.empty();
}

std::size_t BombSystem::dangerRevision() const
{
	return dangerRevision_;
}

std::span<const ActiveBombState> BombSystem::bombs() const
{
	return bombs_;
}

const std::vector<ExplosionProjectionCell> &
BombSystem::explosionProjection() const
{
	return explosionProjection_;
}

bool BombSystem::actorCanPassThrough(const ActorState &actor,
                                     const Bomb &bomb) const
{
	return std::any_of(
	    actor.bombPassThrough.begin(), actor.bombPassThrough.end(),
	    [&bomb](const BombPassThrough &passThrough)
	    { return passThrough.x == bomb.x() && passThrough.y == bomb.y(); });
}

void BombSystem::grantPassThrough(const Arena &arena, const Bomb &bomb,
                                  std::span<ActorState *> actors) const
{
	for (ActorState *actor : actors)
	{
		if (actor != nullptr &&
		    intersectsBombCell(arena, actor->x, actor->y,
		                       actor->collisionHalfExtent, bomb))
		{
			actor->bombPassThrough.push_back({bomb.x(), bomb.y()});
		}
	}
}

void BombSystem::removePassThrough(const Bomb &bomb,
                                   std::span<ActorState *> actors) const
{
	for (ActorState *actor : actors)
	{
		if (actor == nullptr)
			continue;
		actor->bombPassThrough.erase(
		    std::remove_if(actor->bombPassThrough.begin(),
			               actor->bombPassThrough.end(),
			               [&bomb](const BombPassThrough &passThrough)
			               {
			                   return passThrough.x == bomb.x() &&
							          passThrough.y == bomb.y();
		                   }),
		    actor->bombPassThrough.end());
	}
}

void BombSystem::queueDetonation(ActiveBombState bomb,
                                 bool triggeredByExplosion)
{
	if (triggeredByExplosion)
	{
		std::fprintf(stderr, "Bomb chain triggered: position=(%.3f, %.3f).\n",
		             bomb.bomb.x(), bomb.bomb.y());
	}
	pendingDetonations_.push_back(std::move(bomb));
}

void BombSystem::queueBombsAt(const Arena &arena, int column, int row,
                              std::span<ActorState *> actors)
{
	for (auto bomb = bombs_.begin(); bomb != bombs_.end();)
	{
		if (arena.columnAt(bomb->bomb.x()) != column ||
		    arena.rowAt(bomb->bomb.y()) != row)
		{
			++bomb;
			continue;
		}
		removePassThrough(bomb->bomb, actors);
		queueDetonation(*bomb, true);
		bomb = bombs_.erase(bomb);
	}
}

void BombSystem::processPendingDetonations(Arena &arena,
                                           std::span<ActorState *> actors,
                                           BombSystemListener &listener)
{
	// LIFO is observable for simultaneous chains and intentionally preserved.
	while (!pendingDetonations_.empty())
	{
		const ActiveBombState bomb = pendingDetonations_.back();
		pendingDetonations_.pop_back();
		listener.onBombDetonated();
		createExplosion(arena, bomb.bomb.x(), bomb.bomb.y(),
		                bomb.explosionRange, bomb.id, actors, listener);
	}
}

void BombSystem::createExplosion(Arena &arena, float x, float y, int range,
                                 std::size_t bombId,
                                 std::span<ActorState *> actors,
                                 BombSystemListener &listener)
{
	std::vector<ExplosionCell> cells =
	    makeExplosionCells(arena, x, y, range, actors, listener);
	std::vector<ExplosionProjectionCell> projection =
	    makeExplosionProjection(arena, cells);
	explosions_.push_back(
	    {Explosion(std::move(cells), explosionDurationSeconds_),
		 std::move(projection),
		 {},
		 bombId});
	++dangerRevision_;
	listener.onExplosionStarted(x, y);
	rebuildExplosionProjection();
	std::fprintf(stderr,
	             "Explosion created: position=(%.3f, %.3f) range=%d "
	             "duration=%.1f s.\n",
	             x, y, range, explosionDurationSeconds_);
	logExplosionCells(explosions_.back().explosion);
}

std::vector<ExplosionCell>
BombSystem::makeExplosionCells(Arena &arena, float x, float y, int range,
                               std::span<ActorState *> actors,
                               BombSystemListener &listener)
{
	const int centerColumn = arena.columnAt(x);
	const int centerRow = arena.rowAt(y);
	std::vector<ExplosionCell> cells{{centerColumn, centerRow}};
	constexpr int directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
	for (const auto &direction : directions)
	{
		int distance = 1;
		bool isPropagating = true;
		while (isPropagating && distance <= range)
		{
			const int column = centerColumn + direction[0] * distance;
			const int row = centerRow + direction[1] * distance;
			const CellType cellType = arena.cellType(column, row);
			if (cellType == CellType::IndestructibleWall)
			{
				isPropagating = false;
				continue;
			}
			cells.push_back({column, row});
			queueBombsAt(arena, column, row, actors);
			if (cellType == CellType::DestructibleWall)
			{
				arena.destroyDestructibleWall(column, row);
				listener.onDestructibleWallDestroyed(column, row);
				std::fprintf(stderr,
				             "Destructible wall removed: cell=(%d,%d).\n",
				             column, row);
				isPropagating = false;
				continue;
			}
			++distance;
		}
	}
	return cells;
}

std::vector<ExplosionProjectionCell> BombSystem::makeExplosionProjection(
    const Arena &arena, const std::vector<ExplosionCell> &cells) const
{
	std::vector<ExplosionProjectionCell> projection;
	projection.reserve(cells.size());
	for (const ExplosionCell &cell : cells)
	{
		projection.push_back({arena.originX() + cell.column * arena.cellSize(),
		                      arena.originY() + cell.row * arena.cellSize(),
		                      arena.cellSize(), arena.cellSize()});
	}
	return projection;
}

void BombSystem::rebuildExplosionProjection()
{
	explosionProjection_.clear();
	for (const ActiveExplosionState &explosion : explosions_)
	{
		explosionProjection_.insert(explosionProjection_.end(),
		                            explosion.projection.begin(),
		                            explosion.projection.end());
	}
}

void BombSystem::logExplosionCells(const Explosion &explosion) const
{
	std::fputs("Explosion cells:", stderr);
	for (const ExplosionCell &cell : explosion.cells())
		std::fprintf(stderr, " (%d,%d)", cell.column, cell.row);
	std::fputc('\n', stderr);
}
} // namespace game
