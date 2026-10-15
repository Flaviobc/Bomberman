/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   GameSession.hpp                                   :+:      :+:    :+: :+:      */
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

#pragma once

#include "core/Application.hpp"

#include "actors/EnemyRoster.hpp"
#include "actors/PlayerController.hpp"
#include "actors/PlayerState.hpp"
#include "ai/EnemyController.hpp"
#include "combat/Bomb.hpp"
#include "combat/BombSystem.hpp"
#include "combat/Explosion.hpp"
#include "config/GameplayTuning.hpp"
#include "input/PlayerInput.hpp"
#include "pickups/PickupSystem.hpp"
#include "presentation/GameAudioFeedback.hpp"
#include "presentation/GamePresentation.hpp"
#include "world/Arena.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace game
{
class GameSession final : public engine::ApplicationSession,
                          private BombSystemListener,
                          private EnemyControllerEffects
{
  public:
	GameSession(GameplayTuning tuning, std::size_t indestructibleWallModelId,
	            std::size_t labyrinthPillarModelId,
	            std::size_t destructibleWallModelId, std::size_t bombModelId,
	            std::size_t floorModelId,
	            std::size_t powerUpBombCapacityModelId,
	            std::size_t powerUpAuraModelId,
	            std::size_t powerUpBlastRangeModelId,
	            std::size_t powerUpLifeModelId, std::size_t enemyBasicModelId,
	            SessionModelBounds modelBounds,
	            SessionAudioClipSlots audioSlots);

	bool initialize(engine::graphics::Renderer &renderer) override;
	void update(const engine::input::KeyboardState &keyboard,
	            double deltaSeconds) override;
	void applyVisualState(engine::graphics::Renderer &renderer) const override;
	bool configureAudioClips(
	    const std::vector<engine::audio::AudioClipHandle> &handles,
	    std::string &error) override;
	std::vector<engine::audio::AudioPlaybackCommand>
	takeAudioPlaybackCommands() override;
	const std::vector<ExplosionProjectionCell> &explosionProjection() const;

  private:
	struct ActorReferences
	{
		std::array<ActorState *, EnemyRoster::slotCount + 1> values{};
		std::size_t count = 0;

		std::span<ActorState *> view()
		{
			return {values.data(), count};
		}
	};

	enum class SessionState
	{
		Title,
		Playing,
		Paused,
		Defeated,
		Victory,
	};

	void updateAllBombPassThrough();
	ActorReferences actorReferences(bool activeOnly);
	void updateBomb(double deltaSeconds);
	void updateExplosion(double deltaSeconds);
	void updateEnemies(double deltaSeconds);
	void declareVictory();
	void damagePlayer(const char *reason);
	void defeatPlayer(const char *reason);
	std::optional<std::size_t>
	placeBomb(ActorKind ownerKind, std::size_t ownerIndex, ActorState &actor);
	std::size_t activeBombCount(ActorKind ownerKind,
	                            std::size_t ownerIndex) const;
	void revealPowerUpAt(int column, int row);
	void resetPowerUps();
	void collectPowerUps(ActorState &actor, ActorKind actorKind,
	                     std::size_t actorIndex);
	void collectLifePickup(ActorState &actor, ActorKind actorKind,
	                       std::size_t actorIndex);
	std::size_t maximumActiveBombs() const;
	int explosionRange() const;
	void setPlayerAtSpawn();
	void resetSession();
	GamePresentationView makePresentationView() const;
	void onBombDetonated() override;
	void onDestructibleWallDestroyed(int column, int row) override;
	void onExplosionStarted(float x, float y) override;
	void collectEnemyPickups(std::size_t enemyIndex,
	                         ActorState &actor) override;
	std::optional<std::size_t> placeEnemyBomb(std::size_t enemyIndex,
	                                          ActorState &actor) override;

	GameplayTuning tuning_;
	Arena arena_;
	GamePresentation presentation_;
	GameAudioFeedback audioFeedback_;
	PlayerInput playerInput_;
	PlayerController playerController_;
	PlayerState player_;
	BombSystem bombSystem_;
	EnemyRoster enemyRoster_;
	PickupSystem pickups_;
	double visualSeconds_ = 0.0;
	SessionState sessionState_ = SessionState::Title;
	// Reused storage backs the non-owning view for one synchronous projection.
	mutable std::vector<BombPresentationState> presentedBombs_;
	mutable std::vector<PowerUpPresentationState> presentedPowerUps_;
};
} // namespace game
