/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   GameplayTuning.hpp                              :+:      :+:    :+: :+:      */
/*                                                    +:+ +:+   +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+ +#+#+#+#+#+   +#+      */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/11          by flaviobc      #+#    #+#             */
/*   Updated: 2026/10/11          by flaviobc     ###   ########.fr       */
/*                                                                                    */
/* ******************************************************************************** */

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace game
{
// Stable numeric IDs selected by the external cfg. The runtime stores only the
// enum and code-owned factors, never profile names or per-enemy cfg values.
enum class EnemyPersonalityProfile : std::uint8_t
{
	Aggressive = 1,
	Coward = 2,
	Strategist = 3,
	Hunter = 4,
	Scavenger = 5,
};

struct EnemyPersonalityFactors
{
	double survival = 1.0;
	double threatResponse = 1.0;
	double aggression = 1.0;
	double pickup = 1.0;
	double wallDestruction = 1.0;
	double exploration = 1.0;
	double trapAvoidance = 1.0;
};

EnemyPersonalityFactors
enemyPersonalityFactors(EnemyPersonalityProfile profile);

struct AiUtilityBases
{
	static constexpr double survival = 1000.0;
	static constexpr double threat = 35.0;
	static constexpr double attack = 90.0;
	static constexpr double life = 85.0;
	static constexpr double pickup = 65.0;
	static constexpr double wall = 45.0;
	static constexpr double approach = 30.0;
	static constexpr double exploration = 15.0;
	static constexpr double trap = 20.0;
	static constexpr double unknownObservationSeconds = 0.8;
};

struct GameplayTuningMultipliers
{
	double playerSpeed = 1.0;
	double enemySpeed = 1.0;
	double bombFuseDuration = 1.0;
	double explosionDuration = 1.0;
	double damageInvulnerability = 1.0;
	double aiSurvival = 1.0;
	double aiThreatResponse = 1.0;
	double aiAggression = 1.0;
	double aiPickup = 1.0;
	double aiWallDestruction = 1.0;
	double aiExploration = 1.0;
	double aiUnknownBombCaution = 1.0;
	double aiTrapAvoidance = 1.0;
};

GameplayTuningMultipliers
applyEnemyPersonalityProfile(GameplayTuningMultipliers global,
                             EnemyPersonalityProfile profile);

// Base values remain code-owned; the external file can only scale them.
struct GameplayTuning
{
	static constexpr int minimumArenaCells = 11;
	static constexpr int maximumArenaCells = 31;
	static constexpr int baseArenaWidthCells = 15;
	static constexpr int baseArenaHeightCells = 15;
	static constexpr bool baseRandomSeedOnReset = false;
	static constexpr std::uint32_t baseArenaSeed = 0xB06B3EEDU;
	static constexpr double basePlayerSpeed = 0.84375;
	static constexpr double baseEnemySpeed = 0.84375;
	static constexpr double baseBombFuseDurationSeconds = 1.6;
	static constexpr double baseExplosionDurationSeconds = 0.5;
	static constexpr double baseDamageInvulnerabilitySeconds = 0.75;

	GameplayTuningMultipliers multipliers;
	std::array<EnemyPersonalityProfile, 3> enemyProfiles{
	    EnemyPersonalityProfile::Strategist,
	    EnemyPersonalityProfile::Strategist,
	    EnemyPersonalityProfile::Strategist};
	int arenaWidthCells = baseArenaWidthCells;
	int arenaHeightCells = baseArenaHeightCells;
	bool randomSeedOnReset = baseRandomSeedOnReset;
	bool cameraAutoFrame = true;
	int cameraMapCoveragePercent = 100;

	double playerSpeed() const;
	double enemySpeed() const;
	double bombFuseDurationSeconds() const;
	double explosionDurationSeconds() const;
	double damageInvulnerabilitySeconds() const;
};

struct GameplayTuningLoadResult
{
	GameplayTuning tuning;
	bool fileFound = false;
};

std::optional<GameplayTuningLoadResult>
loadGameplayTuning(const std::string &path, std::string &error);
void logGameplayTuning(const GameplayTuningLoadResult &result,
                       const std::string &path);
} // namespace game
