/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   Renderer.cpp                                      :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/09/24          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/09/25          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "graphics/Renderer.hpp"

#include "graphics/features/RenderingAnimation.hpp"
#include "graphics/features/RenderingCamera.hpp"
#include "graphics/features/RenderingDebugGeometry.hpp"
#include "graphics/features/RenderingDirectionalShadow.hpp"
#include "graphics/features/RenderingHud.hpp"
#include "graphics/features/RenderingPointLight.hpp"
#include "graphics/features/RenderingWorldGeometry.hpp"
#include "graphics/features/VfxSystem.hpp"
#include "graphics/foundation/RenderingMath.hpp"
#include "graphics/foundation/RenderingTransforms.hpp"
#include "graphics/passes/RenderingDebugPass.hpp"
#include "graphics/passes/RenderingHudPass.hpp"
#include "graphics/passes/RenderingPostProcessPass.hpp"
#include "graphics/passes/RenderingScenePass.hpp"
#include "graphics/passes/RenderingShadowPass.hpp"
#include "graphics/passes/RenderingVfxPass.hpp"
#include "graphics/resources/RenderingFullscreenQuad.hpp"
#include "graphics/resources/RenderingHudResources.hpp"
#include "graphics/resources/RenderingPostProcess.hpp"
#include "graphics/resources/RenderingPrograms.hpp"
#include "graphics/resources/RenderingShadowMap.hpp"
#include "graphics/resources/RenderingStaticMeshes.hpp"
#include "graphics/resources/RenderingTransientLines.hpp"
#include "graphics/resources/RenderingWorldRectangles.hpp"
#include "graphics/runtime/OpenGlFunctions.hpp"
#include "graphics/runtime/RenderingBootstrap.hpp"
#include "graphics/runtime/RenderingResources.hpp"

#include "animation/SkeletalPose.hpp"
#include "assets/GlbLoader.hpp"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_opengl.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <memory>
#include <optional>
#include <utility>

namespace
{
constexpr int kOpenGlMajorVersion = 3;
constexpr int kOpenGlMinorVersion = 3;

using engine::graphics::detail::AnimationPlaybackFrame;
using engine::graphics::detail::identityMatrix;
using engine::graphics::detail::makeDirectionalShadowFrustum;
using engine::graphics::detail::makeNormalMatrix;
using engine::graphics::detail::makeQuaternionRotationMatrix;
using engine::graphics::detail::makeTranslationMatrix;
using engine::graphics::detail::makeUniformScaleMatrix;
using engine::graphics::detail::makeWorldModelMatrix;
using engine::graphics::detail::makeYRotationMatrix;
using engine::graphics::detail::Matrix3;
using engine::graphics::detail::Matrix4;
using engine::graphics::detail::multiplyMatrices;
using engine::graphics::detail::normalizeVector;
using engine::graphics::detail::Quaternion;
using engine::graphics::detail::RenderingAnimation;
using engine::graphics::detail::RenderingCamera;
using engine::graphics::detail::RenderingDebugGeometry;
using engine::graphics::detail::RenderingHud;
using engine::graphics::detail::RenderingWorldGeometry;
using engine::graphics::detail::Vector3;

bool reportSdlError(const char *operation)
{
	std::fprintf(stderr, "%s failed: %s\n", operation, SDL_GetError());
	return false;
}

bool setGlAttribute(SDL_GLAttr attribute, int value, const char *name)
{
	if (!SDL_GL_SetAttribute(attribute, value))
	{
		return reportSdlError(name);
	}
	return true;
}

} // namespace

namespace engine::graphics
{
bool configureOpenGlWindow()
{
	if (!setGlAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, kOpenGlMajorVersion,
	                    "SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION)") ||
	    !setGlAttribute(SDL_GL_CONTEXT_MINOR_VERSION, kOpenGlMinorVersion,
	                    "SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION)") ||
	    !setGlAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
	                    SDL_GL_CONTEXT_PROFILE_CORE,
	                    "SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK)") ||
	    !setGlAttribute(SDL_GL_DOUBLEBUFFER, 1,
	                    "SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER)") ||
	    !setGlAttribute(SDL_GL_DEPTH_SIZE, 24,
	                    "SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE)"))
	{
		return false;
	}

