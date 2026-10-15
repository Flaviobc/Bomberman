/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingShadowMap.hpp                            :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

#include <SDL3/SDL_opengl.h>

namespace engine::graphics::detail
{
class OpenGlFunctions;

// Owns only the fixed directional-shadow render target. Renderer explicitly
// destroys it while its OpenGL context is current.
class RenderingShadowMap
{
  public:
	[[nodiscard]] bool create(OpenGlFunctions &gl);
	void destroy(OpenGlFunctions &gl);

	[[nodiscard]] GLuint framebuffer() const;
	[[nodiscard]] GLuint depthTexture() const;
	[[nodiscard]] static constexpr GLsizei dimension()
	{
		return 2048;
	}

  private:
	GLuint framebuffer_ = 0;
	GLuint depthTexture_ = 0;
};
} // namespace engine::graphics::detail
