/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingDebugGeometry.cpp                        :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "graphics/features/RenderingDebugGeometry.hpp"

#include <algorithm>
#include <cmath>

namespace
{
using engine::graphics::detail::DebugLineVertices;

void appendDigit(DebugLineVertices &vertices, int digit, float x, float y,
                 float red, float green, float blue)
{
	static constexpr bool segments[10][7] = {
	    {true, true, true, true, true, true, false},
	    {false, true, true, false, false, false, false},
	    {true, true, false, true, true, false, true},
	    {true, true, true, true, false, false, true},
	    {false, true, true, false, false, true, true},
	    {true, false, true, true, false, true, true},
	    {true, false, true, true, true, true, true},
	    {true, true, true, false, false, false, false},
	    {true, true, true, true, true, true, true},
	    {true, true, true, true, false, true, true},
	};
	static constexpr float endpoints[7][4] = {
	    {0.0F, 0.08F, 0.04F, 0.08F}, {0.04F, 0.08F, 0.04F, 0.04F},
	    {0.04F, 0.04F, 0.04F, 0.0F}, {0.0F, 0.0F, 0.04F, 0.0F},
	    {0.0F, 0.04F, 0.0F, 0.0F},   {0.0F, 0.08F, 0.0F, 0.04F},
	    {0.0F, 0.04F, 0.04F, 0.04F},
	};
	for (int segment = 0; segment < 7; ++segment)
	{
		if (!segments[digit][segment])
		{
			continue;
		}
		const float *line = endpoints[segment];
		vertices.addLine(x + line[0], y + line[1], 0.0F, x + line[2],
		                 y + line[3], 0.0F, red, green, blue);
	}
}

void appendNumber(DebugLineVertices &vertices, int value, float y, float red,
                  float green, float blue)
{
	const int clampedValue = std::clamp(value, 0, 999);
	appendDigit(vertices, clampedValue / 100, -0.94F, y, red, green, blue);
	appendDigit(vertices, (clampedValue / 10) % 10, -0.88F, y, red, green,
	            blue);
	appendDigit(vertices, clampedValue % 10, -0.82F, y, red, green, blue);
}
} // namespace

namespace engine::graphics::detail
{
void DebugLineVertices::addLine(float x0, float y0, float z0, float x1,
                                float y1, float z1, float red, float green,
                                float blue)
{
	if (count + 2 > debugLineVertexCapacity)
	{
		return;
	}
	const std::size_t offset = count * 6;
	values[offset] = x0;
	values[offset + 1] = y0;
	values[offset + 2] = z0;
	values[offset + 3] = red;
	values[offset + 4] = green;
	values[offset + 5] = blue;
	values[offset + 6] = x1;
	values[offset + 7] = y1;
	values[offset + 8] = z1;
	values[offset + 9] = red;
	values[offset + 10] = green;
	values[offset + 11] = blue;
	count += 2;
}

DebugGeometryFrame RenderingDebugGeometry::build(
    bool visible, const WorldTransform &objectTransform, double deltaSeconds)
{
	DebugGeometryFrame frame;
	if (!visible)
	{
		return frame;
	}
	constexpr float gridSpacing = 0.25F;
	for (int step = -4; step <= 4; ++step)
	{
		const float coordinate = static_cast<float>(step) * gridSpacing;
		frame.world.addLine(coordinate, 0.0F, -1.0F, coordinate, 0.0F, 1.0F,
		                    0.2F, 0.35F, 0.45F);
		frame.world.addLine(-1.0F, 0.0F, -coordinate, 1.0F, 0.0F, -coordinate,
		                    0.2F, 0.35F, 0.45F);
	}
	constexpr float markerRadius = 0.16F;
	frame.world.addLine(objectTransform.x - markerRadius, objectTransform.y,
	                    objectTransform.z, objectTransform.x + markerRadius,
	                    objectTransform.y, objectTransform.z, 1.0F, 0.3F, 0.3F);
	frame.world.addLine(objectTransform.x, objectTransform.y,
	                    objectTransform.z - markerRadius, objectTransform.x,
	                    objectTransform.y, objectTransform.z + markerRadius,
	                    0.3F, 1.0F, 0.3F);
	frame.world.addLine(objectTransform.x, objectTransform.y, objectTransform.z,
	                    objectTransform.x, objectTransform.y,
	                    objectTransform.z - markerRadius * 1.5F, 1.0F, 0.9F,
	                    0.2F);

	const int framesPerSecond =
	    deltaSeconds > 0.0 ? static_cast<int>(std::lround(1.0 / deltaSeconds))
		                   : 0;
	const int milliseconds =
	    static_cast<int>(std::lround(deltaSeconds * 1000.0));
	appendNumber(frame.overlay, framesPerSecond, 0.82F, 0.2F, 0.9F, 1.0F);
	appendNumber(frame.overlay, milliseconds, 0.68F, 1.0F, 0.65F, 0.2F);
	return frame;
}
} // namespace engine::graphics::detail
