/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingFullscreenQuad.hpp                       :+:        :+:    :+: :+:    : */
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

// Owns the fixed six-vertex position/UV quad used only by blur and composite.
// Renderer explicitly destroys it while its OpenGL context is current.
class RenderingFullscreenQuad
{
  public:
	void create(OpenGlFunctions &gl);
	void destroy(OpenGlFunctions &gl);

	[[nodiscard]] GLuint vertexArray() const;
	[[nodiscard]] static constexpr GLsizei vertexCount()
	{
		return 6;
	}

  private:
	GLuint vertexArray_ = 0;
	GLuint vertexBuffer_ = 0;
};
} // namespace engine::graphics::detail
