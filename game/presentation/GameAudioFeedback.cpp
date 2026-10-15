#include "presentation/GameAudioFeedback.hpp"

#include <cstdio>
#include <utility>

namespace
{
constexpr float kBombPlacementGain = 0.25F;
constexpr float kBombDetonationGain = 0.40F;
constexpr float kPickupGain = 0.35F;
constexpr float kDamageGain = 0.35F;
constexpr float kDefeatGain = 0.35F;
constexpr float kVictoryGain = 0.35F;
} // namespace

namespace game
{
GameAudioFeedback::GameAudioFeedback(SessionAudioClipSlots clipSlots)
    : clipSlots_(clipSlots)
{
}

bool GameAudioFeedback::configure(
    const std::vector<engine::audio::AudioClipHandle> &handles,
    std::string &error)
{
	if (clipSlots_.bombPlacement >= handles.size() ||
	    clipSlots_.bombDetonation >= handles.size() ||
	    clipSlots_.pickup >= handles.size() ||
	    clipSlots_.damage >= handles.size() ||
	    clipSlots_.defeat >= handles.size() ||
	    clipSlots_.victory >= handles.size())
	{
		error = "game audio clip slot is unavailable";
		return false;
	}
	bombPlacementClip_ = handles[clipSlots_.bombPlacement];
	bombDetonationClip_ = handles[clipSlots_.bombDetonation];
	pickupClip_ = handles[clipSlots_.pickup];
	damageClip_ = handles[clipSlots_.damage];
	defeatClip_ = handles[clipSlots_.defeat];
	victoryClip_ = handles[clipSlots_.victory];
	std::fprintf(
	    stderr,
	    "Game audio configured: placement_slot=%zu detonation_slot=%zu "
	    "pickup_slot=%zu damage_slot=%zu defeat_slot=%zu victory_slot=%zu.\n",
	    clipSlots_.bombPlacement, clipSlots_.bombDetonation, clipSlots_.pickup,
	    clipSlots_.damage, clipSlots_.defeat, clipSlots_.victory);
	return true;
}

void GameAudioFeedback::requestBombPlacement()
{
	request(bombPlacementClip_, kBombPlacementGain, "bomb placement");
}

void GameAudioFeedback::requestBombDetonation()
{
	request(bombDetonationClip_, kBombDetonationGain, "bomb detonation");
}

void GameAudioFeedback::requestPickup()
{
	request(pickupClip_, kPickupGain, "pickup");
}

void GameAudioFeedback::requestDamage()
{
	request(damageClip_, kDamageGain, "player damage");
}

void GameAudioFeedback::requestDefeat()
{
	request(defeatClip_, kDefeatGain, "session defeat");
}

void GameAudioFeedback::requestVictory()
{
	request(victoryClip_, kVictoryGain, "session victory");
}

std::vector<engine::audio::AudioPlaybackCommand>
GameAudioFeedback::takePendingCommands()
{
	std::vector<engine::audio::AudioPlaybackCommand> commands =
	    std::move(pendingCommands_);
	pendingCommands_.clear();
	return commands;
}

void GameAudioFeedback::clearPendingCommands()
{
	pendingCommands_.clear();
}

void GameAudioFeedback::request(
    const std::optional<engine::audio::AudioClipHandle> &clip, float gain,
    const char *description)
{
	if (!clip)
	{
		return;
	}
	pendingCommands_.push_back({*clip, gain});
	std::fprintf(stderr, "Game audio requested: event=%s gain=%.2f.\n",
	             description, gain);
}
} // namespace game
