#pragma once

#include "actors/EnemyState.hpp"
#include "actors/PlayerState.hpp"
#include "combat/Explosion.hpp"
#include "graphics/Renderer.hpp"
#include "world/Arena.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace game
{
struct ModelBounds
{
	float minimumX = 0.0F;
	float maximumX = 0.0F;
	float minimumY = 0.0F;
	float maximumY = 0.0F;
	float minimumZ = 0.0F;
	float maximumZ = 0.0F;
	float maximumHorizontalRadius = 0.0F;
};

struct SessionModelBounds
{
	ModelBounds player;
	ModelBounds indestructibleWall;
	ModelBounds labyrinthPillar;
	ModelBounds destructibleWall;
	ModelBounds bomb;
	ModelBounds floor;
	ModelBounds powerUpBombCapacity;
	ModelBounds powerUpAura;
	ModelBounds powerUpBlastRange;
	ModelBounds powerUpLife;
	ModelBounds enemyBasic;
};

struct GamePresentationModelIds
{
	std::size_t indestructibleWall = 0;
	std::size_t labyrinthPillar = 0;
	std::size_t destructibleWall = 0;
	std::size_t bomb = 0;
	std::size_t floor = 0;
	std::size_t powerUpBombCapacity = 0;
	std::size_t powerUpAura = 0;
	std::size_t powerUpBlastRange = 0;
	std::size_t powerUpLife = 0;
	std::size_t enemyBasic = 0;
};

struct CameraPresentationConfig
{
	bool autoFrame = true;
	int mapCoveragePercent = 100;
};

enum class GamePresentationScreen
{
	Title,
	Playing,
	Paused,
	Defeated,
	Victory,
};

enum class PowerUpPresentationKind
{
	BombCapacity,
	ExplosionRange,
};

struct BombPresentationState
{
	float x = 0.0F;
	float y = 0.0F;
};

struct PowerUpPresentationState
{
	int column = 0;
	int row = 0;
	PowerUpPresentationKind kind = PowerUpPresentationKind::BombCapacity;
	std::size_t visualPhaseIndex = 0;
};

struct LifePickupPresentationState
{
	int column = 0;
	int row = 0;
	bool active = false;
};

// This synchronous, non-owning view is valid only for one presentation call.
// Gameplay coordinates use X/Y on the arena plane; presentation maps Y to -Z.
struct GamePresentationView
{
	const Arena &arena;
	const PlayerState &player;
	std::span<const EnemyState> enemies;
	std::span<const BombPresentationState> bombs;
	std::span<const PowerUpPresentationState> powerUps;
	LifePickupPresentationState lifePickup;
	std::span<const ExplosionProjectionCell> explosionCells;
	double visualSeconds = 0.0;
	GamePresentationScreen screen = GamePresentationScreen::Title;
	bool restartAvailable = false;
	bool enemiesEnabled = false;
	bool pickupsEnabled = false;
	int activePlayerBombs = 0;
};

// Owns only presentation configuration, projection caches and transient visual
// commands. GameSession remains the owner of every logical gameplay state.
class GamePresentation
{
  public:
	GamePresentation(GamePresentationModelIds modelIds,
	                 SessionModelBounds modelBounds,
	                 double explosionVisualDurationSeconds,
	                 CameraPresentationConfig cameraConfig);

	const SessionModelBounds &modelBounds() const;
	void initialize(engine::graphics::Renderer &renderer,
	                const GamePresentationView &view) const;
	void apply(engine::graphics::Renderer &renderer,
	           const GamePresentationView &view) const;
	void queueExplosion(float x, float y);
	void queueCameraImpulse(float x, float y);
	void resetTransientState();

  private:
	struct ExplosionSmoke
	{
		float x = 0.0F;
		float y = 0.0F;
	};
	struct ActiveFireballCell
	{
		float x = 0.0F;
		float y = 0.0F;
		bool operator==(const ActiveFireballCell &) const = default;
	};
	class FireballCellTracker
	{
	  public:
		std::span<const ActiveFireballCell>
		update(std::span<const ExplosionProjectionCell> cells);
		void clear();

	  private:
		std::vector<ActiveFireballCell> active_;
		std::vector<ActiveFireballCell> next_;
		std::vector<ActiveFireballCell> newlyActive_;
	};

	struct PendingCameraImpulse
	{
		float x = 0.0F;
		float y = 0.0F;
		bool active = false;
	};

	void updateHudTexts(engine::graphics::Renderer &renderer,
	                    const GamePresentationView &view) const;
	void applyArenaBounds(engine::graphics::Renderer &renderer,
	                      const Arena &arena) const;
	void applyCamera(engine::graphics::Renderer &renderer,
	                 const GamePresentationView &view) const;
	bool isArenaBorderCell(const Arena &arena, int column, int row) const;
	engine::graphics::WorldTransform
	makeModelTransform(float logicalX, float logicalY,
	                   const ModelBounds &modelBounds, float scale,
	                   float rotationRadians, bool alignTop) const;
	const std::vector<engine::graphics::StaticMeshInstance> &
	makeSceneInstances(const GamePresentationView &view) const;

	GamePresentationModelIds modelIds_;
	SessionModelBounds modelBounds_;
	double explosionVisualDurationSeconds_ = 0.0;
	CameraPresentationConfig cameraConfig_;
	mutable std::optional<GamePresentationScreen> appliedHudTextScreen_;
	mutable bool appliedHudTextRestartAvailable_ = false;
	mutable bool hudTextUpdateFailed_ = false;
	mutable std::vector<ExplosionSmoke> explosionSmokePending_;
	mutable FireballCellTracker fireballCells_;
	mutable bool vfxResetPending_ = false;
	mutable PendingCameraImpulse cameraImpulsePending_;
	mutable bool cameraTrackingResetPending_ = true;
	mutable std::vector<engine::graphics::StaticMeshInstance> sceneInstances_;
};
} // namespace game