#ifndef NDEBUG
	if (!setGlAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG,
	                    "SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS)"))
	{
		return false;
	}
#endif
	return true;
}

Renderer::Renderer(SDL_Window &window)
    : window_(&window),
      bootstrap_(std::make_unique<detail::RenderingBootstrap>(window))
{
}

Renderer::~Renderer()
{
	bootstrap_->shutdown();
}

bool Renderer::initialize()
{
	return bootstrap_->initialize();
}

bool Renderer::resizePostProcessTargets(int width, int height)
{
	if (!bootstrap_->hasResources() || !bootstrap_->hasFunctions())
	{
		std::fputs("HDR post-process targets are invalid.\n", stderr);
		return false;
	}
	return bootstrap_->resources().resizePostProcessTargets(
	    bootstrap_->functions(), width, height);
}

bool Renderer::setStaticMeshes(const std::vector<StaticMeshInput> &inputs)
{
	if (!bootstrap_->hasResources() || !bootstrap_->hasFunctions())
	{
		std::fputs("Static meshes require one initialized upload.\n", stderr);
		return false;
	}
	return bootstrap_->resources().staticMeshes.upload(bootstrap_->functions(),
	                                                   inputs);
}
bool Renderer::setObjectAnimationClips(
    const std::vector<TransformAnimationClip> &clips)
{
	if (!bootstrap_->hasResources())
	{
		std::fputs("Object animation collection is invalid.\n", stderr);
		return false;
	}
	return bootstrap_->resources().animationPlayback.setTransformClips(clips);
}

bool Renderer::setSkeletalPoseSource(animation::SkeletalPoseSource source)
{
	if (!bootstrap_->hasResources())
	{
		std::fputs("Skeletal pose source is invalid for this renderer.\n",
		           stderr);
		return false;
	}
	return bootstrap_->resources().animationPlayback.setSkeletalPoseSource(
	    std::move(source));
}

bool Renderer::selectObjectAnimation(std::size_t clipIndex)
{
	if (!bootstrap_->hasResources())
	{
		std::fprintf(stderr, "Object animation index is invalid: %zu.\n",
		             clipIndex);
		return false;
	}
	return bootstrap_->resources().animationPlayback.selectClip(clipIndex);
}

void Renderer::setStaticMeshInstances(
    const std::vector<StaticMeshInstance> &instances)
{
	bootstrap_->resources().staticMeshInstances = instances;
}

void Renderer::emitParticleBurst(const ParticleBurst &burst)
{
	bootstrap_->resources().vfx.emit(burst);
}

bool Renderer::setVfxSpriteImages(const VfxSpriteImages &images)
{
	if (!bootstrap_->hasResources() || !bootstrap_->hasFunctions())
	{
		std::fputs("VFX sprite image requires an initialized renderer.\n",
		           stderr);
		return false;
	}
	return bootstrap_->resources().vfxSprites.uploadImages(
	    bootstrap_->functions(), images);
}

void Renderer::emitPointLight(const PointLightBurst &light)
{
	bootstrap_->resources().pointLight.emit(light);
}

void Renderer::emitCameraImpulse(const CameraImpulse &impulse)
{
	if (impulse.amplitude <= 0.0F || impulse.durationSeconds <= 0.0 ||
	    impulse.frequencyHz <= 0.0F)
	{
		std::fputs("Camera impulse ignored: amplitude, duration, and frequency "
		           "must be positive.\n",
		           stderr);
		return;
	}
	bootstrap_->resources().camera.replaceImpulse(
	    impulse.x, impulse.y, impulse.z, impulse.amplitude,
	    impulse.durationSeconds, impulse.frequencyHz);
	std::fprintf(stderr,
	             "Camera impulse started: position=(%.3f, %.3f, %.3f) "
	             "amplitude=%.3f duration=%.3f s frequency=%.3f Hz.\n",
	             impulse.x, impulse.y, impulse.z, impulse.amplitude,
	             impulse.durationSeconds, impulse.frequencyHz);
}

bool Renderer::setSceneBounds(const SceneBounds &bounds)
{
	if (!bootstrap_ ||
	    !bootstrap_->resources().camera.setSceneBounds(
	        bounds.minimumX, bounds.maximumX, bounds.minimumZ, bounds.maximumZ))
	{
		std::fputs("Renderer rejected invalid scene bounds.\n", stderr);
		return false;
	}
	sceneBounds_ = bounds;
	return true;
}

