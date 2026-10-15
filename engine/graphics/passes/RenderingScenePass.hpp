/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingScenePass.hpp                            :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/09          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/09          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

#include "graphics/features/RenderingAnimation.hpp"
#include "graphics/foundation/RenderingMath.hpp"
#include "graphics/resources/RenderingStaticMeshes.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace engine::graphics::detail
{
class OpenGlFunctions;
class RenderingPostProcess;
class RenderingPrograms;
class RenderingShadowMap;
class RenderingWorldRectangles;

struct ScenePassInstance
{
	std::size_t modelId = 0;
	Matrix4 modelMatrix;
	Matrix3 normalMatrix;
	std::array<float, 4> colorization{1.0F, 1.0F, 1.0F, 0.0F};
	std::array<float, 4> emission{};
};

struct ScenePassInput
{
	int viewportWidth = 0;
	int viewportHeight = 0;
	std::size_t animatedModelId = 0;
	Matrix4 worldViewProjection;
	Matrix4 playerModelViewProjection;
	Matrix4 playerModelMatrix;
	Matrix3 playerNormalMatrix;
	Matrix4 lightSpaceMatrix;
	Matrix4 worldRectangleModelMatrix;
	Matrix3 worldRectangleNormalMatrix;
	std::array<float, RenderingAnimation::jointCount * 16> jointPalette{};
	std::span<const ScenePassInstance> instances;
	bool pointLightActive = false;
	std::array<float, 4> pointLightPositionIntensity{};
	std::array<float, 4> pointLightColorRadius{};
};

// Submits the prepared HDR scene. It owns no resources and performs no frame
// simulation or matrix construction; Renderer supplies all transient data.
class RenderingScenePass
{
  public:
	static void
	execute(OpenGlFunctions &gl, const RenderingPrograms &programs,
	        const RenderingPostProcess &postProcess,
	        const RenderingShadowMap &shadowMap,
	        RenderingStaticMeshes &staticMeshes,
	        const RenderingWorldRectangles &worldRectangles,
	        const ScenePassInput &input,
	        std::vector<RenderingStaticMeshes::GpuInstance> &scratch);
};
} // namespace engine::graphics::detail
