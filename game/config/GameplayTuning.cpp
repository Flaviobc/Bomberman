/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   GameplayTuning.cpp                              :+:      :+:    :+: :+:      */
/*                                                    +:+ +:+   +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+ +#+#+#+#+#+   +#+      */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/11          by flaviobc      #+#    #+#             */
/*   Updated: 2026/10/11          by flaviobc     ###   ########.fr       */
/*                                                                                    */
/* ******************************************************************************** */

#include "config/GameplayTuning.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string_view>

namespace
{
struct AiMultiplierField
{
	std::string_view key;
	double game::GameplayTuningMultipliers::*member;
	double base;
};
constexpr std::array<AiMultiplierField, 8> kAiFields{{
    {"ai_survival_utility_multiplier",
	 &game::GameplayTuningMultipliers::aiSurvival,
	 game::AiUtilityBases::survival},
    {"ai_threat_response_multiplier",
	 &game::GameplayTuningMultipliers::aiThreatResponse,
	 game::AiUtilityBases::threat},
    {"ai_aggression_utility_multiplier",
	 &game::GameplayTuningMultipliers::aiAggression,
	 game::AiUtilityBases::attack},
    {"ai_pickup_utility_multiplier", &game::GameplayTuningMultipliers::aiPickup,
	 game::AiUtilityBases::pickup},
    {"ai_wall_destruction_utility_multiplier",
	 &game::GameplayTuningMultipliers::aiWallDestruction,
	 game::AiUtilityBases::wall},
    {"ai_exploration_utility_multiplier",
	 &game::GameplayTuningMultipliers::aiExploration,
	 game::AiUtilityBases::exploration},
    {"ai_unknown_bomb_caution_multiplier",
	 &game::GameplayTuningMultipliers::aiUnknownBombCaution,
	 game::AiUtilityBases::unknownObservationSeconds},
    {"ai_trap_avoidance_multiplier",
	 &game::GameplayTuningMultipliers::aiTrapAvoidance,
	 game::AiUtilityBases::trap},
}};
std::string_view trim(std::string_view value)
{
	const std::size_t first = value.find_first_not_of(" \t\r\n");
	if (first == std::string_view::npos)
		return {};
	const std::size_t last = value.find_last_not_of(" \t\r\n");
	return value.substr(first, last - first + 1);
}

bool parsePositiveFinite(std::string_view text, double &value)
{
	const std::string owned(text);
	char *end = nullptr;
	errno = 0;
	value = std::strtod(owned.c_str(), &end);
	return end != owned.c_str() && *end == '\0' && errno != ERANGE &&
	       std::isfinite(value) && value > 0.0;
}

bool parseArenaDimension(std::string_view text, int &value)
{
	const std::string owned(text);
	char *end = nullptr;
	errno = 0;
	const long parsed = std::strtol(owned.c_str(), &end, 10);
	if (end == owned.c_str() || *end != '\0' || errno == ERANGE ||
	    parsed < std::numeric_limits<int>::min() ||
	    parsed > std::numeric_limits<int>::max())
	{
		return false;
	}
	value = static_cast<int>(parsed);
	return value >= game::GameplayTuning::minimumArenaCells &&
	       value <= game::GameplayTuning::maximumArenaCells && value % 2 != 0;
}

bool parseCameraCoveragePercent(std::string_view text, int &value)
{
	const std::string owned(text);
	char *end = nullptr;
	errno = 0;
	const long parsed = std::strtol(owned.c_str(), &end, 10);
	if (end == owned.c_str() || *end != '\0' || errno == ERANGE ||
	    parsed < 60 || parsed > 100)
	{
		return false;
	}
	value = static_cast<int>(parsed);
	return true;
}

bool parseBoolean(std::string_view text, bool &value)
{
	if (text == "true")
	{
		value = true;
		return true;
	}
	if (text == "false")
	{
		value = false;
		return true;
	}
	return false;
}

bool parseEnemyProfile(std::string_view text,
                       game::EnemyPersonalityProfile &profile)
{
	const std::string owned(text);
	char *end = nullptr;
	errno = 0;
	const long parsed = std::strtol(owned.c_str(), &end, 10);
	if (end == owned.c_str() || *end != '\0' || errno == ERANGE ||
	    parsed < static_cast<long>(game::EnemyPersonalityProfile::Aggressive) ||
	    parsed > static_cast<long>(game::EnemyPersonalityProfile::Scavenger))
	{
		return false;
	}
	profile = static_cast<game::EnemyPersonalityProfile>(parsed);
	return true;
}

std::string lineError(const std::string &path, std::size_t line,
                      std::string_view key, const char *reason)
{
	return path + ": line " + std::to_string(line) + ", key '" +
	       std::string(key) + "': " + reason;
}
} // namespace

