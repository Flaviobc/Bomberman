#include "ai/EnemyBrain.hpp"

#include "actors/EnemyState.hpp"
#include "combat/BombSystem.hpp"
#include "pickups/PickupSystem.hpp"
#include "world/Arena.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

namespace game
{
void EnemyBrain::reset(std::uint32_t seed)
{
	*this = EnemyBrain{};
	random.seed(seed);
}

void EnemyBrain::capture(const Arena &arena, const BombSystem &system,
                         const PickupSystem &items, const ActorState &player,
                         std::span<const EnemyState> enemies, std::size_t self,
                         double deltaSeconds, bool pickupsEnabled)
{
	const double dt = std::max(0.0, deltaSeconds);
	timeSeconds += dt;
	selfIdentity = self + 1;
	const ActorState &me = enemies[self].actor;
	actors.clear();
	actors.push_back({0, player.x, player.y, player.collisionHalfExtent, {}});
	for (std::size_t index = 0; index < enemies.size(); ++index)
	{
		if (index == self || !enemies[index].active)
			continue;
		// Only observed position/direction and public collision geometry cross
		// this boundary. Never copy another EnemyState or read its brain.
		const ActorState &other = enemies[index].actor;
		actors.push_back({index + 1, other.x, other.y,
		                  other.collisionHalfExtent, enemies[index].direction});
	}
	std::vector<ObservedBomb> observed;
	observed.reserve(system.bombs().size());
	bool newBomb = false;
	bool changed = bombs.size() != system.bombs().size();
	for (const ActiveBombState &bomb : system.bombs())
	{
		const auto prior = std::find_if(bombs.begin(), bombs.end(),
		                                [&bomb](const ObservedBomb &entry)
		                                { return entry.id == bomb.id; });
		ObservedBomb entry{bomb.id, bomb.bomb.x(), bomb.bomb.y(),
		                   prior == bombs.end() ? timeSeconds
		                                        : prior->firstSeenSeconds,
		                   std::nullopt};
		if (bomb.ownerKind == ActorKind::Enemy && bomb.ownerIndex == self)
			entry.own = KnownBombTiming{bomb.explosionRange,
			                            bomb.bomb.remainingSeconds()};
		// No opponent fuse/range reads occur in the unknown branch.
		else if (prior == bombs.end())
			newBomb = newBomb || std::hypot(entry.x - me.x, entry.y - me.y) <
			                         arena.cellSize() * 5.0F;
		if (prior == bombs.end())
			std::fprintf(stderr,
			             "Enemy observed bomb: index=%zu id=%zu knowledge=%s "
			             "first_seen=%.3f.\n",
			             self, entry.id,
			             entry.own ? "own_exact" : "foreign_uncertain",
			             timeSeconds);
		changed = changed || prior == bombs.end();
		observed.push_back(entry);
	}
	bombs = std::move(observed);
	const auto &visibleExplosions = system.explosionProjection();
	changed = changed || explosions.size() != visibleExplosions.size() ||
	          !std::equal(explosions.begin(), explosions.end(),
	                      visibleExplosions.begin(),
	                      [](const auto &a, const auto &b)
	                      {
		                      return a.x == b.x && a.y == b.y &&
							         a.width == b.width && a.height == b.height;
	                      });
	explosions = visibleExplosions;
	if (changed)
		++perceptionRevision;
	pickups.clear();
	if (pickupsEnabled)
	{
		for (const PickupState &item : items.powerUps())
			if (item.revealed && !item.collected)
				pickups.push_back({{item.column, item.row},
				                   false,
				                   item.effect == PickupEffect::BombCapacity});
		if (items.lifePickup().active)
			pickups.push_back(
			    {{items.lifePickup().column, items.lifePickup().row},
				 true,
				 false});
	}
	int exits = 0;
	constexpr std::array<ExplosionCell, 4> directions{
	    {{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
	for (auto direction : directions)
	{
		const int column = arena.columnAt(me.x) + direction.column;
		const int row = arena.rowAt(me.y) + direction.row;
		const bool actorAtExit =
		    std::any_of(actors.begin(), actors.end(),
			            [&](const auto &other)
			            {
			                return arena.columnAt(other.x) == column &&
							       arena.rowAt(other.y) == row;
		                });
		if (!arena.isOccupied(column, row) && !hasBombAt(arena, column, row) &&
		    !actorAtExit)
			++exits;
	}
	threat = 0.0;
	for (ObservedActor &other : actors)
	{
		auto memory =
		    std::find_if(adversaries.begin(), adversaries.end(),
			             [&other](const AdversaryMemory &entry)
			             { return entry.identity == other.identity; });
		if (memory == adversaries.end())
		{
			adversaries.push_back({other.identity, other.x, other.y, 0.0, 0.0});
			memory = std::prev(adversaries.end());
		}
		const float dx = other.x - memory->x;
		const float dy = other.y - memory->y;
		if (other.identity == 0)
			other.direction = {(dx > 0.0F) - (dx < 0.0F),
			                   (dy > 0.0F) - (dy < 0.0F)};
		const double distance = std::hypot(other.x - me.x, other.y - me.y);
		const bool closing =
		    dx * (me.x - memory->x) + dy * (me.y - memory->y) > 0.000001F;
		memory->approachSeconds =
		    closing ? std::min(3.0, memory->approachSeconds + dt)
			        : std::max(0.0, memory->approachSeconds - dt * 0.25);
		const bool aligned = arena.columnAt(other.x) == arena.columnAt(me.x) ||
		                     arena.rowAt(other.y) == arena.rowAt(me.y);
		const bool near = distance < arena.cellSize() * 5.0;
		memory->suspicion = std::max(0.0, memory->suspicion - dt * 0.4);
		if (near && (newBomb || exits < previousExits))
			memory->suspicion = std::min(2.0, memory->suspicion + 0.6);
		if (near)
			threat =
			    std::max(threat, std::min(3.0, memory->approachSeconds *
				                                       (aligned ? 0.8 : 0.4) +
				                                   memory->suspicion));
		memory->x = other.x;
		memory->y = other.y;
	}
	previousExits = exits;
}

void EnemyBrain::rememberOwnBomb(std::size_t id, float x, float y, int range,
                                 double fuseSeconds)
{
	bombs.push_back(
	    {id, x, y, timeSeconds, KnownBombTiming{range, fuseSeconds}});
	++perceptionRevision;
}

double
EnemyBrain::unknownDeadline(const ObservedBomb &bomb,
                            const GameplayTuningMultipliers &weights) const
{
	// An estimate from first observation, never a promise about actual fuse.
	// Floor allows best-effort flight even for old bombs; caution never clears
	// risk.
	return std::max(0.1, AiUtilityBases::unknownObservationSeconds /
	                             std::max(0.25, weights.aiUnknownBombCaution) -
	                         (timeSeconds - bomb.firstSeenSeconds));
}

bool EnemyBrain::hasBombAt(const Arena &arena, int column, int row) const
{
	return std::any_of(bombs.begin(), bombs.end(),
	                   [&](const auto &bomb)
	                   {
		                   return arena.columnAt(bomb.x) == column &&
						          arena.rowAt(bomb.y) == row;
	                   });
}

bool EnemyBrain::intersectsExplosion(float x, float y, float halfExtent) const
{
	return std::any_of(explosions.begin(), explosions.end(),
	                   [&](const auto &cell)
	                   {
		                   return x + halfExtent >= cell.x &&
						          x - halfExtent <= cell.x + cell.width &&
						          y + halfExtent >= cell.y &&
						          y - halfExtent <= cell.y + cell.height;
	                   });
}

bool EnemyBrain::bombThreatens(const Arena &arena, float x, float y,
                               float halfExtent, const ObservedBomb &bomb) const
{
	const float separation = halfExtent + arena.cellSize() * 0.5F;
	const auto overlaps = [&](int column, int row)
	{
		return std::abs(x - arena.cellCenterX(column)) <= separation &&
		       std::abs(y - arena.cellCenterY(row)) <= separation;
	};
	const int column = arena.columnAt(bomb.x);
	const int row = arena.rowAt(bomb.y);
	if (overlaps(column, row))
		return true;
	// Unknown reach covers the entire visible corridor, stopped by real
	// terrain.
	const int range =
	    bomb.own ? bomb.own->range : std::max(arena.width(), arena.height());
	constexpr std::array<ExplosionCell, 4> directions{
	    {{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
	for (auto direction : directions)
		for (int step = 1; step <= range; ++step)
		{
			const int c = column + direction.column * step;
			const int r = row + direction.row * step;
			const CellType type = arena.cellType(c, r);
			if (type == CellType::IndestructibleWall)
				break;
			if (overlaps(c, r))
				return true;
			if (type == CellType::DestructibleWall)
				break;
		}
	return false;
}

std::size_t EnemyBrain::ownBombCount() const
{
	return static_cast<std::size_t>(
	    std::count_if(bombs.begin(), bombs.end(),
		              [](const auto &bomb) { return bomb.own.has_value(); }));
}

double EnemyBrain::randomTieBreak()
{
	return static_cast<double>(random()) / std::mt19937::max() * 0.25;
}
} // namespace game
