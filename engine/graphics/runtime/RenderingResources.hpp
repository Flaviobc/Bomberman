/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingResources.hpp                            :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

#include "graphics/features/RenderingAnimation.hpp"
#include "graphics/features/RenderingCamera.hpp"
#include "graphics/features/RenderingHud.hpp"
#include "graphics/features/RenderingPointLight.hpp"
#include "graphics/features/VfxSystem.hpp"
#include "graphics/passes/RenderingScenePass.hpp"
#include "graphics/passes/RenderingShadowPass.hpp"
#include "graphics/resources/RenderingFullscreenQuad.hpp"
#include "graphics/resources/RenderingHudResources.hpp"
#include "graphics/resources/RenderingPostProcess.hpp"
#include "graphics/resources/RenderingPrograms.hpp"
#include "graphics/resources/RenderingShadowMap.hpp"
#include "graphics/resources/RenderingStaticMeshes.hpp"
#include "graphics/resources/RenderingTransientLines.hpp"
#include "graphics/resources/RenderingVfxSprites.hpp"
#include "graphics/resources/RenderingWorldRectangles.hpp"

#include "graphics/Renderer.hpp"

#include <vector>

namespace engine::graphics::detail
{
class OpenGlFunctions;

// Composes Rendering's internal CPU/GPU state. Lifecycle methods are explicit
// because RenderingBootstrap keeps its OpenGL context current around both
// calls.
struct RenderingResources
{
	[[nodiscard]] bool create(OpenGlFunctions &gl);
	[[nodiscard]] bool resizePostProcessTargets(OpenGlFunctions &gl, int width,
	                                            int height);
	void destroy(OpenGlFunctions &gl);

	RenderingStaticMeshes staticMeshes;
	std::vector<StaticMeshInstance> staticMeshInstances;
	std::vector<ShadowPassInstance> shadowPassInstances;
	std::vector<ScenePassInstance> scenePassInstances;
	std::vector<RenderingStaticMeshes::GpuInstance> staticMeshGpuInstances;
	RenderingAnimation animationPlayback;
	VfxSystem vfx;
	RenderingVfxSprites vfxSprites;
	RenderingPointLight pointLight;
	RenderingPostProcess postProcess;
	RenderingPrograms programs;
	RenderingShadowMap shadowMap;
	RenderingFullscreenQuad fullscreenQuad;
	RenderingCamera camera;
	RenderingHud hud;
	RenderingHudResources hudResources;
	RenderingWorldRectangles worldRectangles;
	RenderingTransientLines transientLines;
};
} // namespace engine::graphics::detail
