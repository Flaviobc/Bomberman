#include "presentation/GamePresentation.hpp"

#include "graphics/Renderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <utility>

namespace
{
constexpr float kPlayerScaleFromCell = 0.8F;
constexpr float kPlayerVisualRotationOffset = 3.14159265358979323846F;
constexpr float kEnemyVisualCellFraction = 0.8F;
constexpr float kEnemyFloatAmplitude = 0.012F;
constexpr float kEnemyFloatRadiansPerSecond = 6.28318530717959F;
constexpr float kEnemyAuraCellFraction = 0.76F;
constexpr float kEnemyAuraPulseAmplitude = 0.055F;
constexpr float kEnemyAuraPulseRadiansPerSecond = 5.0F;
constexpr float kEnemyAuraFloorOffset = 0.004F;
constexpr float kAuraBloomLuminance = 1.12F;
constexpr engine::graphics::InstanceColorization kPowerUpAuraColor{
    0.30980393F, 0.13725491F, 0.56862748F, 1.0F};
constexpr float kPowerUpVisualCellFraction = 0.6F;
constexpr float kPowerUpAuraCellFraction = 0.88F;
constexpr float kPowerUpAuraPulseAmplitude = 0.08F;
constexpr float kPowerUpAuraPulseRadiansPerSecond = 9.42477796076938F;
constexpr float kPowerUpAuraFloatAmplitude = 0.012F;
constexpr float kPowerUpAuraFloatRadiansPerSecond = 6.28318530717959F;
constexpr float kPowerUpAuraRotationRadiansPerSecond = 0.8F;
constexpr float kDamageCameraImpulseAmplitude = 0.035F;
constexpr double kDamageCameraImpulseDurationSeconds = 0.18;
constexpr float kDamageCameraImpulseFrequencyHz = 18.0F;
constexpr double kExplosionSmokeDurationSeconds = 0.35;
constexpr double kExplosionSmokeOverlapSeconds = 0.05;

engine::graphics::InstanceColorization
enemyProfileColorization(game::EnemyPersonalityProfile profile)
{
	using game::EnemyPersonalityProfile;
	switch (profile)
	{
	case EnemyPersonalityProfile::Aggressive:
		return {1.0F, 0.16F, 0.12F, 1.0F};
	case EnemyPersonalityProfile::Coward:
		return {0.81960785F, 0.49411765F, 0.05098039F, 1.0F};
	case EnemyPersonalityProfile::Strategist:
		return {0.20F, 0.48F, 1.0F, 1.0F};
	case EnemyPersonalityProfile::Hunter:
		return {0.16F, 0.78F, 0.25F, 1.0F};
	case EnemyPersonalityProfile::Scavenger:
		return {0.05F, 0.85F, 0.72F, 1.0F};
	}
	return {};
}

engine::graphics::InstanceColorization
enemyProfileAuraColor(game::EnemyPersonalityProfile profile)
{
	return enemyProfileColorization(profile);
}

engine::graphics::InstanceEmission
auraEmission(const engine::graphics::InstanceColorization &color)
{
	constexpr float kRedLuminance = 0.2126F;
	constexpr float kGreenLuminance = 0.7152F;
	constexpr float kBlueLuminance = 0.0722F;
	const float luminance = color.red * kRedLuminance +
	                        color.green * kGreenLuminance +
	                        color.blue * kBlueLuminance;
	return {color.red, color.green, color.blue,
	        kAuraBloomLuminance / std::max(luminance, 0.0001F)};
}

engine::graphics::TerminalHudState
terminalHudState(game::GamePresentationScreen screen)
{
	switch (screen)
	{
	case game::GamePresentationScreen::Title:
		return engine::graphics::TerminalHudState::Title;
	case game::GamePresentationScreen::Paused:
		return engine::graphics::TerminalHudState::Paused;
	case game::GamePresentationScreen::Victory:
		return engine::graphics::TerminalHudState::Success;
	case game::GamePresentationScreen::Defeated:
		return engine::graphics::TerminalHudState::Failure;
	case game::GamePresentationScreen::Playing:
		return engine::graphics::TerminalHudState::None;
	}
	return engine::graphics::TerminalHudState::None;
}
} // namespace

