/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   AudioSystem.cpp                                   :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/03          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/10/03          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "AudioSystem.hpp"

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_stdinc.h>

#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <limits>
#include <memory>
#include <vector>

namespace
{
constexpr SDL_AudioSpec kClipFormat{SDL_AUDIO_S16, 1, 48000};

struct SdlBufferDeleter
{
	void operator()(Uint8 *buffer) const
	{
		SDL_free(buffer);
	}
};
} // namespace

namespace engine::audio
{
struct AudioSystem::Impl
{
	using Clock = std::chrono::steady_clock;

	struct Clip
	{
		std::vector<Uint8> bytes;
		Clock::duration duration;
	};

	SDL_AudioDeviceID device = 0;
	std::array<SDL_AudioStream *, AudioSystem::MaxVoices> streams{};
	std::array<Clock::time_point, AudioSystem::MaxVoices> availableAt{};
	std::vector<Clip> clips;

	~Impl()
	{
		shutdown();
	}

	void shutdown()
	{
		const bool hadAudioResources =
		    device != 0 || streams.front() != nullptr;
		SDL_UnbindAudioStreams(streams.data(),
		                       static_cast<int>(streams.size()));
		for (SDL_AudioStream *&stream : streams)
		{
			SDL_DestroyAudioStream(stream);
			stream = nullptr;
		}
		if (device != 0)
		{
			SDL_CloseAudioDevice(device);
			device = 0;
		}
		if (hadAudioResources)
		{
			std::fputs("Audio voices and device destroyed.\n", stderr);
		}
	}
};

AudioClipLoadResult::operator bool() const
{
	return handle.has_value();
}

AudioSystem::AudioSystem() : impl_(std::make_unique<Impl>())
{
}

AudioSystem::~AudioSystem() = default;

bool AudioSystem::initialize(std::string &error)
{
	if (impl_->device != 0)
	{
		error = "audio system is already initialized";
		return false;
	}
	impl_->device =
	    SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &kClipFormat);
	if (impl_->device == 0)
	{
		error = std::string{"SDL_OpenAudioDevice failed: "} + SDL_GetError();
		return false;
	}
	for (SDL_AudioStream *&stream : impl_->streams)
	{
		stream = SDL_CreateAudioStream(&kClipFormat, nullptr);
		if (!stream)
		{
			error =
			    std::string{"SDL_CreateAudioStream failed: "} + SDL_GetError();
			impl_->shutdown();
			return false;
		}
	}
	if (!SDL_BindAudioStreams(impl_->device, impl_->streams.data(),
	                          static_cast<int>(impl_->streams.size())))
	{
		error = std::string{"SDL_BindAudioStreams failed: "} + SDL_GetError();
		impl_->shutdown();
		return false;
	}
	std::fprintf(stderr,
	             "Audio initialized: driver=%s voices=%zu format=%s "
	             "channels=%d frequency=%d Hz.\n",
	             SDL_GetCurrentAudioDriver(), impl_->streams.size(),
	             SDL_GetAudioFormatName(kClipFormat.format),
	             kClipFormat.channels, kClipFormat.freq);
	return true;
}

