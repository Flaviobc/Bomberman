/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingPostProcess.cpp                          :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "graphics/resources/RenderingPostProcess.hpp"

#include "graphics/runtime/OpenGlFunctions.hpp"

#include <cstdio>

namespace engine::graphics::detail
{
bool RenderingPostProcess::create(OpenGlFunctions &gl)
{
	const auto configureColorTexture = [&gl](GLuint &texture)
	{
		gl.genTextures(1, &texture);
		gl.bindTexture(GL_TEXTURE_2D, texture);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	};

	gl.genFramebuffers(1, &hdrFramebuffer_);
	gl.bindFramebuffer(GL_FRAMEBUFFER, hdrFramebuffer_);
	configureColorTexture(sceneTexture_);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
	                        sceneTexture_, 0);
	configureColorTexture(bloomTexture_);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
	                        bloomTexture_, 0);
	gl.genTextures(1, &depthTexture_);
	gl.bindTexture(GL_TEXTURE_2D, depthTexture_);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
	                        depthTexture_, 0);
	const GLenum hdrAttachments[] = {GL_COLOR_ATTACHMENT0,
	                                 GL_COLOR_ATTACHMENT1};
	gl.drawBuffers(2, hdrAttachments);
	for (std::size_t index = 0; index < blurFramebuffers_.size(); ++index)
	{
		gl.genFramebuffers(1, &blurFramebuffers_[index]);
		gl.bindFramebuffer(GL_FRAMEBUFFER, blurFramebuffers_[index]);
		configureColorTexture(blurTextures_[index]);
		gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		                        GL_TEXTURE_2D, blurTextures_[index], 0);
		gl.drawBuffer(GL_COLOR_ATTACHMENT0);
	}
	return true;
}

bool RenderingPostProcess::resize(OpenGlFunctions &gl, int width, int height)
{
	if (width <= 0 || height <= 0)
	{
		std::fputs("HDR post-process targets are invalid.\n", stderr);
		return false;
	}
	if (width_ == width && height_ == height)
	{
		return true;
	}
	const auto allocateColorTexture = [&gl, width, height](GLuint texture)
	{
		gl.bindTexture(GL_TEXTURE_2D, texture);
		gl.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA,
		              GL_FLOAT, nullptr);
	};
	allocateColorTexture(sceneTexture_);
	allocateColorTexture(bloomTexture_);
	for (const GLuint texture : blurTextures_)
	{
		allocateColorTexture(texture);
	}
	gl.bindTexture(GL_TEXTURE_2D, depthTexture_);
	gl.texImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0,
	              GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	const GLenum hdrAttachments[] = {GL_COLOR_ATTACHMENT0,
	                                 GL_COLOR_ATTACHMENT1};
	gl.bindFramebuffer(GL_FRAMEBUFFER, hdrFramebuffer_);
	gl.drawBuffers(2, hdrAttachments);
	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::fputs("HDR framebuffer is incomplete.\n", stderr);
		gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
		return false;
	}
	for (const GLuint framebuffer : blurFramebuffers_)
	{
		gl.bindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		gl.drawBuffer(GL_COLOR_ATTACHMENT0);
		if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) !=
		    GL_FRAMEBUFFER_COMPLETE)
		{
			std::fputs("Bloom blur framebuffer is incomplete.\n", stderr);
			gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
			return false;
		}
	}
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	gl.bindTexture(GL_TEXTURE_2D, 0);
	width_ = width;
	height_ = height;
	std::fprintf(stderr, "HDR bloom targets resized: %dx%d format=RGBA16F.\n",
	             width, height);
	return true;
}

void RenderingPostProcess::destroy(OpenGlFunctions &gl)
{
	if (hdrFramebuffer_ != 0)
	{
		gl.deleteFramebuffers(1, &hdrFramebuffer_);
	}
	for (const GLuint framebuffer : blurFramebuffers_)
	{
		if (framebuffer != 0)
		{
			gl.deleteFramebuffers(1, &framebuffer);
		}
	}
	if (sceneTexture_ != 0)
	{
		gl.deleteTextures(1, &sceneTexture_);
	}
	if (bloomTexture_ != 0)
	{
		gl.deleteTextures(1, &bloomTexture_);
	}
	if (depthTexture_ != 0)
	{
		gl.deleteTextures(1, &depthTexture_);
	}
	for (const GLuint texture : blurTextures_)
	{
		if (texture != 0)
		{
			gl.deleteTextures(1, &texture);
		}
	}
	hdrFramebuffer_ = 0;
	sceneTexture_ = 0;
	bloomTexture_ = 0;
	depthTexture_ = 0;
	blurFramebuffers_ = {};
	blurTextures_ = {};
	width_ = 0;
	height_ = 0;
}

GLuint RenderingPostProcess::hdrFramebuffer() const
{
	return hdrFramebuffer_;
}

GLuint RenderingPostProcess::sceneTexture() const
{
	return sceneTexture_;
}

GLuint RenderingPostProcess::bloomTexture() const
{
	return bloomTexture_;
}

GLuint RenderingPostProcess::blurFramebuffer(std::size_t index) const
{
	return blurFramebuffers_[index];
}

GLuint RenderingPostProcess::blurTexture(std::size_t index) const
{
	return blurTextures_[index];
}

int RenderingPostProcess::width() const
{
	return width_;
}

int RenderingPostProcess::height() const
{
	return height_;
}
} // namespace engine::graphics::detail