namespace game
{
EnemyPersonalityFactors enemyPersonalityFactors(EnemyPersonalityProfile profile)
{
	switch (profile)
	{
	case EnemyPersonalityProfile::Aggressive:
		return {.survival = 0.85,
		        .threatResponse = 0.75,
		        .aggression = 1.60,
		        .pickup = 0.70,
		        .wallDestruction = 1.25,
		        .exploration = 0.80,
		        .trapAvoidance = 0.85};
	case EnemyPersonalityProfile::Coward:
		return {.survival = 1.25,
		        .threatResponse = 1.80,
		        .aggression = 0.45,
		        .pickup = 0.95,
		        .wallDestruction = 0.70,
		        .exploration = 0.80,
		        .trapAvoidance = 1.50};
	case EnemyPersonalityProfile::Strategist:
		return {.survival = 1.05,
		        .threatResponse = 1.10,
		        .aggression = 1.00,
		        .pickup = 1.00,
		        .wallDestruction = 1.30,
		        .exploration = 0.90,
		        .trapAvoidance = 1.30};
	case EnemyPersonalityProfile::Hunter:
		return {.survival = 0.95,
		        .threatResponse = 0.85,
		        .aggression = 1.45,
		        .pickup = 0.60,
		        .wallDestruction = 0.90,
		        .exploration = 1.15,
		        .trapAvoidance = 0.95};
	case EnemyPersonalityProfile::Scavenger:
		return {.survival = 1.00,
		        .threatResponse = 1.05,
		        .aggression = 0.70,
		        .pickup = 1.80,
		        .wallDestruction = 0.80,
		        .exploration = 1.30,
		        .trapAvoidance = 1.10};
	}
	return {};
}

GameplayTuningMultipliers
applyEnemyPersonalityProfile(GameplayTuningMultipliers global,
                             EnemyPersonalityProfile profile)
{
	const EnemyPersonalityFactors factors = enemyPersonalityFactors(profile);
	global.aiSurvival *= factors.survival;
	global.aiThreatResponse *= factors.threatResponse;
	global.aiAggression *= factors.aggression;
	global.aiPickup *= factors.pickup;
	global.aiWallDestruction *= factors.wallDestruction;
	global.aiExploration *= factors.exploration;
	global.aiTrapAvoidance *= factors.trapAvoidance;
	return global;
}

double GameplayTuning::playerSpeed() const
{
	return basePlayerSpeed * multipliers.playerSpeed;
}

double GameplayTuning::enemySpeed() const
{
	return baseEnemySpeed * multipliers.enemySpeed;
}

double GameplayTuning::bombFuseDurationSeconds() const
{
	return baseBombFuseDurationSeconds * multipliers.bombFuseDuration;
}

double GameplayTuning::explosionDurationSeconds() const
{
	return baseExplosionDurationSeconds * multipliers.explosionDuration;
}

double GameplayTuning::damageInvulnerabilitySeconds() const
{
	return baseDamageInvulnerabilitySeconds * multipliers.damageInvulnerability;
}

std::optional<GameplayTuningLoadResult>
loadGameplayTuning(const std::string &path, std::string &error)
{
	error.clear();
	std::error_code existsError;
	const bool fileExists = std::filesystem::exists(path, existsError);
	if (existsError)
	{
		error = "Gameplay tuning path check failed: " + path + ": " +
		        existsError.message();
		return std::nullopt;
	}
	if (!fileExists)
		return GameplayTuningLoadResult{};

	std::ifstream file(path);
	if (!file)
	{
		error = "Gameplay tuning file could not be opened: " + path;
		return std::nullopt;
	}

	GameplayTuningLoadResult result;
	result.fileFound = true;
	bool playerSpeedSeen = false;
	bool enemySpeedSeen = false;
	bool bombFuseSeen = false;
	bool explosionDurationSeen = false;
	bool damageInvulnerabilitySeen = false;
	bool arenaWidthSeen = false;
	bool arenaHeightSeen = false;
	bool randomSeedOnResetSeen = false;
	bool cameraAutoFrameSeen = false;
	bool cameraCoverageSeen = false;
	std::array<bool, kAiFields.size()> aiSeen{};
	std::array<bool, 3> enemyProfileSeen{};
	std::string lineText;
	std::size_t lineNumber = 0;
	while (std::getline(file, lineText))
	{
		++lineNumber;
		const std::size_t comment = lineText.find('#');
		const std::string_view content = trim(std::string_view(lineText).substr(
		    0, comment == std::string::npos ? lineText.size() : comment));
		if (content.empty())
			continue;
		const std::size_t separator = content.find('=');
		const std::string_view key = separator == std::string_view::npos
		                                 ? trim(content)
		                                 : trim(content.substr(0, separator));
		if (separator == std::string_view::npos || key.empty())
		{
			error = lineError(path, lineNumber, key, "expected 'key = value'");
			return std::nullopt;
		}
		double *destination = nullptr;
		bool *seen = nullptr;
		if (key == "player_speed_multiplier")
		{
			destination = &result.tuning.multipliers.playerSpeed;
			seen = &playerSpeedSeen;
		}
		else if (key == "enemy_speed_multiplier")
		{
			destination = &result.tuning.multipliers.enemySpeed;
			seen = &enemySpeedSeen;
		}
		else if (key == "bomb_fuse_duration_multiplier")
		{
			destination = &result.tuning.multipliers.bombFuseDuration;
			seen = &bombFuseSeen;
		}
		else if (key == "explosion_duration_multiplier")
		{
			destination = &result.tuning.multipliers.explosionDuration;
			seen = &explosionDurationSeen;
		}
		else if (key == "damage_invulnerability_multiplier")
		{
			destination = &result.tuning.multipliers.damageInvulnerability;
			seen = &damageInvulnerabilitySeen;
		}
		else if (key == "arena_width_cells" || key == "arena_height_cells")
		{
			seen =
			    key == "arena_width_cells" ? &arenaWidthSeen : &arenaHeightSeen;
			if (*seen)
			{
				error = lineError(path, lineNumber, key, "duplicate key");
				return std::nullopt;
			}
			int dimension = 0;
			if (!parseArenaDimension(trim(content.substr(separator + 1)),
			                         dimension))
			{
				error =
				    lineError(path, lineNumber, key,
					          "value must be an odd integer between 11 and 31");
				return std::nullopt;
			}
			*seen = true;
			if (key == "arena_width_cells")
				result.tuning.arenaWidthCells = dimension;
			else
				result.tuning.arenaHeightCells = dimension;
			continue;
		}
		else if (key == "random_seed_on_reset")
		{
			if (randomSeedOnResetSeen)
			{
				error = lineError(path, lineNumber, key, "duplicate key");
				return std::nullopt;
			}
			if (!parseBoolean(trim(content.substr(separator + 1)),
			                  result.tuning.randomSeedOnReset))
			{
				error = lineError(path, lineNumber, key,
				                  "value must be 'true' or 'false'");
				return std::nullopt;
			}
			randomSeedOnResetSeen = true;
			continue;
		}
		else if (key == "camera_auto_frame")
		{
			if (cameraAutoFrameSeen)
			{
				error = lineError(path, lineNumber, key, "duplicate key");
				return std::nullopt;
			}
			if (!parseBoolean(trim(content.substr(separator + 1)),
			                  result.tuning.cameraAutoFrame))
			{
				error = lineError(path, lineNumber, key,
				                  "value must be 'true' or 'false'");
				return std::nullopt;
			}
			cameraAutoFrameSeen = true;
			continue;
		}
		else if (key == "camera_map_coverage_percent")
		{
			if (cameraCoverageSeen)
			{
				error = lineError(path, lineNumber, key, "duplicate key");
				return std::nullopt;
			}
			if (!parseCameraCoveragePercent(
			        trim(content.substr(separator + 1)),
			        result.tuning.cameraMapCoveragePercent))
			{
				error =
				    lineError(path, lineNumber, key,
					          "value must be an integer from 60 through 100");
				return std::nullopt;
			}
			cameraCoverageSeen = true;
			continue;
		}
		else if (key.starts_with("enemy_") && key.ends_with("_profile") &&
		         key.size() == std::string_view("enemy_0_profile").size() &&
		         key[6] >= '0' && key[6] <= '2')
		{
			const std::size_t slot = static_cast<std::size_t>(key[6] - '0');
			if (enemyProfileSeen[slot])
			{
				error = lineError(path, lineNumber, key, "duplicate key");
				return std::nullopt;
			}
			EnemyPersonalityProfile profile;
			if (!parseEnemyProfile(trim(content.substr(separator + 1)),
			                       profile))
			{
				error = lineError(
				    path, lineNumber, key,
				    "value must be an integer profile ID from 1 through 5");
				return std::nullopt;
			}
			enemyProfileSeen[slot] = true;
			result.tuning.enemyProfiles[slot] = profile;
			continue;
		}
		else
		{
			for (std::size_t index = 0; index < kAiFields.size(); ++index)
				if (key == kAiFields[index].key)
				{
					destination =
					    &(result.tuning.multipliers.*kAiFields[index].member);
					seen = &aiSeen[index];
					break;
				}
			if (!destination)
			{
				error = lineError(path, lineNumber, key, "unknown key");
				return std::nullopt;
			}
		}
		if (*seen)
		{
			error = lineError(path, lineNumber, key, "duplicate key");
			return std::nullopt;
		}
		const std::string_view valueText = trim(content.substr(separator + 1));
		double value = 0.0;
		if (!parsePositiveFinite(valueText, value))
		{
			error = lineError(path, lineNumber, key,
			                  "value must be finite and greater than zero");
			return std::nullopt;
		}
		*seen = true;
		*destination = value;
	}
	if (!file.eof())
	{
		error = "Gameplay tuning file read failed: " + path;
		return std::nullopt;
	}
	// Multipliers can be finite while the resolved duration overflows, or the
	// speed cannot be represented by the float consumed by actor controllers.
	const auto validSpeed = [](double speed)
	{
		return std::isfinite(speed) && speed > 0.0 &&
		       speed <= std::numeric_limits<float>::max() &&
		       static_cast<float>(speed) > 0.0F;
	};
	const auto validDuration = [](double duration)
	{ return std::isfinite(duration) && duration > 0.0; };
	if (!validSpeed(result.tuning.playerSpeed()) ||
	    !validSpeed(result.tuning.enemySpeed()) ||
	    !validDuration(result.tuning.bombFuseDurationSeconds()) ||
	    !validDuration(result.tuning.explosionDurationSeconds()) ||
	    !validDuration(result.tuning.damageInvulnerabilitySeconds()))
	{
		error = "Gameplay tuning resolved value is not positive and "
		        "representable (float speed / double duration): " +
		        path;
		return std::nullopt;
	}
	return result;
}

void logGameplayTuning(const GameplayTuningLoadResult &result,
                       const std::string &path)
{
	const GameplayTuning &tuning = result.tuning;
	for (const auto &field : kAiFields)
	{
		const double multiplier = tuning.multipliers.*field.member;
		const bool inverse =
		    field.member == &GameplayTuningMultipliers::aiUnknownBombCaution;
		const long double resolved =
		    inverse ? field.base / std::max(0.25, multiplier)
			        : static_cast<long double>(field.base) * multiplier;
		std::fprintf(stderr,
		             "Gameplay AI tuning: key=%.*s base=%.3f multiplier=%.5f "
		             "nominal=%.5Lg mode=%s.\n",
		             static_cast<int>(field.key.size()), field.key.data(),
		             field.base, multiplier, resolved,
		             inverse ? "inverse_window" : "utility");
	}
	for (std::size_t slot = 0; slot < tuning.enemyProfiles.size(); ++slot)
	{
		const EnemyPersonalityProfile profile = tuning.enemyProfiles[slot];
		const EnemyPersonalityFactors factors =
		    enemyPersonalityFactors(profile);
		std::fprintf(stderr,
		             "Gameplay enemy profile: slot=%zu id=%u factors="
		             "survival=%.2f threat=%.2f aggression=%.2f pickup=%.2f "
		             "wall=%.2f exploration=%.2f trap=%.2f.\n",
		             slot, static_cast<unsigned>(profile), factors.survival,
		             factors.threatResponse, factors.aggression, factors.pickup,
		             factors.wallDestruction, factors.exploration,
		             factors.trapAvoidance);
	}
	std::fprintf(
	    stderr,
	    "Gameplay tuning: source=%s path=%s.\n"
	    "Gameplay tuning value: player_speed base=%.5f "
	    "multiplier=%.5f resolved=%.5f.\n"
	    "Gameplay tuning value: enemy_speed base=%.5f "
	    "multiplier=%.5f resolved=%.5f.\n"
	    "Gameplay tuning value: bomb_fuse_duration base=%.3f "
	    "multiplier=%.5f resolved=%.3f seconds.\n"
	    "Gameplay tuning value: explosion_duration base=%.3f "
	    "multiplier=%.5f resolved=%.3f seconds.\n"
	    "Gameplay tuning value: damage_invulnerability base=%.3f "
	    "multiplier=%.5f resolved=%.3f seconds.\n",
	    result.fileFound ? "file" : "defaults_missing_file", path.c_str(),
	    GameplayTuning::basePlayerSpeed, tuning.multipliers.playerSpeed,
	    tuning.playerSpeed(), GameplayTuning::baseEnemySpeed,
	    tuning.multipliers.enemySpeed, tuning.enemySpeed(),
	    GameplayTuning::baseBombFuseDurationSeconds,
	    tuning.multipliers.bombFuseDuration, tuning.bombFuseDurationSeconds(),
	    GameplayTuning::baseExplosionDurationSeconds,
	    tuning.multipliers.explosionDuration, tuning.explosionDurationSeconds(),
	    GameplayTuning::baseDamageInvulnerabilitySeconds,
	    tuning.multipliers.damageInvulnerability,
	    tuning.damageInvulnerabilitySeconds());
	std::fprintf(stderr,
	             "Gameplay arena: dimensions=%dx%d supported_odd_range=%d..%d "
	             "random_seed_on_reset=%s fixed_seed=0x%08X "
	             "camera_auto_frame=%s camera_map_coverage_percent=%d.\n",
	             tuning.arenaWidthCells, tuning.arenaHeightCells,
	             GameplayTuning::minimumArenaCells,
	             GameplayTuning::maximumArenaCells,
	             tuning.randomSeedOnReset ? "true" : "false",
	             GameplayTuning::baseArenaSeed,
	             tuning.cameraAutoFrame ? "true" : "false",
	             tuning.cameraMapCoveragePercent);
}
} // namespace game
