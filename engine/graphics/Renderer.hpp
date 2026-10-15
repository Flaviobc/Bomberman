/* **********************************************************************************
 */
/*                                                                                    */
/*                                                      :::::::::: :::::::::
 * :::::: */
/*   Renderer.hpp                                      :+:        :+:    :+: :+:
 * : */
/*                                                    +:+        +:+    +:+ +:+
 */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+
 */
/*                                                  +#+        +#+    +#+ +#+ */
/*   Created: 2026/09/24          by flaviobc      #+#        #+#    #+# #+# #+#
 */
/*   Updated: 2026/09/25          by flaviobc     ###        ######### ########
 */
/*                                                                                    */
/* **********************************************************************************
 */

#pragma once

#include <SDL3/SDL_video.h>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace engine::animation
{
struct SkeletalPoseSource;
}

namespace engine::assets
{
struct DecodedImageRgba8;
struct StaticMeshSource;
struct StaticMeshTexCoord;
} // namespace engine::assets

namespace engine::graphics
{
namespace detail
{
class RenderingBootstrap;
} // namespace detail

bool configureOpenGlWindow();

struct WorldRectangle
{
	float x = 0.0F;
	float y = 0.0F;
	float width = 0.0F;
	float height = 0.0F;
	float red = 1.0F;
	float green = 1.0F;
	float blue = 1.0F;
};

struct StaticMeshInput
{
	const assets::StaticMeshSource *source = nullptr;
	std::size_t modelId = 0;
	bool skinned = false;
	float red = 1.0F;
	float green = 1.0F;
	float blue = 1.0F;
	float alpha = 1.0F;
	float emissiveRed = 0.0F;
	float emissiveGreen = 0.0F;
	float emissiveBlue = 0.0F;
	const assets::DecodedImageRgba8 *baseColorImage = nullptr;
	const std::vector<assets::StaticMeshTexCoord> *baseColorTexCoords = nullptr;
};

struct WorldTransform
{
	float x = 0.0F;
	float y = 0.0F;
	float z = 0.0F;
	float rotationRadians = 0.0F;
	float scale = 1.0F;
};

// Alpha is the colorization amount. Zero preserves the authored material;
// one replaces its hue with RGB while retaining the material luminance.
struct InstanceColorization
{
	float red = 1.0F;
	float green = 1.0F;
	float blue = 1.0F;
	float amount = 0.0F;
};

// Zero intensity preserves the material emission. A positive intensity uses
// RGB as the instance emission, allowing authored geometry to be reused.
struct InstanceEmission
{
	float red = 0.0F;
	float green = 0.0F;
	float blue = 0.0F;
	float intensity = 0.0F;
};

struct StaticMeshInstance
{
	std::size_t modelId = 0;
	WorldTransform transform;
	InstanceColorization colorization;
	InstanceEmission emission;
	bool castsShadow = true;
};

struct TransformTranslationKey
{
	float timeSeconds = 0.0F;
	float x = 0.0F;
	float y = 0.0F;
	float z = 0.0F;
};

struct TransformRotationKey
{
	float timeSeconds = 0.0F;
	float x = 0.0F;
	float y = 0.0F;
	float z = 0.0F;
	float w = 1.0F;
};

struct TransformAnimationClip
{
	float durationSeconds = 0.0F;
	std::vector<TransformTranslationKey> translations;
	std::vector<TransformRotationKey> rotations;
};

enum class VfxSpriteKind
{
	Smoke,
	Fireball,
};

struct ParticleBurst
{
	WorldTransform transform;
	double durationSeconds = 0.0;
	float red = 1.0F;
	float green = 1.0F;
	float blue = 1.0F;
	int count = 0;
	VfxSpriteKind sprite = VfxSpriteKind::Smoke;
	double delaySeconds = 0.0;
};

struct VfxSpriteImages
{
	const assets::DecodedImageRgba8 &smoke;
	const assets::DecodedImageRgba8 &fireball;
};

struct PointLightBurst
{
	WorldTransform transform;
	double durationSeconds = 0.0;
	float red = 1.0F;
	float green = 1.0F;
	float blue = 1.0F;
	float intensity = 1.0F;
	float radius = 1.0F;
};

struct CameraImpulse
{
	float x = 0.0F;
	float y = 0.0F;
	float z = 0.0F;
	float amplitude = 0.0F;
	double durationSeconds = 0.0;
	float frequencyHz = 0.0F;
};

// Generic world-space footprint used only to frame the fixed scene camera.
struct SceneBounds
{
	float minimumX = 0.0F;
	float maximumX = 0.0F;
	float minimumZ = 0.0F;
	float maximumZ = 0.0F;
};

// Generic camera configuration resolved outside Rendering. Coverage is a linear
// fraction of scene bounds, where 1.0 preserves the complete-scene framing.
struct CameraFraming
{
	bool autoFrame = true;
	float coverage = 1.0F;
};

// Generic world-space tracking target. Reset discards previous interpolation.
struct CameraTrackingTarget
{
	float x = 0.0F;
	float y = 0.0F;
	float z = 0.0F;
	bool reset = false;
};

struct HudState
{
	int lifeCount = 0;
	int bombCapacity = 0;
	int activeBombCount = 0;
	int blastRange = 0;
};

struct HudText
{
	std::string content;
	float x = 0.0F;
	float y = 0.0F;
	float glyphSize = 0.0F;
};

enum class TerminalHudState
{
	Title,
	None,
	Paused,
	Success,
	Failure,
};

class Renderer
{
  public:
	Renderer(const Renderer &) = delete;
	Renderer &operator=(const Renderer &) = delete;
	Renderer(Renderer &&) = delete;
	Renderer &operator=(Renderer &&) = delete;

	explicit Renderer(SDL_Window &window);
	~Renderer();

	bool initialize();
	bool setStaticMeshes(const std::vector<StaticMeshInput> &inputs);
	bool
	setObjectAnimationClips(const std::vector<TransformAnimationClip> &clips);
	bool setSkeletalPoseSource(animation::SkeletalPoseSource source);
	bool selectObjectAnimation(std::size_t clipIndex);
	void
	setStaticMeshInstances(const std::vector<StaticMeshInstance> &instances);
	// Empty burst (count and duration zero) cancels active visual effects.
	void emitParticleBurst(const ParticleBurst &burst);
	bool setVfxSpriteImages(const VfxSpriteImages &images);
	void emitPointLight(const PointLightBurst &light);
	void emitCameraImpulse(const CameraImpulse &impulse);
	bool setSceneBounds(const SceneBounds &bounds);
	bool setCameraFraming(const CameraFraming &framing);
	bool setCameraTrackingTarget(const CameraTrackingTarget &target);
	void setHudState(const HudState &state);
	void setTerminalHudState(TerminalHudState state);
	bool setHudIconAtlas(const assets::DecodedImageRgba8 &image);
	bool setHudFontAtlas(const assets::DecodedImageRgba8 &image);
	bool setHudTexts(const std::vector<HudText> &texts);
	bool setOccupiedRectangles(const std::vector<WorldRectangle> &rectangles);
	void setObjectTransform(const WorldTransform &transform);
	bool renderFrame(bool debugVisible, double deltaSeconds);

  private:
	bool resizePostProcessTargets(int width, int height);

	SDL_Window *window_;
	std::unique_ptr<detail::RenderingBootstrap> bootstrap_;
	WorldTransform objectTransform_;
	SceneBounds sceneBounds_{-1.875F, 1.875F, -1.875F, 1.875F};
};
} // namespace engine::graphics
