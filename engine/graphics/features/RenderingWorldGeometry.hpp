/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingWorldGeometry.hpp                        :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

#include "graphics/Renderer.hpp"

#include <vector>

namespace engine::graphics::detail
{
// Converts Arena's X/Y plane to Rendering's X/Z plane. The small fixed Y
// elevation keeps these generic markers above the floor without z-fighting.
class RenderingWorldGeometry
{
  public:
	[[nodiscard]] static std::vector<float>
	buildRectangles(const std::vector<WorldRectangle> &rectangles);
};
} // namespace engine::graphics::detail
