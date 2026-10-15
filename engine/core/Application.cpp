/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   Application.cpp                                   :+:      :+:    :+: :+:      */
/*                                                    +:+ +:+   +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+ +#+#+#+#+#+   +#+      */
/*                                                +#+#+#+#+#+   +#+           +#+#+#+#+#+   */
/*   Created: 2026/09/18          by flaviobc      #+#    #+#             */
/*   Updated: 2026/10/03          by flaviobc     ###   ########.fr       */
/*                                                                                    */
/* ******************************************************************************** */
/*                                                                                    */
/* **********************************************************************************
 */

#include "Application.hpp"

#include "animation/SkeletalPose.hpp"
#include "audio/AudioSystem.hpp"
#include "graphics/Renderer.hpp"
#include "input/KeyboardState.hpp"
#include "input/SdlKeyboardAdapter.hpp"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{
struct SdlShutdown
{
	SdlShutdown() = default;
	SdlShutdown(const SdlShutdown &) = delete;
	SdlShutdown &operator=(const SdlShutdown &) = delete;

	~SdlShutdown()
	{
		SDL_Quit();
		std::fputs("SDL shutdown complete.\n", stderr);
	}
};

struct WindowDeleter
{
	void operator()(SDL_Window *window) const
	{
		SDL_DestroyWindow(window);
		std::fputs("Window destroyed.\n", stderr);
	}
};

int reportSdlError(const char *operation)
{
	std::fprintf(stderr, "%s failed: %s\n", operation, SDL_GetError());
	return 1;
}
} // namespace

