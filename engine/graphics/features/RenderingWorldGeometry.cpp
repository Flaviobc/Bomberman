/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingWorldGeometry.cpp                        :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "graphics/features/RenderingWorldGeometry.hpp"

#include <iterator>

namespace
{
constexpr float kWorldRectangleElevation = 0.002F;
}

namespace engine::graphics::detail
{
std::vector<float> RenderingWorldGeometry::buildRectangles(
    const std::vector<WorldRectangle> &rectangles)
{
	std::vector<float> vertices;
	vertices.reserve(rectangles.size() * 36);
	for (const WorldRectangle &rectangle : rectangles)
	{
		const float left = rectangle.x;
		const float right = rectangle.x + rectangle.width;
		const float front = -rectangle.y;
		const float back = -(rectangle.y + rectangle.height);
		const float rectangleVertices[] = {
		    left,          kWorldRectangleElevation, front,
		    rectangle.red, rectangle.green,          rectangle.blue,
		    right,         kWorldRectangleElevation, front,
		    rectangle.red, rectangle.green,          rectangle.blue,
		    right,         kWorldRectangleElevation, back,
		    rectangle.red, rectangle.green,          rectangle.blue,
		    left,          kWorldRectangleElevation, front,
		    rectangle.red, rectangle.green,          rectangle.blue,
		    right,         kWorldRectangleElevation, back,
		    rectangle.red, rectangle.green,          rectangle.blue,
		    left,          kWorldRectangleElevation, back,
		    rectangle.red, rectangle.green,          rectangle.blue,
		};
		vertices.insert(vertices.end(), std::begin(rectangleVertices),
		                std::end(rectangleVertices));
	}
	return vertices;
}
} // namespace engine::graphics::detail
