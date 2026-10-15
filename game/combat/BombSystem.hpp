/* ********************************************************************************
 */
/*                                                                                    */
/*                                                      :::      ::::::::   :::
 */
/*   BombSystem.hpp                                   :+:      :+:    :+:   :+:
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
#include "combat/Bomb.hpp"
#include "combat/Explosion.hpp"
#include "combat/ExplosionDamage.hpp"
#include "config/GameplayTuning.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace game
{
class Arena;

struct ActiveBombState
{
	Bomb bomb;
	ActorKind ownerKind = ActorKind::Player;
	std::size_t ownerIndex = 0;
	// Captured at placement so later pickups cannot change this bomb.
	int explosionRange = 1;
	std::size_t id = 0;
};

// Synchronous notifications preserve effect ordering without retaining Session.
class BombSystemListener
{
  public:
	virtual ~BombSystemListener() = default;
	virtual void onBombDetonated() = 0;
	virtual void onDestructibleWallDestroyed(int column, int row) = 0;
	virtual void onExplosionStarted(float x, float y) = 0;
};

// Owns the complete bomb/explosion lifecycle while Arena remains terrain
// authority. Read-only spans remain valid until the next non-const operation.
class BombSystem final
{
  public:
	explicit BombSystem(double fuseDurationSeconds =
	                        GameplayTuning::baseBombFuseDurationSeconds,
	                    double explosionDurationSeconds =
	                        GameplayTuning::baseExplosionDurationSeconds);
	double fuseDurationSeconds() const;
	double explosionDurationSeconds() const;

	std::optional<std::size_t>
	placeBomb(const Arena &arena, ActorKind ownerKind, std::size_t ownerIndex,
	          ActorState &owner, std::span<ActorState *> overlappingActors);
	void updateBombs(double deltaSeconds, Arena &arena,
	                 std::span<ActorState *> actors,
	                 BombSystemListener &listener);
	ExplosionDamageResult
	updateExplosions(double deltaSeconds,
	                 std::span<const ExplosionDamageTarget> targets,
	                 bool damageEnabled);
	void reset();

	void updatePassThrough(const Arena &arena, ActorState &actor) const;
	bool blocksActor(const Arena &arena, float centerX, float centerY,
	                 const ActorState &actor) const;
	bool intersectsBombCell(const Arena &arena, float centerX, float centerY,
	                        float halfExtent, const Bomb &bomb) const;
	bool intersectsActiveExplosion(float centerX, float centerY,
	                               float halfExtent) const;
	bool bombThreatens(const Arena &arena, float x, float y, float halfExtent,
	                   const Bomb &bomb, int range) const;
	bool hasBombAt(const Arena &arena, int column, int row) const;
	std::size_t activeBombCount(ActorKind ownerKind,
	                            std::size_t ownerIndex) const;
	bool bombCycleActive(std::size_t id) const;
	bool hasActiveBombs() const;
	bool hasActiveExplosions() const;
	std::size_t dangerRevision() const;
	std::span<const ActiveBombState> bombs() const;
	const std::vector<ExplosionProjectionCell> &explosionProjection() const;

  private:
	struct ActiveExplosionState
	{
		Explosion explosion;
		std::vector<ExplosionProjectionCell> projection;
		std::vector<ExplosionActorIdentity> damagedActors;
		std::size_t bombId = 0;
	};

	bool actorCanPassThrough(const ActorState &actor, const Bomb &bomb) const;
	void grantPassThrough(const Arena &arena, const Bomb &bomb,
	                      std::span<ActorState *> actors) const;
	void removePassThrough(const Bomb &bomb,
	                       std::span<ActorState *> actors) const;
	void queueDetonation(ActiveBombState bomb, bool triggeredByExplosion);
	void queueBombsAt(const Arena &arena, int column, int row,
	                  std::span<ActorState *> actors);
	void processPendingDetonations(Arena &arena, std::span<ActorState *> actors,
	                               BombSystemListener &listener);
	void createExplosion(Arena &arena, float x, float y, int range,
	                     std::size_t bombId, std::span<ActorState *> actors,
	                     BombSystemListener &listener);
	std::vector<ExplosionCell>
	makeExplosionCells(Arena &arena, float x, float y, int range,
	                   std::span<ActorState *> actors,
	                   BombSystemListener &listener);
	std::vector<ExplosionProjectionCell>
	makeExplosionProjection(const Arena &arena,
	                        const std::vector<ExplosionCell> &cells) const;
	void rebuildExplosionProjection();
	void logExplosionCells(const Explosion &explosion) const;

	std::vector<ActiveBombState> bombs_;
	std::vector<ActiveBombState> pendingDetonations_;
	std::vector<ActiveExplosionState> explosions_;
	std::vector<ExplosionProjectionCell> explosionProjection_;
	std::size_t nextBombId_ = 1;
	std::size_t dangerRevision_ = 0;
	double fuseDurationSeconds_;
	double explosionDurationSeconds_;
};
} // namespace game
