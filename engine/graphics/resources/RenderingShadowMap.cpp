/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingShadowMap.cpp                            :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "graphics/resources/RenderingShadowMap.hpp"

#include "graphics/runtime/OpenGlFunctions.hpp"

#include <cstdio>

namespace engine::graphics::detail
{
bool RenderingShadowMap::create(OpenGlFunctions &gl)
{
	gl.genTextures(1, &depthTexture_);
	gl.bindTexture(GL_TEXTURE_2D, depthTexture_);
	gl.texImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, dimension(),
	              dimension(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	gl.genFramebuffers(1, &framebuffer_);
	gl.bindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
	                        depthTexture_, 0);
	gl.drawBuffer(GL_NONE);
	gl.readBuffer(GL_NONE);
	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::fputs("Shadow framebuffer is incomplete.\n", stderr);
		gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
		return false;
	}
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	gl.bindTexture(GL_TEXTURE_2D, 0);
	std::fprintf(stderr, "Directional shadow map initialized: %dx%d.\n",
	             dimension(), dimension());
	return true;
}

void RenderingShadowMap::destroy(OpenGlFunctions &gl)
{
	if (framebuffer_ != 0)
	{
		gl.deleteFramebuffers(1, &framebuffer_);
	}
	if (depthTexture_ != 0)
	{
		gl.deleteTextures(1, &depthTexture_);
	}
	framebuffer_ = 0;
	depthTexture_ = 0;
}

GLuint RenderingShadowMap::framebuffer() const
{
	return framebuffer_;
}

GLuint RenderingShadowMap::depthTexture() const
{
	return depthTexture_;
}
} // namespace engine::graphics::detail
