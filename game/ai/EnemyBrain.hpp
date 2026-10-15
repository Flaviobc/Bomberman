#pragma once

#include "actors/ActorState.hpp"
#include "combat/Explosion.hpp"
#include "config/GameplayTuning.hpp"

#include <cstdint>
#include <optional>
#include <random>
#include <span>
#include <vector>

namespace game
{
class Arena;
class BombSystem;
class PickupSystem;
struct EnemyState;

enum class EnemyActionState
{
	Patrol,
	ChooseAction,
	PlaceBomb,
	EscapeToSafeCell,
	WaitForExplosion,
	InspectResult,
};
enum class EnemyObjective
{
	None,
	AttackPlayer,
	AttackEnemy,
	LifePickup,
	PowerUp,
	DestroyWall,
	ApproachPlayer,
	ApproachEnemy,
	Patrol,
	Survive,
	EvadeThreat,
	DefensiveBomb,
};

// Observation contains public geometry only, never health, capacity, range,
// intentions or another brain. Index 0 is Player; enemies use slot + 1.
struct ObservedActor
{
	std::size_t identity = 0;
	float x = 0.0F;
	float y = 0.0F;
	float halfExtent = 0.0F;
	ExplosionCell direction{};
};
struct KnownBombTiming
{
	int range = 1;
	double remainingSeconds = 0.0;
};
struct ObservedBomb
{
	std::size_t id = 0;
	float x = 0.0F;
	float y = 0.0F;
	double firstSeenSeconds = 0.0;
	// Only populated after checking ownership. Unknown bombs have no exact
	// range/timer or owner attributes anywhere in the decision input.
	std::optional<KnownBombTiming> own;
};
struct ObservedPickup
{
	ExplosionCell cell;
	bool life = false;
	bool capacity = false;
};
struct AdversaryMemory
{
	std::size_t identity = 0;
	float x = 0.0F;
	float y = 0.0F;
	double approachSeconds = 0.0;
	double suspicion = 0.0;
};

// Tracks the result associated with every owned bomb independently. The
// current escape bomb is separate because movement can commit to only one
// route at a time while older bombs continue their own lifecycle.
struct OwnedBombPlan
{
	std::size_t id = 0;
	std::optional<ExplosionCell> inspectedWall;
	ExplosionCell placementDirection{1, 0};
};

// Each EnemyState owns exactly one brain by value. Snapshots and memory never
// alias another actor. capture() is the only boundary reading authoritative
// world data, and is called for ALL brains before any enemy executes actions.
struct EnemyBrain
{
	void reset(std::uint32_t seed);
	void capture(const Arena &arena, const BombSystem &system,
	             const PickupSystem &pickups, const ActorState &player,
	             std::span<const EnemyState> enemies, std::size_t self,
	             double deltaSeconds, bool pickupsEnabled);
	void rememberOwnBomb(std::size_t id, float x, float y, int range,
	                     double fuseSeconds);
	double unknownDeadline(const ObservedBomb &bomb,
	                       const GameplayTuningMultipliers &weights) const;
	bool hasBombAt(const Arena &arena, int column, int row) const;
	bool intersectsExplosion(float x, float y, float halfExtent) const;
	bool bombThreatens(const Arena &arena, float x, float y, float halfExtent,
	                   const ObservedBomb &bomb) const;
	std::size_t ownBombCount() const;
	double randomTieBreak();

	double timeSeconds = 0.0;
	double threat = 0.0;
	double threatAtDecision = 0.0;
	double retrySeconds = 0.0;
	double blockedSeconds = 0.0;
	int previousExits = 4;
	std::size_t perceptionRevision = 0;
	std::size_t selfIdentity = 0;
	std::vector<ObservedActor> actors;
	std::vector<ObservedBomb> bombs;
	std::vector<ExplosionProjectionCell> explosions;
	std::vector<ObservedPickup> pickups;
	std::vector<AdversaryMemory> adversaries;
	std::mt19937 random{1};
	EnemyPersonalityProfile personality = EnemyPersonalityProfile::Strategist;
	std::vector<unsigned> patrolVisits;
	bool safetyRefusalLogged = false;
	EnemyActionState action = EnemyActionState::ChooseAction;
	std::vector<ExplosionCell> route;
	std::size_t routeStep = 0;
	std::size_t observedDangerRevision = 0;
	std::vector<OwnedBombPlan> ownedBombPlans;
	std::vector<OwnedBombPlan> completedBombPlans;
	std::optional<std::size_t> escapingBombId;
	bool waitingForPlacementWindow = false;
	EnemyObjective objective = EnemyObjective::None;
	ExplosionCell objectiveCell{};
	std::size_t objectiveActor = 0;
};
} // namespace game