int engine::runApplication(
    ApplicationSession &session,
    const std::vector<graphics::StaticMeshInput> &staticMeshes,
    const std::vector<graphics::TransformAnimationClip> &animationClips,
    animation::SkeletalPoseSource skeletalPoseSource,
    const assets::DecodedImageRgba8 &hudIconAtlas,
    const assets::DecodedImageRgba8 &hudFontAtlas,
    const graphics::VfxSpriteImages &vfxSpriteImages,
    const std::vector<std::string> &audioClipPaths)
{
	const SdlShutdown shutdown;
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
	{
		return reportSdlError("SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)");
	}
	std::fputs("SDL video and audio initialized.\n", stderr);

	audio::AudioSystem audioSystem;
	std::string audioError;
	if (!audioSystem.initialize(audioError))
	{
		std::fprintf(stderr, "Audio initialization failed: %s\n",
		             audioError.c_str());
		return 1;
	}
	std::vector<audio::AudioClipHandle> audioClipHandles;
	audioClipHandles.reserve(audioClipPaths.size());
	for (const std::string &path : audioClipPaths)
	{
		audio::AudioClipLoadResult loaded = audioSystem.loadWav(path);
		if (!loaded)
		{
			std::fprintf(stderr, "Audio clip loading failed: %s\n",
			             loaded.error.c_str());
			return 1;
		}
		audioClipHandles.push_back(*loaded.handle);
	}
	if (!session.configureAudioClips(audioClipHandles, audioError))
	{
		std::fprintf(stderr, "Session audio configuration failed: %s\n",
		             audioError.c_str());
		return 1;
	}

	const char *backend = SDL_GetCurrentVideoDriver();
	if (!backend)
	{
		return reportSdlError("SDL_GetCurrentVideoDriver");
	}
	std::fprintf(stderr, "Video backend: %s\n", backend);

	if (!graphics::configureOpenGlWindow())
	{
		return 1;
	}
	const std::unique_ptr<SDL_Window, WindowDeleter> window(
	    SDL_CreateWindow("Bomberman - OpenGL Triangle", 800, 600,
		                 SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE));
	if (!window)
	{
		return reportSdlError("SDL_CreateWindow");
	}
	std::fputs("Window created.\n", stderr);

	const SDL_WindowID windowId = SDL_GetWindowID(window.get());
	if (windowId == 0)
	{
		return reportSdlError("SDL_GetWindowID");
	}

	graphics::Renderer renderer(*window);
	if (!renderer.initialize() ||
	    (!staticMeshes.empty() && !renderer.setStaticMeshes(staticMeshes)) ||
	    !renderer.setObjectAnimationClips(animationClips) ||
	    !renderer.setSkeletalPoseSource(std::move(skeletalPoseSource)) ||
	    !renderer.setHudIconAtlas(hudIconAtlas) ||
	    !renderer.setHudFontAtlas(hudFontAtlas) ||
	    !renderer.setVfxSpriteImages(vfxSpriteImages) ||
	    !session.initialize(renderer))
	{
		return 1;
	}

	input::KeyboardState keyboard;
	bool debugVisible = false;
	bool f1Pressed = false;

	using Clock = std::chrono::steady_clock;
	constexpr auto framePeriod = std::chrono::milliseconds(16);
	auto previousStart = Clock::now();
	bool firstFrame = true;
	double intervalSeconds = 0.0;
	double maxFrameSeconds = 0.0;
	unsigned int samples = 0;
	std::fputs(
	    "Timing: first dt=0 s; frame time=start-to-start including pacing; "
	    "target period=16 ms.\n",
	    stderr);

	bool isRunning = true;
	while (isRunning)
	{
		keyboard.beginFrame();
		const auto frameStart = Clock::now();
		const double deltaSeconds =
		    firstFrame
		        ? 0.0
		        : std::chrono::duration<double>(frameStart - previousStart)
		              .count();
		previousStart = frameStart;
		if (!firstFrame)
		{
			intervalSeconds += deltaSeconds;
			maxFrameSeconds = std::max(maxFrameSeconds, deltaSeconds);
			++samples;
		}
		firstFrame = false;

		SDL_PumpEvents();
		SDL_Event events[256];
		const int eventCount = SDL_PeepEvents(events, 256, SDL_GETEVENT,
		                                      SDL_EVENT_FIRST, SDL_EVENT_LAST);
		if (eventCount < 0)
		{
			return reportSdlError("SDL_PeepEvents");
		}
		for (int i = 0; i < eventCount; ++i)
		{
			const auto &event = events[i];
			if (event.type == SDL_EVENT_QUIT ||
			    (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
			     event.window.windowID == windowId))
			{
				std::fputs("Close requested.\n", stderr);
				isRunning = false;
				break;
			}
			if (event.type == SDL_EVENT_KEY_DOWN)
			{
				if (event.key.key == SDLK_F1)
				{
					if (!f1Pressed)
					{
						debugVisible = !debugVisible;
						std::fprintf(stderr, "Debug visualizer: %s.\n",
						             debugVisible ? "enabled" : "disabled");
					}
					f1Pressed = true;
				}
				else if (const auto key =
				             input::keyFromSdlKeycode(event.key.key))
				{
					keyboard.setPressed(*key, true);
				}
			}
			else if (event.type == SDL_EVENT_KEY_UP)
			{
				if (event.key.key == SDLK_F1)
				{
					f1Pressed = false;
				}
				else if (const auto key =
				             input::keyFromSdlKeycode(event.key.key))
				{
					keyboard.setPressed(*key, false);
				}
			}
			else if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST)
			{
				keyboard.clear();
				f1Pressed = false;
			}
		}

		if (!isRunning)
		{
			continue;
		}
		session.update(keyboard, deltaSeconds);
		for (const audio::AudioPlaybackCommand &command :
		     session.takeAudioPlaybackCommands())
		{
			audioError.clear();
			const audio::AudioPlayResult result =
			    audioSystem.playOneShot(command.clip, command.gain, audioError);
			if (result == audio::AudioPlayResult::Failed)
			{
				std::fprintf(stderr, "Audio command failed: %s\n",
				             audioError.c_str());
			}
		}
		session.applyVisualState(renderer);
		if (!renderer.renderFrame(debugVisible, deltaSeconds))
		{
			return 1;
		}

		if (intervalSeconds >= 1.0)
		{
			std::fprintf(stderr,
			             "Timing: samples=%u dt=%.6f s frame_mean=%.3f ms "
			             "frame_max=%.3f ms\n",
			             samples, deltaSeconds,
			             intervalSeconds * 1000.0 / samples,
			             maxFrameSeconds * 1000.0);
			intervalSeconds = 0.0;
			maxFrameSeconds = 0.0;
			samples = 0;
		}

		const auto remaining = frameStart + framePeriod - Clock::now();
		if (remaining > Clock::duration::zero())
		{
			const auto delay =
			    std::chrono::ceil<std::chrono::milliseconds>(remaining);
			SDL_Delay(static_cast<Uint32>(delay.count()));
		}
	}
	return 0;
}