bool Renderer::setCameraFraming(const CameraFraming &framing)
{
	if (!bootstrap_ || !bootstrap_->resources().camera.setFraming(
	                       framing.autoFrame, framing.coverage))
	{
		std::fputs("Renderer rejected invalid camera framing.\n", stderr);
		return false;
	}
	return true;
}

bool Renderer::setCameraTrackingTarget(const CameraTrackingTarget &target)
{
	if (!bootstrap_ || !bootstrap_->resources().camera.setTrackingTarget(
	                       target.x, target.y, target.z, target.reset))
	{
		std::fputs("Renderer rejected invalid camera tracking target.\n",
		           stderr);
		return false;
	}
	return true;
}

void Renderer::setHudState(const HudState &state)
{
	bootstrap_->resources().hud.setState(state);
}

void Renderer::setTerminalHudState(TerminalHudState state)
{
	bootstrap_->resources().hud.setTerminalState(state);
}

bool Renderer::setHudIconAtlas(const assets::DecodedImageRgba8 &image)
{
	if (!bootstrap_->hasResources() || !bootstrap_->hasFunctions())
	{
		std::fputs("HUD icon atlas requires one valid RGBA8 upload.\n", stderr);
		return false;
	}
	return bootstrap_->resources().hudResources.uploadIconAtlas(
	    bootstrap_->functions(), image, RenderingHud::iconAtlasVertices());
}

bool Renderer::setHudFontAtlas(const assets::DecodedImageRgba8 &image)
{
	if (!bootstrap_->hasResources() || !bootstrap_->hasFunctions())
	{
		std::fputs("HUD font atlas requires one 256x48 RGBA8 upload.\n",
		           stderr);
		return false;
	}
	return bootstrap_->resources().hudResources.uploadFontAtlas(
	    bootstrap_->functions(), image);
}

bool Renderer::setHudTexts(const std::vector<HudText> &texts)
{
	if (!bootstrap_->hasResources() || !bootstrap_->hasFunctions() ||
	    bootstrap_->resources().hudResources.fontAtlasTexture() == 0)
	{
		std::fputs("HUD text requires a loaded font atlas.\n", stderr);
		return false;
	}
	const detail::HudTextUpdateResult update =
	    bootstrap_->resources().hud.setTexts(texts);
	if (update.error == detail::HudTextUpdateError::NonPositiveGlyphSize)
	{
		std::fputs("HUD text glyph size must be positive.\n", stderr);
		return false;
	}
	if (update.error == detail::HudTextUpdateError::UnsupportedGlyph)
	{
		std::fprintf(stderr, "HUD text has unsupported glyph: %c\n",
		             update.unsupportedGlyph);
		return false;
	}
	const std::vector<float> &vertices =
	    bootstrap_->resources().hud.glyphVertices();
	bootstrap_->resources().hudResources.updateGlyphs(bootstrap_->functions(),
	                                                  vertices, texts.size());
	return true;
}

bool Renderer::setOccupiedRectangles(
    const std::vector<WorldRectangle> &rectangles)
{
	if (!bootstrap_->hasResources() || !bootstrap_->hasFunctions())
	{
		std::fputs("Occupied rectangles require an initialized renderer.\n",
		           stderr);
		return false;
	}
	const std::vector<float> vertices =
	    RenderingWorldGeometry::buildRectangles(rectangles);

	bootstrap_->resources().worldRectangles.upload(bootstrap_->functions(),
	                                               vertices);
	return true;
}

void Renderer::setObjectTransform(const WorldTransform &transform)
{
	objectTransform_ = transform;
}