AudioClipLoadResult AudioSystem::loadWav(const std::string &path)
{
	if (impl_->device == 0)
	{
		return {std::nullopt, "audio system is not initialized"};
	}
	SDL_AudioSpec loadedSpec{};
	Uint8 *loadedData = nullptr;
	Uint32 loadedBytes = 0;
	if (!SDL_LoadWAV(path.c_str(), &loadedSpec, &loadedData, &loadedBytes))
	{
		return {std::nullopt, std::string{"SDL_LoadWAV failed for "} + path +
		                          ": " + SDL_GetError()};
	}
	const std::unique_ptr<Uint8, SdlBufferDeleter> ownedData(loadedData);
	if (loadedBytes == 0 || loadedSpec.format != kClipFormat.format ||
	    loadedSpec.channels != kClipFormat.channels ||
	    loadedSpec.freq != kClipFormat.freq)
	{
		return {std::nullopt, std::string{"unsupported WAV format for "} +
		                          path +
		                          "; expected signed 16-bit mono at 48000 Hz"};
	}
	constexpr std::size_t bytesPerFrame = sizeof(Sint16);
	if (loadedBytes % bytesPerFrame != 0)
	{
		return {std::nullopt,
		        std::string{"WAV data is not frame-aligned for "} + path};
	}
	const std::size_t frameCount = loadedBytes / bytesPerFrame;
	const auto duration = std::chrono::duration_cast<Impl::Clock::duration>(
	    std::chrono::duration<double>(static_cast<double>(frameCount) /
		                              kClipFormat.freq));
	impl_->clips.push_back(
	    {{ownedData.get(), ownedData.get() + loadedBytes}, duration});
	const AudioClipHandle handle{impl_->clips.size()};
	std::fprintf(stderr,
	             "Audio clip loaded: handle=%zu path=%s bytes=%u format=%s "
	             "channels=%d frequency=%d Hz.\n",
	             handle.value, path.c_str(), loadedBytes,
	             SDL_GetAudioFormatName(loadedSpec.format), loadedSpec.channels,
	             loadedSpec.freq);
	return {handle, {}};
}

AudioPlayResult AudioSystem::playOneShot(AudioClipHandle handle, float gain,
                                         std::string &error)
{
	error.clear();
	if (impl_->device == 0)
	{
		error = "audio system is not initialized";
		return AudioPlayResult::Failed;
	}
	if (handle.value == 0 || handle.value > impl_->clips.size())
	{
		error = "audio clip handle is invalid";
		return AudioPlayResult::Failed;
	}
	if (!std::isfinite(gain) || gain < 0.0F || gain > 1.0F)
	{
		error = "audio one-shot gain must be finite and between 0 and 1";
		return AudioPlayResult::Failed;
	}
	const Impl::Clip &clip = impl_->clips[handle.value - 1];
	if (clip.bytes.size() >
	    static_cast<std::size_t>(std::numeric_limits<int>::max()))
	{
		error = "audio clip is too large to queue";
		return AudioPlayResult::Failed;
	}

	const auto now = Impl::Clock::now();
	std::size_t voiceIndex = MaxVoices;
	for (std::size_t i = 0; i < impl_->streams.size(); ++i)
	{
		if (now < impl_->availableAt[i])
		{
			continue;
		}
		const int queuedBytes = SDL_GetAudioStreamQueued(impl_->streams[i]);
		if (queuedBytes < 0)
		{
			error = std::string{"SDL_GetAudioStreamQueued failed: "} +
			        SDL_GetError();
			return AudioPlayResult::Failed;
		}
		if (queuedBytes == 0)
		{
			voiceIndex = i;
			break;
		}
	}
	if (voiceIndex == MaxVoices)
	{
		std::fprintf(stderr,
		             "Audio one-shot dropped: all %zu voices are busy.\n",
		             MaxVoices);
		return AudioPlayResult::DroppedNoVoice;
	}

	SDL_AudioStream *stream = impl_->streams[voiceIndex];
	if (!SDL_ClearAudioStream(stream) ||
	    !SDL_SetAudioStreamGain(stream, gain) ||
	    !SDL_PutAudioStreamData(stream, clip.bytes.data(),
	                            static_cast<int>(clip.bytes.size())) ||
	    !SDL_FlushAudioStream(stream))
	{
		error = std::string{"audio one-shot queue failed: "} + SDL_GetError();
		return AudioPlayResult::Failed;
	}
	impl_->availableAt[voiceIndex] = now + clip.duration;
	std::fprintf(stderr,
	             "Audio one-shot queued: handle=%zu voice=%zu bytes=%zu "
	             "gain=%.3f.\n",
	             handle.value, voiceIndex + 1, clip.bytes.size(), gain);
	return AudioPlayResult::Played;
}
} // namespace engine::audio
