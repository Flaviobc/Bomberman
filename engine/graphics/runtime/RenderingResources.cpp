/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingResources.cpp                            :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "graphics/runtime/RenderingResources.hpp"

#include "graphics/runtime/OpenGlFunctions.hpp"

namespace engine::graphics::detail
{
bool RenderingResources::create(OpenGlFunctions &gl)
{
	if (!programs.create(gl))
	{
		return false;
	}
	if (!shadowMap.create(gl))
	{
		return false;
	}
	if (!postProcess.create(gl))
	{
		return false;
	}
	fullscreenQuad.create(gl);
	hudResources.create(gl);
	transientLines.create(gl);
	return vfxSprites.create(gl);
}

bool RenderingResources::resizePostProcessTargets(OpenGlFunctions &gl,
                                                  int width, int height)
{
	return postProcess.resize(gl, width, height);
}

void RenderingResources::destroy(OpenGlFunctions &gl)
{
	vfx.clear();
	vfxSprites.destroy(gl);
	programs.destroy(gl);
	shadowMap.destroy(gl);
	postProcess.destroy(gl);
	fullscreenQuad.destroy(gl);
	hudResources.destroy(gl);
	worldRectangles.destroy(gl);
	transientLines.destroy(gl);
	staticMeshes.destroy(gl);
}
} // namespace engine::graphics::detail
