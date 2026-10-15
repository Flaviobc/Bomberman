/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingDebugPass.hpp                            :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/09          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/09          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

namespace engine::graphics::detail
{
struct DebugGeometryFrame;
struct Matrix4;
class OpenGlFunctions;
class RenderingPrograms;
class RenderingTransientLines;

// Submits one CPU-prepared debug frame in world-then-overlay order. It borrows
// the shared line buffer and leaves bindings ready for the following HUD pass.
class RenderingDebugPass
{
  public:
	static void execute(OpenGlFunctions &gl, const RenderingPrograms &programs,
	                    RenderingTransientLines &transientLines,
	                    const DebugGeometryFrame &frame,
	                    const Matrix4 &worldViewProjection);
};
} // namespace engine::graphics::detail
