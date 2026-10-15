/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   GameSession.cpp                                   :+:      :+:    :+: :+:      */
/*                                                    +:+ +:+   +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+ +#+#+#+#+#+   +#+      */
/*                                                +#+#+#+#+#+   +#+           +#+#+#+#+#+   */
/*   Created: 2026/09/25          by flaviobc      #+#    #+#             */
/*   Updated: 2026/10/03          by flaviobc     ###   ########.fr       */
/*                                                                                    */
/* ******************************************************************************** */
/*                                                                                    */
/* **********************************************************************************
 */

#include "GameSession.hpp"

#include "graphics/Renderer.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <optional>
#include <random>
#include <utility>
#include <vector>

namespace
{
constexpr float kPlayerScaleFromCell = 0.8F;
constexpr float kPlayerCollisionCellFraction = 0.25F;
constexpr float kPlayerSpawnRotationRadians = 3.14159265358979323846F;
constexpr float kEnemyVisualCellFraction = 0.8F;
constexpr bool kEnemiesEnabled = true;
constexpr bool kPickupsEnabled = true;
constexpr std::size_t kInitialPlayerMaximumActiveBombs = 1;
constexpr int kInitialPlayerLives = 1;
constexpr int kInitialPlayerExplosionRange = 1;

constexpr float kBombRed = 0.78F;
constexpr float kBombGreen = 0.16F;
constexpr float kBombBlue = 0.72F;

float inscribedPlayerHalfExtent(const game::ModelBounds &bounds, float scale)
{
	const float halfWidth = (bounds.maximumX - bounds.minimumX) * 0.5F;
	const float halfDepth = (bounds.maximumZ - bounds.minimumZ) * 0.5F;
	return std::min(halfWidth, halfDepth) * scale;
}

std::uint32_t chooseArenaSeed(const game::GameplayTuning &tuning,
                              std::optional<std::uint32_t> previousSeed = {})
{
	if (!tuning.randomSeedOnReset)
		return game::GameplayTuning::baseArenaSeed;
	std::uint32_t selected = std::random_device{}();
	if (previousSeed && selected == *previousSeed)
		++selected;
	return selected;
}

game::Arena makeConfiguredArena(const game::GameplayTuning &tuning,
                                std::optional<std::uint32_t> previousSeed = {})
{
	return game::Arena::makePrototype(tuning.arenaWidthCells,
	                                  tuning.arenaHeightCells,
	                                  chooseArenaSeed(tuning, previousSeed));
}

} // namespace

