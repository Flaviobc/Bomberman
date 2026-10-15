/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingDebugGeometry.hpp                        :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

#include "graphics/Renderer.hpp"

#include <array>
#include <cstddef>

namespace engine::graphics::detail
{
constexpr std::size_t debugLineVertexCapacity = 256;

struct DebugLineVertices
{
	void addLine(float x0, float y0, float z0, float x1, float y1, float z1,
	             float red, float green, float blue);

	std::array<float, debugLineVertexCapacity * 6> values{};
	std::size_t count = 0;
};

struct DebugGeometryFrame
{
	DebugLineVertices world;
	DebugLineVertices overlay;
};

class RenderingDebugGeometry
{
  public:
	[[nodiscard]] static DebugGeometryFrame
	build(bool visible, const WorldTransform &objectTransform,
	      double deltaSeconds);
};
} // namespace engine::graphics::detail
