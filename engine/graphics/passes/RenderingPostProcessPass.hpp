/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingPostProcessPass.hpp                      :+:        :+:    :+: :+:    : */
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
class OpenGlFunctions;
class RenderingFullscreenQuad;
class RenderingPostProcess;
class RenderingPrograms;

struct PostProcessPassInput
{
	int viewportWidth = 0;
	int viewportHeight = 0;
};

// Executes the fixed bloom blur and HDR composition sequence. It borrows every
// GPU resource and leaves the main program ready for debug and HUD overlays.
class RenderingPostProcessPass
{
  public:
	static void execute(OpenGlFunctions &gl, const RenderingPrograms &programs,
	                    const RenderingPostProcess &postProcess,
	                    const RenderingFullscreenQuad &fullscreenQuad,
	                    const PostProcessPassInput &input);
};
} // namespace engine::graphics::detail
