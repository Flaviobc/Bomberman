/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingTransientLines.hpp                       :+:        :+: :+:    :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                                                    */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

#include <SDL3/SDL_opengl.h>

#include <cstddef>

namespace engine::graphics::detail
{
class OpenGlFunctions;

// Owns the one sequentially reused position/color buffer for transient lines.
// CPU geometry and draw submission remain in their existing Renderer helpers.
class RenderingTransientLines
{
  public:
	void create(OpenGlFunctions &gl);
	void upload(OpenGlFunctions &gl, const float *vertices,
	            std::size_t vertexCount);
	void destroy(OpenGlFunctions &gl);

	[[nodiscard]] GLuint vertexArray() const;

  private:
	GLuint vertexArray_ = 0;
	GLuint vertexBuffer_ = 0;
};
} // namespace engine::graphics::detail
