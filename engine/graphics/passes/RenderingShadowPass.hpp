/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingShadowPass.hpp                           :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

#include "graphics/features/RenderingAnimation.hpp"
#include "graphics/foundation/RenderingMath.hpp"
#include "graphics/resources/RenderingStaticMeshes.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace engine::graphics::detail
{
class OpenGlFunctions;
class RenderingPrograms;
class RenderingShadowMap;

struct ShadowPassInstance
{
	std::size_t modelId = 0;
	Matrix4 modelMatrix;
};

// Executes only the existing directional depth pass. Renderer prepares the
// frame matrices and remains responsible for pass ordering and the next target.
class RenderingShadowPass
{
  public:
	static void
	execute(OpenGlFunctions &gl, const RenderingPrograms &programs,
	        const RenderingShadowMap &shadowMap,
	        RenderingStaticMeshes &staticMeshes,
	        const std::vector<ShadowPassInstance> &instances,
	        const Matrix4 &lightSpaceMatrix,
	        const Matrix4 &playerModelViewProjection,
	        const std::array<float, RenderingAnimation::jointCount * 16>
	            &jointPalette,
	        std::vector<RenderingStaticMeshes::GpuInstance> &scratch);
};
} // namespace engine::graphics::detail