namespace game
{
GameSession::GameSession(
    GameplayTuning tuning, std::size_t indestructibleWallModelId,
    std::size_t labyrinthPillarModelId, std::size_t destructibleWallModelId,
    std::size_t bombModelId, std::size_t floorModelId,
    std::size_t powerUpBombCapacityModelId, std::size_t powerUpAuraModelId,
    std::size_t powerUpBlastRangeModelId, std::size_t powerUpLifeModelId,
    std::size_t enemyBasicModelId, SessionModelBounds modelBounds,
    SessionAudioClipSlots audioSlots)
    : tuning_(std::move(tuning)), arena_(makeConfiguredArena(tuning_)),
      presentation_(
          {indestructibleWallModelId, labyrinthPillarModelId,
		   destructibleWallModelId, bombModelId, floorModelId,
		   powerUpBombCapacityModelId, powerUpAuraModelId,
		   powerUpBlastRangeModelId, powerUpLifeModelId, enemyBasicModelId},
          std::move(modelBounds), tuning_.explosionDurationSeconds(),
          {tuning_.cameraAutoFrame, tuning_.cameraMapCoveragePercent}),
      audioFeedback_(audioSlots),
      bombSystem_(tuning_.bombFuseDurationSeconds(),
	              tuning_.explosionDurationSeconds()),
      enemyRoster_(tuning_.damageInvulnerabilitySeconds(),
	               tuning_.enemyProfiles)
{
	const SessionModelBounds &presentationBounds = presentation_.modelBounds();
	setPlayerAtSpawn();
	const float playerScale = arena_.cellSize() * kPlayerScaleFromCell;
	const float meshInscribedHalfExtent =
	    inscribedPlayerHalfExtent(presentationBounds.player, playerScale);
	const float corridorHalfExtent =
	    arena_.cellSize() * kPlayerCollisionCellFraction;
	player_.actor.collisionHalfExtent =
	    std::min(meshInscribedHalfExtent, corridorHalfExtent);
	const float enemyScale =
	    arena_.cellSize() * kEnemyVisualCellFraction /
	    std::max(presentationBounds.enemyBasic.maximumX -
		             presentationBounds.enemyBasic.minimumX,
		         presentationBounds.enemyBasic.maximumZ -
		             presentationBounds.enemyBasic.minimumZ);
	const float enemyCollisionHalfExtent =
	    inscribedPlayerHalfExtent(presentationBounds.enemyBasic, enemyScale);
	enemyRoster_.configureCollisionHalfExtent(enemyCollisionHalfExtent);
	enemyRoster_.reset(arena_, kEnemiesEnabled);
	resetPowerUps();
	const int centerColumn = arena_.width() / 2;
	const int centerRow = arena_.height() / 2;
	std::fprintf(stderr,
	             "Arena prototype: dimensions=%dx%d origin=(%.3f, %.3f) "
	             "center_cell=(%d,%d) center=(%.3f, %.3f).\n",
	             arena_.width(), arena_.height(), arena_.originX(),
	             arena_.originY(), centerColumn, centerRow,
	             arena_.cellCenterX(centerColumn),
	             arena_.cellCenterY(centerRow));
	std::fprintf(stderr, "Player spawn: cell=(%d,%d) position=(%.3f, %.3f).\n",
	             1, arena_.height() - 2, player_.actor.x, player_.actor.y);
	std::fprintf(stderr,
	             "Player collider: mesh_half_extents=(%.3f, %.3f) "
	             "visual_radius=%.3f mesh_inscribed_half_extent=%.3f "
	             "collision_half_extent=%.3f.\n",
	             (presentationBounds.player.maximumX -
	              presentationBounds.player.minimumX) *
	                 0.5F * playerScale,
	             (presentationBounds.player.maximumZ -
	              presentationBounds.player.minimumZ) *
	                 0.5F * playerScale,
	             presentationBounds.player.maximumHorizontalRadius *
	                 playerScale,
	             meshInscribedHalfExtent, player_.actor.collisionHalfExtent);
	const std::span<const EnemyState> enemies = enemyRoster_.view();
	for (std::size_t index = 0; index < enemies.size(); ++index)
	{
		const EnemyState &enemy = enemies[index];
		if (!enemy.active)
		{
			continue;
		}
		std::fprintf(stderr,
		             "Enemy patrol: index=%zu navigation=global lives=%d/%d "
		             "position=(%.3f, %.3f) collision_half_extent=%.3f.\n",
		             index, enemy.actor.health.lives,
		             PickupSystem::maximumLives, enemy.actor.x, enemy.actor.y,
		             enemy.actor.collisionHalfExtent);
	}
	std::fprintf(
	    stderr,
	    "Prototype map content: active_enemies=%zu pickups_enabled=%s.\n",
	    enemyRoster_.activeEnemyCount(), kPickupsEnabled ? "yes" : "no");
	std::fprintf(
	    stderr,
	    "Player effects: lives=%d bomb_capacity=%zu explosion_range=%d.\n",
	    player_.actor.health.lives, maximumActiveBombs(), explosionRange());
	std::fputs("Session state: Title. Press Space to start.\n", stderr);
}

bool GameSession::initialize(engine::graphics::Renderer &renderer)
{
	presentation_.initialize(renderer, makePresentationView());
	return true;
}

bool GameSession::configureAudioClips(
    const std::vector<engine::audio::AudioClipHandle> &handles,
    std::string &error)
{
	return audioFeedback_.configure(handles, error);
}

std::vector<engine::audio::AudioPlaybackCommand>
GameSession::takeAudioPlaybackCommands()
{
	return audioFeedback_.takePendingCommands();
}

void GameSession::update(const engine::input::KeyboardState &keyboard,
                         double deltaSeconds)
{
	const PlayerActions actions = playerInput_.actions(keyboard);
	if (sessionState_ == SessionState::Title)
	{
		if (actions.placeBomb)
		{
			sessionState_ = SessionState::Playing;
			std::fputs("Session state: Playing.\n", stderr);
		}
		return;
	}
	if (sessionState_ == SessionState::Paused)
	{
		if (actions.togglePause)
		{
			sessionState_ = SessionState::Playing;
			std::fputs("Session state: Playing (resumed).\n", stderr);
		}
		return;
	}
	if (sessionState_ == SessionState::Victory)
	{
		if (actions.placeBomb)
		{
			std::fputs("Victory reset requested.\n", stderr);
			resetSession();
		}
		return;
	}
	if (sessionState_ == SessionState::Defeated)
	{
		updateExplosion(deltaSeconds);
		if (!bombSystem_.hasActiveExplosions() && actions.placeBomb)
		{
			std::fputs("Defeat reset requested.\n", stderr);
			resetSession();
		}
		return;
	}
	if (actions.togglePause)
	{
		sessionState_ = SessionState::Paused;
		std::fputs("Session state: Paused.\n", stderr);
		return;
	}
	visualSeconds_ += deltaSeconds;
	player_.actor.health.invulnerabilityRemainingSeconds =
	    std::max(0.0, player_.actor.health.invulnerabilityRemainingSeconds -
		                  deltaSeconds);
	enemyRoster_.advanceInvulnerability(deltaSeconds);
	playerController_.update(player_, actions.move, deltaSeconds,
	                         static_cast<float>(tuning_.playerSpeed()), arena_,
	                         bombSystem_, enemyRoster_.view());
	updateAllBombPassThrough();
	updateExplosion(deltaSeconds);
	if (sessionState_ != SessionState::Playing)
	{
		return;
	}
	if constexpr (kEnemiesEnabled)
	{
		updateEnemies(deltaSeconds);
	}
	if constexpr (kPickupsEnabled)
	{
		collectPowerUps(player_.actor, ActorKind::Player, 0);
		collectLifePickup(player_.actor, ActorKind::Player, 0);
	}
	updateBomb(deltaSeconds);
	if (actions.placeBomb &&
	    activeBombCount(ActorKind::Player, 0) < maximumActiveBombs())
	{
		placeBomb(ActorKind::Player, 0, player_.actor);
	}
	if (sessionState_ == SessionState::Defeated &&
	    !bombSystem_.hasActiveExplosions())
	{
		resetSession();
	}
}

void GameSession::applyVisualState(engine::graphics::Renderer &renderer) const
{
	presentation_.apply(renderer, makePresentationView());
}

GamePresentationView GameSession::makePresentationView() const
{
	presentedBombs_.clear();
	presentedBombs_.reserve(bombSystem_.bombs().size());
	for (const ActiveBombState &activeBomb : bombSystem_.bombs())
	{
		presentedBombs_.push_back({activeBomb.bomb.x(), activeBomb.bomb.y()});
	}

	presentedPowerUps_.clear();
	const std::span<const PickupState> powerUps = pickups_.powerUps();
	presentedPowerUps_.reserve(powerUps.size());
	for (std::size_t index = 0; index < powerUps.size(); ++index)
	{
		const PickupState &powerUp = powerUps[index];
		if (!powerUp.revealed || powerUp.collected)
		{
			continue;
		}
		presentedPowerUps_.push_back(
		    {powerUp.column, powerUp.row,
			 powerUp.effect == PickupEffect::ExplosionRange
		         ? PowerUpPresentationKind::ExplosionRange
				 : PowerUpPresentationKind::BombCapacity,
			 index});
	}

	GamePresentationScreen screen = GamePresentationScreen::Playing;
	switch (sessionState_)
	{
	case SessionState::Title:
		screen = GamePresentationScreen::Title;
		break;
	case SessionState::Playing:
		break;
	case SessionState::Paused:
		screen = GamePresentationScreen::Paused;
		break;
	case SessionState::Defeated:
		screen = GamePresentationScreen::Defeated;
		break;
	case SessionState::Victory:
		screen = GamePresentationScreen::Victory;
		break;
	}

	return {arena_,
	        player_,
	        enemyRoster_.view(),
	        presentedBombs_,
	        presentedPowerUps_,
	        {pickups_.lifePickup().column, pickups_.lifePickup().row,
	         pickups_.lifePickup().active},
	        bombSystem_.explosionProjection(),
	        visualSeconds_,
	        screen,
	        sessionState_ == SessionState::Defeated &&
	            !bombSystem_.hasActiveExplosions(),
	        kEnemiesEnabled,
	        kPickupsEnabled,
	        static_cast<int>(activeBombCount(ActorKind::Player, 0))};
}
const std::vector<ExplosionProjectionCell> &
GameSession::explosionProjection() const
{
	return bombSystem_.explosionProjection();
}

void GameSession::updateAllBombPassThrough()
{
	bombSystem_.updatePassThrough(arena_, player_.actor);
	for (EnemyState &enemy : enemyRoster_.view())
	{
		if (enemy.active)
		{
			bombSystem_.updatePassThrough(arena_, enemy.actor);
		}
	}
}

GameSession::ActorReferences GameSession::actorReferences(bool activeOnly)
{
	ActorReferences references;
	references.values[references.count++] = &player_.actor;
	for (EnemyState &enemy : enemyRoster_.view())
	{
		if (!activeOnly || enemy.active)
			references.values[references.count++] = &enemy.actor;
	}
	return references;
}

void GameSession::updateBomb(double deltaSeconds)
{
	ActorReferences actors = actorReferences(false);
	bombSystem_.updateBombs(deltaSeconds, arena_, actors.view(), *this);
}

void GameSession::updateExplosion(double deltaSeconds)
{
	std::array<ExplosionDamageTarget, EnemyRoster::slotCount + 1> targets{};
	targets[0] = {ActorKind::Player, 0, &player_.actor, true};
	const std::span<EnemyState> enemies = enemyRoster_.view();
	for (std::size_t index = 0; index < enemies.size(); ++index)
		targets[index + 1] = {ActorKind::Enemy, index, &enemies[index].actor,
		                      enemies[index].active};
	const ExplosionDamageResult damage = bombSystem_.updateExplosions(
	    deltaSeconds, targets, sessionState_ == SessionState::Playing);
	if (sessionState_ != SessionState::Playing)
		return;
	for (const std::size_t index : damage.enemiesHit)
		enemyRoster_.applyDamage(index);
	if (damage.playerHit)
	{
		damagePlayer("explosion");
	}
	else
	{
		declareVictory();
	}
}
void GameSession::updateEnemies(double deltaSeconds)
{
	if (sessionState_ != SessionState::Playing)
		return;
	EnemyController(
	    arena_, bombSystem_, pickups_, player_.actor, enemyRoster_.view(),
	    *this, static_cast<float>(tuning_.enemySpeed()), tuning_.multipliers)
	    .update(deltaSeconds, kPickupsEnabled);
}

void GameSession::collectEnemyPickups(std::size_t enemyIndex, ActorState &actor)
{
	collectPowerUps(actor, ActorKind::Enemy, enemyIndex);
	collectLifePickup(actor, ActorKind::Enemy, enemyIndex);
}

std::optional<std::size_t> GameSession::placeEnemyBomb(std::size_t enemyIndex,
                                                       ActorState &actor)
{
	if (activeBombCount(ActorKind::Enemy, enemyIndex) >=
	    actor.maximumActiveBombs)
		return std::nullopt;
	return placeBomb(ActorKind::Enemy, enemyIndex, actor);
}

void GameSession::declareVictory()
{
	if constexpr (!kEnemiesEnabled)
	{
		return;
	}
	if (sessionState_ != SessionState::Playing || enemyRoster_.hasActiveEnemy())
	{
		return;
	}
	sessionState_ = SessionState::Victory;
	audioFeedback_.requestVictory();
	std::fputs("Session state: Victory. Press Space to reset.\n", stderr);
}

void GameSession::damagePlayer(const char *reason)
{
	if (sessionState_ != SessionState::Playing ||
	    player_.actor.health.invulnerabilityRemainingSeconds > 0.0)
	{
		return;
	}
	presentation_.queueCameraImpulse(player_.actor.x, player_.actor.y);
	--player_.actor.health.lives;
	std::fprintf(stderr,
	             "Player damaged: reason=%s lives=%d/%d position=(%.3f, "
	             "%.3f).\n",
	             reason, player_.actor.health.lives, PickupSystem::maximumLives,
	             player_.actor.x, player_.actor.y);
	audioFeedback_.requestDamage();
	if (player_.actor.health.lives <= 0)
	{
		defeatPlayer(reason);
		return;
	}
	player_.actor.health.invulnerabilityRemainingSeconds =
	    tuning_.damageInvulnerabilitySeconds();
	std::fprintf(stderr,
	             "Player damage invulnerability: seconds=%.2f position=(%.3f, "
	             "%.3f).\n",
	             tuning_.damageInvulnerabilitySeconds(), player_.actor.x,
	             player_.actor.y);
}

void GameSession::defeatPlayer(const char *reason)
{
	if (sessionState_ != SessionState::Playing)
	{
		return;
	}
	sessionState_ = SessionState::Defeated;
	audioFeedback_.requestDefeat();
	std::fprintf(stderr,
	             "Session state: Defeated reason=%s position=(%.3f, %.3f).\n",
	             reason, player_.actor.x, player_.actor.y);
}

std::optional<std::size_t> GameSession::placeBomb(ActorKind ownerKind,
                                                  std::size_t ownerIndex,
                                                  ActorState &actor)
{
	ActorReferences actors = actorReferences(true);
	const std::optional<std::size_t> id = bombSystem_.placeBomb(
	    arena_, ownerKind, ownerIndex, actor, actors.view());
	if (id)
		audioFeedback_.requestBombPlacement();
	return id;
}

std::size_t GameSession::activeBombCount(ActorKind ownerKind,
                                         std::size_t ownerIndex) const
{
	return bombSystem_.activeBombCount(ownerKind, ownerIndex);
}

void GameSession::resetPowerUps()
{
	pickups_.reset(arena_, kPickupsEnabled);
}

void GameSession::revealPowerUpAt(int column, int row)
{
	pickups_.revealAt(column, row);
}

void GameSession::collectPowerUps(ActorState &actor, ActorKind actorKind,
                                  std::size_t actorIndex)
{
	if (sessionState_ != SessionState::Playing ||
	    (actorKind == ActorKind::Enemy && !enemyRoster_.isActive(actorIndex)))
	{
		return;
	}
	if (const std::optional<PickupCollection> collection =
	        pickups_.collectAt(arena_, actor))
	{
		const char *actorName =
		    actorKind == ActorKind::Player ? "player" : "enemy";
		std::fprintf(
		    stderr,
		    "Power-up collected: actor=%s index=%zu cell=(%d,%d) effect=%s "
		    "bomb_capacity=%zu explosion_range=%d.\n",
		    actorName, actorIndex, collection->column, collection->row,
		    collection->effect == PickupEffect::BombCapacity
		        ? "bomb_capacity"
				: "explosion_range",
		    actor.maximumActiveBombs, actor.explosionRange);
		audioFeedback_.requestPickup();
	}
}

void GameSession::collectLifePickup(ActorState &actor, ActorKind actorKind,
                                    std::size_t actorIndex)
{
	if (sessionState_ != SessionState::Playing ||
	    (actorKind == ActorKind::Enemy && !enemyRoster_.isActive(actorIndex)))
	{
		return;
	}
	const std::optional<LifePickupCollection> collection =
	    pickups_.collectLifeAt(arena_, actor);
	if (!collection)
		return;
	std::fprintf(
	    stderr,
	    "Life pickup collected: actor=%s index=%zu cell=(%d,%d) lives=%d/%d "
	    "previous=%d.\n",
	    actorKind == ActorKind::Player ? "player" : "enemy", actorIndex,
	    collection->column, collection->row, actor.health.lives,
	    PickupSystem::maximumLives, collection->previousLives);
	audioFeedback_.requestPickup();
}

std::size_t GameSession::maximumActiveBombs() const
{
	return player_.actor.maximumActiveBombs;
}

int GameSession::explosionRange() const
{
	return player_.actor.explosionRange;
}

void GameSession::setPlayerAtSpawn()
{
	player_.actor.x = arena_.cellCenterX(1);
	player_.actor.y = arena_.cellCenterY(arena_.height() - 2);
	player_.facingRadians = kPlayerSpawnRotationRadians;
	player_.actor.maximumActiveBombs = kInitialPlayerMaximumActiveBombs;
	player_.actor.explosionRange = kInitialPlayerExplosionRange;
	player_.actor.bombPassThrough.clear();
}

void GameSession::onBombDetonated()
{
	audioFeedback_.requestBombDetonation();
}

void GameSession::onDestructibleWallDestroyed(int column, int row)
{
	revealPowerUpAt(column, row);
}

void GameSession::onExplosionStarted(float x, float y)
{
	presentation_.queueExplosion(x, y);
}

void GameSession::resetSession()
{
	arena_ = makeConfiguredArena(tuning_, arena_.generationSeed());
	setPlayerAtSpawn();
	enemyRoster_.reset(arena_, kEnemiesEnabled);
	playerController_.reset(player_);
	bombSystem_.reset();
	audioFeedback_.clearPendingCommands();
	resetPowerUps();
	player_.actor.health.lives = kInitialPlayerLives;
	player_.actor.health.invulnerabilityRemainingSeconds = 0.0;
	visualSeconds_ = 0.0;
	sessionState_ = SessionState::Playing;
	presentation_.resetTransientState();
	std::fprintf(
	    stderr,
	    "Session reset: arena=%dx%d seed=0x%08X seed_mode=%s "
	    "origin=(%.3f, %.3f), player "
	    "spawn cell=(%d,%d) position=(%.3f, %.3f), arena restored "
	    "with generated destructible layout, lives=%d/%d, "
	    "bomb_capacity=%zu "
	    "and explosion_range=%d enemies_enabled=%s pickups_enabled=%s.\n",
	    arena_.width(), arena_.height(), arena_.generationSeed(),
	    tuning_.randomSeedOnReset ? "random_per_reset" : "fixed",
	    arena_.originX(), arena_.originY(), 1, arena_.height() - 2,
	    player_.actor.x, player_.actor.y, player_.actor.health.lives,
	    PickupSystem::maximumLives, maximumActiveBombs(), explosionRange(),
	    kEnemiesEnabled ? "yes" : "no", kPickupsEnabled ? "yes" : "no");
}

} // namespace game
