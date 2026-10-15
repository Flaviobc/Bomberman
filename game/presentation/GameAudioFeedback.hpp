#pragma once

#include "audio/AudioSystem.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace game
{
struct SessionAudioClipSlots
{
	std::size_t bombPlacement = 0;
	std::size_t bombDetonation = 0;
	std::size_t pickup = 0;
	std::size_t damage = 0;
	std::size_t defeat = 0;
	std::size_t victory = 0;
};

// Associates Bomberman feedback events with preloaded Engine Audio clips. The
// AudioSystem owns clip data; configured handles remain valid for this session.
class GameAudioFeedback
{
  public:
	explicit GameAudioFeedback(SessionAudioClipSlots clipSlots);

	bool configure(const std::vector<engine::audio::AudioClipHandle> &handles,
	               std::string &error);
	void requestBombPlacement();
	void requestBombDetonation();
	void requestPickup();
	void requestDamage();
	void requestDefeat();
	void requestVictory();

	// Transfers queued commands once; Application consumes the returned values.
	std::vector<engine::audio::AudioPlaybackCommand> takePendingCommands();
	void clearPendingCommands();

  private:
	void request(const std::optional<engine::audio::AudioClipHandle> &clip,
	             float gain, const char *description);

	SessionAudioClipSlots clipSlots_;
	std::optional<engine::audio::AudioClipHandle> bombPlacementClip_;
	std::optional<engine::audio::AudioClipHandle> bombDetonationClip_;
	std::optional<engine::audio::AudioClipHandle> pickupClip_;
	std::optional<engine::audio::AudioClipHandle> damageClip_;
	std::optional<engine::audio::AudioClipHandle> defeatClip_;
	std::optional<engine::audio::AudioClipHandle> victoryClip_;
	std::vector<engine::audio::AudioPlaybackCommand> pendingCommands_;
};
} // namespace game