bool Renderer::renderFrame(bool debugVisible, double deltaSeconds)
{
	int width = 0;
	int height = 0;
	if (!SDL_GetWindowSizeInPixels(window_, &width, &height))
	{
		return reportSdlError("SDL_GetWindowSizeInPixels");
	}
	if (width == 0 || height == 0)
	{
		return true;
	}
	if (!resizePostProcessTargets(width, height))
	{
		return false;
	}
	if (bootstrap_->resources().hud.terminalState() == TerminalHudState::Title)
	{
		const detail::HudDrawPlan hudPlan =
		    bootstrap_->resources().hud.buildDrawPlan();
		detail::RenderingHudPass::executeTitleFrame(
		    bootstrap_->functions(), bootstrap_->resources().programs,
		    bootstrap_->resources().hudResources,
		    bootstrap_->resources().transientLines, hudPlan,
		    detail::HudTitleFrameInput{width, height});
		if (!SDL_GL_SwapWindow(window_))
		{
			return reportSdlError("SDL_GL_SwapWindow");
		}
		return true;
	}
	if (bootstrap_->resources().hud.terminalState() == TerminalHudState::Paused)
	{
		deltaSeconds = 0.0;
	}
	const std::optional<AnimationPlaybackFrame> animationFrame =
	    bootstrap_->resources().animationPlayback.update(deltaSeconds);
	if (!animationFrame)
	{
		return false;
	}
	const Vector3 &localTranslation = animationFrame->localTranslation;
	const Quaternion &localRotation = animationFrame->localRotation;
	const Matrix4 playerWorldRotation =
	    multiplyMatrices(makeYRotationMatrix(objectTransform_.rotationRadians),
		                 makeQuaternionRotationMatrix(localRotation));
	const Matrix4 playerModel = multiplyMatrices(
	    makeTranslationMatrix(objectTransform_.x, objectTransform_.y,
		                      objectTransform_.z),
	    multiplyMatrices(
	        makeYRotationMatrix(objectTransform_.rotationRadians),
	        multiplyMatrices(
	            makeTranslationMatrix(localTranslation.x, localTranslation.y,
				                      localTranslation.z),
	            multiplyMatrices(
	                makeQuaternionRotationMatrix(localRotation),
	                makeUniformScaleMatrix(objectTransform_.scale)))));
	const RenderingCamera::Matrices cameraMatrices =
	    bootstrap_->resources().camera.update(width, height, deltaSeconds);
	const Matrix4 &worldViewProjection = cameraMatrices.worldViewProjection;
	const Matrix4 playerModelViewProjection =
	    multiplyMatrices(worldViewProjection, playerModel);
	const Matrix3 playerNormalMatrix = makeNormalMatrix(playerWorldRotation);
	const Vector3 directionalLight = normalizeVector({-0.45F, 0.8F, 0.35F});
	const auto shadowFrustum = makeDirectionalShadowFrustum(
	    sceneBounds_.minimumX, sceneBounds_.maximumX, sceneBounds_.minimumZ,
	    sceneBounds_.maximumZ, directionalLight);
	if (!shadowFrustum)
	{
		std::fputs("Directional shadow frustum calculation failed.\n", stderr);
		return false;
	}
	const Matrix4 &lightSpaceMatrix = shadowFrustum->lightSpace;
	const Matrix4 shadowPlayerModelViewProjection =
	    multiplyMatrices(lightSpaceMatrix, playerModel);
	bootstrap_->resources().shadowPassInstances.clear();
	bootstrap_->resources().shadowPassInstances.reserve(
	    bootstrap_->resources().staticMeshInstances.size());
	for (const StaticMeshInstance &instance :
	     bootstrap_->resources().staticMeshInstances)
	{
		if (!instance.castsShadow)
		{
			continue;
		}
		bootstrap_->resources().shadowPassInstances.push_back(
		    {instance.modelId,
			 makeWorldModelMatrix(instance.transform.x, instance.transform.y,
			                      instance.transform.z,
			                      instance.transform.rotationRadians,
			                      instance.transform.scale)});
	}

	detail::RenderingShadowPass::execute(
	    bootstrap_->functions(), bootstrap_->resources().programs,
	    bootstrap_->resources().shadowMap, bootstrap_->resources().staticMeshes,
	    bootstrap_->resources().shadowPassInstances, lightSpaceMatrix,
	    shadowPlayerModelViewProjection, animationFrame->jointPalette,
	    bootstrap_->resources().staticMeshGpuInstances);
	bootstrap_->resources().pointLight.advance(deltaSeconds);
	const auto &activePointLight = bootstrap_->resources().pointLight.active();
	std::array<float, 4> pointLightPositionIntensity{};
	std::array<float, 4> pointLightColorRadius{};
	if (activePointLight)
	{
		const PointLightBurst &light = activePointLight->burst;
		pointLightPositionIntensity = {light.transform.x, light.transform.y,
		                               light.transform.z, light.intensity};
		pointLightColorRadius = {light.red, light.green, light.blue,
		                         light.radius};
	}

	bootstrap_->resources().scenePassInstances.clear();
	bootstrap_->resources().scenePassInstances.reserve(
	    bootstrap_->resources().staticMeshInstances.size());
	for (const StaticMeshInstance &instance :
	     bootstrap_->resources().staticMeshInstances)
	{
		const Matrix4 modelMatrix = makeWorldModelMatrix(
		    instance.transform.x, instance.transform.y, instance.transform.z,
		    instance.transform.rotationRadians, instance.transform.scale);
		bootstrap_->resources().scenePassInstances.push_back(
		    {instance.modelId,
			 modelMatrix,
			 makeNormalMatrix(instance.transform.rotationRadians),
			 {instance.colorization.red, instance.colorization.green,
			  instance.colorization.blue, instance.colorization.amount},
			 {instance.emission.red, instance.emission.green,
			  instance.emission.blue, instance.emission.intensity}});
	}

	// Spawn requests were consumed before the frame. Update, expire, then
	// prepare sprites.
	bootstrap_->resources().vfx.update(deltaSeconds);
	const auto spriteVertices =
	    bootstrap_->resources().vfx.buildSprites(cameraMatrices.view);

	const detail::ScenePassInput scenePassInput{
	    .viewportWidth = width,
	    .viewportHeight = height,
	    .animatedModelId = 0,
	    .worldViewProjection = worldViewProjection,
	    .playerModelViewProjection = playerModelViewProjection,
	    .playerModelMatrix = playerModel,
	    .playerNormalMatrix = playerNormalMatrix,
	    .lightSpaceMatrix = lightSpaceMatrix,
	    .worldRectangleModelMatrix = identityMatrix(),
	    .worldRectangleNormalMatrix = makeNormalMatrix(0.0F),
	    .jointPalette = animationFrame->jointPalette,
	    .instances = std::span<const detail::ScenePassInstance>(
	        bootstrap_->resources().scenePassInstances),
	    .pointLightActive = activePointLight.has_value(),
	    .pointLightPositionIntensity = pointLightPositionIntensity,
	    .pointLightColorRadius = pointLightColorRadius};
	detail::RenderingScenePass::execute(
	    bootstrap_->functions(), bootstrap_->resources().programs,
	    bootstrap_->resources().postProcess, bootstrap_->resources().shadowMap,
	    bootstrap_->resources().staticMeshes,
	    bootstrap_->resources().worldRectangles, scenePassInput,
	    bootstrap_->resources().staticMeshGpuInstances);

	detail::RenderingVfxPass::execute(
	    bootstrap_->functions(), bootstrap_->resources().programs,
	    bootstrap_->resources().postProcess, bootstrap_->resources().vfxSprites,
	    worldViewProjection, spriteVertices);

	detail::RenderingPostProcessPass::execute(
	    bootstrap_->functions(), bootstrap_->resources().programs,
	    bootstrap_->resources().postProcess,
	    bootstrap_->resources().fullscreenQuad,
	    detail::PostProcessPassInput{width, height});
	if (debugVisible)
	{
		const detail::DebugGeometryFrame debugFrame =
		    RenderingDebugGeometry::build(true, objectTransform_, deltaSeconds);
		detail::RenderingDebugPass::execute(
		    bootstrap_->functions(), bootstrap_->resources().programs,
		    bootstrap_->resources().transientLines, debugFrame,
		    worldViewProjection);
	}
	const detail::HudDrawPlan hudPlan =
	    bootstrap_->resources().hud.buildDrawPlan();
	detail::RenderingHudPass::executeOverlay(
	    bootstrap_->functions(), bootstrap_->resources().programs,
	    bootstrap_->resources().hudResources,
	    bootstrap_->resources().transientLines, hudPlan);
	if (!SDL_GL_SwapWindow(window_))
	{
		return reportSdlError("SDL_GL_SwapWindow");
	}
	return true;
}

} // namespace engine::graphics
