/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   AudioSystem.hpp                                   :+:      :+:    :+: :+:      */
/*                                                    +:+ +:+   +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+ +#+#+#+#+#+   +#+      */
/*                                                +#+#+#+#+#+   +#+           +#+#+#+#+#+   */
/*   Created: 2026/10/03          by flaviobc      #+#    #+#             */
/*   Updated: 2026/10/03          by flaviobc     ###   ########.fr       */
/*                                                                                    */
/* ******************************************************************************** */
/*                                                                                    */
/* **********************************************************************************
 */

#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace engine::audio
{
struct AudioClipHandle
{
	std::size_t value = 0;
};

struct AudioPlaybackCommand
{
	AudioClipHandle clip;
	float gain = 1.0F;
};

struct AudioClipLoadResult
{
	std::optional<AudioClipHandle> handle;
	std::string error;

	[[nodiscard]] explicit operator bool() const;
};

enum class AudioPlayResult
{
	Played,
	DroppedNoVoice,
	Failed,
};

class AudioSystem
{
  public:
	static constexpr std::size_t MaxVoices = 8;

	AudioSystem();
	~AudioSystem();

	AudioSystem(const AudioSystem &) = delete;
	AudioSystem &operator=(const AudioSystem &) = delete;
	AudioSystem(AudioSystem &&) = delete;
	AudioSystem &operator=(AudioSystem &&) = delete;

	[[nodiscard]] bool initialize(std::string &error);
	[[nodiscard]] AudioClipLoadResult loadWav(const std::string &path);
	[[nodiscard]] AudioPlayResult playOneShot(AudioClipHandle handle,
	                                          float gain, std::string &error);

  private:
	struct Impl;
	std::unique_ptr<Impl> impl_;
};
} // namespace engine::audio