namespace game
{
GamePresentation::GamePresentation(GamePresentationModelIds modelIds,
                                   SessionModelBounds modelBounds,
                                   double explosionVisualDurationSeconds,
                                   CameraPresentationConfig cameraConfig)
    : modelIds_(modelIds), modelBounds_(std::move(modelBounds)),
      explosionVisualDurationSeconds_(explosionVisualDurationSeconds),
      cameraConfig_(cameraConfig)
{
}

const SessionModelBounds &GamePresentation::modelBounds() const
{
	return modelBounds_;
}

std::span<const GamePresentation::ActiveFireballCell>
GamePresentation::FireballCellTracker::update(
    std::span<const ExplosionProjectionCell> cells)
{
	next_.clear();
	newlyActive_.clear();
	next_.reserve(cells.size());
	newlyActive_.reserve(cells.size());
	for (const ExplosionProjectionCell &cell : cells)
	{
		const ActiveFireballCell current{cell.x + cell.width * 0.5F,
		                                 cell.y + cell.height * 0.5F};
		if (std::find(next_.begin(), next_.end(), current) != next_.end())
		{
			continue;
		}
		next_.push_back(current);
		if (std::find(active_.begin(), active_.end(), current) == active_.end())
		{
			newlyActive_.push_back(current);
		}
	}
	active_.swap(next_);
	return newlyActive_;
}

void GamePresentation::FireballCellTracker::clear()
{
	active_.clear();
	next_.clear();
	newlyActive_.clear();
}

void GamePresentation::initialize(engine::graphics::Renderer &renderer,
                                  const GamePresentationView &view) const
{
	applyArenaBounds(renderer, view.arena);
	applyCamera(renderer, view);
	renderer.setStaticMeshInstances(makeSceneInstances(view));
}

void GamePresentation::apply(engine::graphics::Renderer &renderer,
                             const GamePresentationView &view) const
{
	applyArenaBounds(renderer, view.arena);
	applyCamera(renderer, view);
	renderer.selectObjectAnimation(view.player.visualAnimationIndex);
	renderer.setHudState(
	    {view.player.actor.health.lives,
		 static_cast<int>(view.player.actor.maximumActiveBombs),
		 view.activePlayerBombs, view.player.actor.explosionRange});
	renderer.setTerminalHudState(terminalHudState(view.screen));
	updateHudTexts(renderer, view);
	if (vfxResetPending_)
	{
		renderer.emitParticleBurst({});
		vfxResetPending_ = false;
	}
	for (const ExplosionSmoke &burst : explosionSmokePending_)
	{
		renderer.emitPointLight({{burst.x, 0.15F, -burst.y, 0.0F, 1.0F},
		                         explosionVisualDurationSeconds_,
		                         1.0F,
		                         0.35F,
		                         0.05F,
		                         1.2F,
		                         0.8F});
	}
	explosionSmokePending_.clear();
	for (const ActiveFireballCell &active :
	     fireballCells_.update(view.explosionCells))
	{
		renderer.emitParticleBurst({{active.x, 0.13F, -active.y, 0.0F, 1.0F},
		                            explosionVisualDurationSeconds_,
		                            1.45F,
		                            1.18F,
		                            0.78F,
		                            9,
		                            engine::graphics::VfxSpriteKind::Fireball});
		renderer.emitParticleBurst(
		    {{active.x, 0.11F, -active.y, 0.0F, 1.0F},
			 kExplosionSmokeDurationSeconds,
			 0.72F,
			 0.68F,
			 0.64F,
			 4,
			 engine::graphics::VfxSpriteKind::Smoke,
			 std::max(0.0, explosionVisualDurationSeconds_ -
			                   kExplosionSmokeOverlapSeconds)});
	}
	if (cameraImpulsePending_.active)
	{
		renderer.emitCameraImpulse(
		    {cameraImpulsePending_.x, 0.12F, -cameraImpulsePending_.y,
			 kDamageCameraImpulseAmplitude, kDamageCameraImpulseDurationSeconds,
			 kDamageCameraImpulseFrequencyHz});
		cameraImpulsePending_.active = false;
	}
	// Title renders only the HUD. The complete scene is refreshed on entry
	// to gameplay, so no arena revision or duplicated logical state is needed.
	if (view.screen != GamePresentationScreen::Title)
		renderer.setStaticMeshInstances(makeSceneInstances(view));
	const float playerScale = view.arena.cellSize() * kPlayerScaleFromCell;
	renderer.setObjectTransform(makeModelTransform(
	    view.player.actor.x, view.player.actor.y, modelBounds_.player,
	    playerScale, view.player.facingRadians + kPlayerVisualRotationOffset,
	    false));
}

void GamePresentation::applyArenaBounds(engine::graphics::Renderer &renderer,
                                        const Arena &arena) const
{
	const float maximumX =
	    arena.originX() + static_cast<float>(arena.width()) * arena.cellSize();
	const float maximumLogicalY =
	    arena.originY() + static_cast<float>(arena.height()) * arena.cellSize();
	if (!renderer.setSceneBounds(
	        {arena.originX(), maximumX, -maximumLogicalY, -arena.originY()}))
	{
		std::fputs("Game arena bounds projection failed.\n", stderr);
	}
}

void GamePresentation::applyCamera(engine::graphics::Renderer &renderer,
                                   const GamePresentationView &view) const
{
	const float coverage =
	    static_cast<float>(cameraConfig_.mapCoveragePercent) / 100.0F;
	if (!renderer.setCameraFraming({cameraConfig_.autoFrame, coverage}) ||
	    !renderer.setCameraTrackingTarget({view.player.actor.x, 0.0F,
	                                       -view.player.actor.y,
	                                       cameraTrackingResetPending_}))
	{
		std::fputs("Game camera projection failed.\n", stderr);
		return;
	}
	cameraTrackingResetPending_ = false;
}

void GamePresentation::queueExplosion(float x, float y)
{
	explosionSmokePending_.push_back({x, y});
}

void GamePresentation::queueCameraImpulse(float x, float y)
{
	cameraImpulsePending_ = {x, y, true};
}

void GamePresentation::resetTransientState()
{
	vfxResetPending_ = true;
	explosionSmokePending_.clear();
	fireballCells_.clear();
	cameraImpulsePending_ = {};
	cameraTrackingResetPending_ = true;
}

void GamePresentation::updateHudTexts(engine::graphics::Renderer &renderer,
                                      const GamePresentationView &view) const
{
	if (appliedHudTextScreen_ && *appliedHudTextScreen_ == view.screen &&
	    appliedHudTextRestartAvailable_ == view.restartAvailable)
	{
		return;
	}
	std::vector<engine::graphics::HudText> texts;
	switch (view.screen)
	{
	case GamePresentationScreen::Title:
		texts = {{"AGENTS BOMBERMAN", -0.33F, 0.55F, 0.05F},
		         {"SPACE: START", -0.21F, -0.36F, 0.035F}};
		break;
	case GamePresentationScreen::Paused:
		texts = {{"PAUSED", -0.15F, 0.34F, 0.05F},
		         {"P: RESUME", -0.1575F, -0.27F, 0.035F}};
		break;
	case GamePresentationScreen::Victory:
		texts = {{"VICTORY!", -0.2F, 0.34F, 0.05F},
		         {"SPACE: START", -0.21F, -0.27F, 0.035F}};
		break;
	case GamePresentationScreen::Defeated:
		texts = {{"DEFEAT!", -0.175F, 0.34F, 0.05F}};
		if (view.restartAvailable)
		{
			texts.push_back({"SPACE: START", -0.21F, -0.27F, 0.035F});
		}
		break;
	case GamePresentationScreen::Playing:
		break;
	}
	appliedHudTextScreen_ = view.screen;
	appliedHudTextRestartAvailable_ = view.restartAvailable;
	if (!renderer.setHudTexts(texts))
	{
		if (!hudTextUpdateFailed_)
		{
			std::fputs("Game HUD text projection failed.\n", stderr);
		}
		hudTextUpdateFailed_ = true;
		return;
	}
	hudTextUpdateFailed_ = false;
}

const std::vector<engine::graphics::StaticMeshInstance> &
GamePresentation::makeSceneInstances(const GamePresentationView &view) const
{
	auto &instances = sceneInstances_;
	instances.clear();
	for (int row = 0; row < view.arena.height(); ++row)
	{
		for (int column = 0; column < view.arena.width(); ++column)
		{
			const float x = view.arena.cellCenterX(column);
			const float y = view.arena.cellCenterY(row);
			instances.push_back(
			    {modelIds_.floor,
				 makeModelTransform(x, y, modelBounds_.floor,
				                    view.arena.cellSize(), 0.0F, true),
				 {},
				 {},
				 true});
			const CellType cellType = view.arena.cellType(column, row);
			if (cellType == CellType::Free)
			{
				continue;
			}
			const bool isBorder = isArenaBorderCell(view.arena, column, row);
			const bool isLabyrinthPillar =
			    cellType == CellType::IndestructibleWall && !isBorder;
			const std::size_t modelId =
			    cellType == CellType::DestructibleWall
			        ? modelIds_.destructibleWall
			        : (isLabyrinthPillar ? modelIds_.labyrinthPillar
			                             : modelIds_.indestructibleWall);
			const ModelBounds &modelBounds =
			    cellType == CellType::DestructibleWall
			        ? modelBounds_.destructibleWall
			        : (isLabyrinthPillar ? modelBounds_.labyrinthPillar
			                             : modelBounds_.indestructibleWall);
			instances.push_back(
			    {modelId,
				 makeModelTransform(x, y, modelBounds, view.arena.cellSize(),
				                    0.0F, false),
				 {},
				 {},
				 true});
		}
	}
	for (const BombPresentationState &bomb : view.bombs)
	{
		instances.push_back(
		    {modelIds_.bomb,
			 makeModelTransform(bomb.x, bomb.y, modelBounds_.bomb,
			                    view.arena.cellSize(), 0.0F, false),
			 {},
			 {},
			 true});
	}
	if (view.pickupsEnabled)
	{
		for (const PowerUpPresentationState &powerUp : view.powerUps)
		{
			const bool isBlastRange =
			    powerUp.kind == PowerUpPresentationKind::ExplosionRange;
			const ModelBounds &powerUpBounds =
			    isBlastRange ? modelBounds_.powerUpBlastRange
				             : modelBounds_.powerUpBombCapacity;
			const float powerUpScale =
			    view.arena.cellSize() * kPowerUpVisualCellFraction /
			    std::max(powerUpBounds.maximumX - powerUpBounds.minimumX,
				         powerUpBounds.maximumZ - powerUpBounds.minimumZ);
			instances.push_back(
			    {isBlastRange ? modelIds_.powerUpBlastRange
				              : modelIds_.powerUpBombCapacity,
				 makeModelTransform(view.arena.cellCenterX(powerUp.column),
				                    view.arena.cellCenterY(powerUp.row),
				                    powerUpBounds, powerUpScale, 0.0F, false),
				 {},
				 {},
				 true});
			const float auraScale =
			    view.arena.cellSize() * kPowerUpAuraCellFraction /
			    std::max(modelBounds_.powerUpAura.maximumX -
				             modelBounds_.powerUpAura.minimumX,
				         modelBounds_.powerUpAura.maximumZ -
				             modelBounds_.powerUpAura.minimumZ);
			const float visualSeconds =
			    static_cast<float>(view.visualSeconds) +
			    static_cast<float>(powerUp.visualPhaseIndex) * 1.1F;
			const float pulse =
			    1.0F +
			    kPowerUpAuraPulseAmplitude *
			        std::sin(visualSeconds * kPowerUpAuraPulseRadiansPerSecond);
			auto auraTransform = makeModelTransform(
			    view.arena.cellCenterX(powerUp.column),
			    view.arena.cellCenterY(powerUp.row), modelBounds_.powerUpAura,
			    auraScale * pulse,
			    visualSeconds * kPowerUpAuraRotationRadiansPerSecond, false);
			auraTransform.y +=
			    kPowerUpAuraFloatAmplitude *
			    (1.0F +
				 std::sin(visualSeconds * kPowerUpAuraFloatRadiansPerSecond));
			instances.push_back({modelIds_.powerUpAura, auraTransform,
			                     kPowerUpAuraColor,
			                     auraEmission(kPowerUpAuraColor), true});
		}
		if (view.lifePickup.active)
		{
			const float lifePickupScale =
			    view.arena.cellSize() * kPowerUpVisualCellFraction /
			    std::max(modelBounds_.powerUpLife.maximumX -
				             modelBounds_.powerUpLife.minimumX,
				         modelBounds_.powerUpLife.maximumZ -
				             modelBounds_.powerUpLife.minimumZ);
			const float visualSeconds =
			    static_cast<float>(view.visualSeconds) + 4.4F;
			instances.push_back(
			    {modelIds_.powerUpLife,
				 makeModelTransform(
			         view.arena.cellCenterX(view.lifePickup.column),
			         view.arena.cellCenterY(view.lifePickup.row),
			         modelBounds_.powerUpLife, lifePickupScale, 0.0F, false),
				 {},
				 {},
				 true});
			const float auraScale =
			    view.arena.cellSize() * kPowerUpAuraCellFraction /
			    std::max(modelBounds_.powerUpAura.maximumX -
				             modelBounds_.powerUpAura.minimumX,
				         modelBounds_.powerUpAura.maximumZ -
				             modelBounds_.powerUpAura.minimumZ);
			const float pulse =
			    1.0F +
			    kPowerUpAuraPulseAmplitude *
			        std::sin(visualSeconds * kPowerUpAuraPulseRadiansPerSecond);
			auto auraTransform = makeModelTransform(
			    view.arena.cellCenterX(view.lifePickup.column),
			    view.arena.cellCenterY(view.lifePickup.row),
			    modelBounds_.powerUpAura, auraScale * pulse,
			    visualSeconds * kPowerUpAuraRotationRadiansPerSecond, false);
			auraTransform.y +=
			    kPowerUpAuraFloatAmplitude *
			    (1.0F +
				 std::sin(visualSeconds * kPowerUpAuraFloatRadiansPerSecond));
			instances.push_back({modelIds_.powerUpAura, auraTransform,
			                     kPowerUpAuraColor,
			                     auraEmission(kPowerUpAuraColor), true});
		}
	}
	if (view.enemiesEnabled)
	{
		for (std::size_t index = 0; index < view.enemies.size(); ++index)
		{
			const EnemyState &enemy = view.enemies[index];
			if (!enemy.active)
			{
				continue;
			}
			const float enemyScale =
			    view.arena.cellSize() * kEnemyVisualCellFraction /
			    std::max(modelBounds_.enemyBasic.maximumX -
				             modelBounds_.enemyBasic.minimumX,
				         modelBounds_.enemyBasic.maximumZ -
				             modelBounds_.enemyBasic.minimumZ);
			const float visualSeconds = static_cast<float>(view.visualSeconds) +
			                            static_cast<float>(index) * 0.8F;
			auto enemyTransform = makeModelTransform(
			    enemy.actor.x, enemy.actor.y, modelBounds_.enemyBasic,
			    enemyScale,
			    std::atan2(static_cast<float>(enemy.direction.row),
				           static_cast<float>(enemy.direction.column)),
			    false);
			enemyTransform.y +=
			    kEnemyFloatAmplitude *
			    std::sin(visualSeconds * kEnemyFloatRadiansPerSecond);
			const auto profileColor =
			    enemyProfileColorization(enemy.brain.personality);
			const auto auraColor =
			    enemyProfileAuraColor(enemy.brain.personality);
			const float auraScale =
			    view.arena.cellSize() * kEnemyAuraCellFraction /
			    std::max(modelBounds_.powerUpAura.maximumX -
				             modelBounds_.powerUpAura.minimumX,
				         modelBounds_.powerUpAura.maximumZ -
				             modelBounds_.powerUpAura.minimumZ);
			const float auraPulse =
			    1.0F +
			    kEnemyAuraPulseAmplitude *
			        std::sin(visualSeconds * kEnemyAuraPulseRadiansPerSecond);
			auto auraTransform = makeModelTransform(
			    enemy.actor.x, enemy.actor.y, modelBounds_.powerUpAura,
			    auraScale * auraPulse, 0.0F, false);
			auraTransform.y += kEnemyAuraFloorOffset;
			instances.push_back({modelIds_.powerUpAura, auraTransform,
			                     auraColor, auraEmission(auraColor), false});
			instances.push_back(
			    {modelIds_.enemyBasic, enemyTransform, profileColor, {}, true});
		}
	}
	return instances;
}

engine::graphics::WorldTransform GamePresentation::makeModelTransform(
    float logicalX, float logicalY, const ModelBounds &modelBounds, float scale,
    float rotationRadians, bool alignTop) const
{
	const float centerX = (modelBounds.minimumX + modelBounds.maximumX) * 0.5F;
	const float centerZ = (modelBounds.minimumZ + modelBounds.maximumZ) * 0.5F;
	const float cosine = std::cos(rotationRadians);
	const float sine = std::sin(rotationRadians);
	const float worldX = logicalX - scale * (cosine * centerX + sine * centerZ);
	const float worldZ =
	    -logicalY + scale * (sine * centerX - cosine * centerZ);
	const float worldY =
	    -(alignTop ? modelBounds.maximumY : modelBounds.minimumY) * scale;
	return {worldX, worldY, worldZ, rotationRadians, scale};
}

bool GamePresentation::isArenaBorderCell(const Arena &arena, int column,
                                         int row) const
{
	return column == 0 || row == 0 || column == arena.width() - 1 ||
	       row == arena.height() - 1;
}
} // namespace game
