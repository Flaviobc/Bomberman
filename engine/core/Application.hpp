/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   Application.hpp                                   :+:      :+:    :+: :+:      */
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

#pragma once

#include "audio/AudioSystem.hpp"

#include <string>
#include <vector>

namespace engine::animation
{
struct SkeletalPoseSource;
}

namespace engine::assets
{
struct DecodedImageRgba8;
}

namespace engine::graphics
{
class Renderer;
struct StaticMeshInput;
struct StaticMeshInstance;
struct TransformAnimationClip;
struct VfxSpriteImages;
} // namespace engine::graphics

namespace engine::input
{
class KeyboardState;
}

namespace engine
{
class ApplicationSession
{
  public:
	virtual ~ApplicationSession() = default;
	virtual bool initialize(graphics::Renderer &renderer) = 0;
	virtual void update(const input::KeyboardState &keyboard,
	                    double deltaSeconds) = 0;
	virtual void applyVisualState(graphics::Renderer &renderer) const = 0;
	virtual bool
	configureAudioClips(const std::vector<audio::AudioClipHandle> &handles,
	                    std::string &error)
	{
		(void)handles;
		(void)error;
		return true;
	}
	virtual std::vector<audio::AudioPlaybackCommand> takeAudioPlaybackCommands()
	{
		return {};
	}
};

// Call on the main thread; returns zero after a normal close request.
int runApplication(
    ApplicationSession &session,
    const std::vector<graphics::StaticMeshInput> &staticMeshes,
    const std::vector<graphics::TransformAnimationClip> &animationClips,
    animation::SkeletalPoseSource skeletalPoseSource,
    const assets::DecodedImageRgba8 &hudIconAtlas,
    const assets::DecodedImageRgba8 &hudFontAtlas,
    const graphics::VfxSpriteImages &vfxSpriteImages,
    const std::vector<std::string> &audioClipPaths);
} // namespace engine
