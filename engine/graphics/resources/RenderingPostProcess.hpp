/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingPostProcess.hpp                          :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

#include <SDL3/SDL_opengl.h>

#include <array>
#include <cstddef>

namespace engine::graphics::detail
{
class OpenGlFunctions;

// Owns only the render-target handles used by HDR and bloom. Renderer must
// explicitly destroy them while its OpenGL context is still current.
class RenderingPostProcess
{
  public:
	[[nodiscard]] bool create(OpenGlFunctions &gl);
	[[nodiscard]] bool resize(OpenGlFunctions &gl, int width, int height);
	void destroy(OpenGlFunctions &gl);

	[[nodiscard]] GLuint hdrFramebuffer() const;
	[[nodiscard]] GLuint sceneTexture() const;
	[[nodiscard]] GLuint bloomTexture() const;
	[[nodiscard]] GLuint blurFramebuffer(std::size_t index) const;
	[[nodiscard]] GLuint blurTexture(std::size_t index) const;
	[[nodiscard]] int width() const;
	[[nodiscard]] int height() const;

  private:
	GLuint hdrFramebuffer_ = 0;
	GLuint sceneTexture_ = 0;
	GLuint bloomTexture_ = 0;
	GLuint depthTexture_ = 0;
	std::array<GLuint, 2> blurFramebuffers_{};
	std::array<GLuint, 2> blurTextures_{};
	int width_ = 0;
	int height_ = 0;
};
} // namespace engine::graphics::detail
