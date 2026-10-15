/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingWorldRectangles.hpp                      :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                                                    */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

#include <SDL3/SDL_opengl.h>

#include <vector>

namespace engine::graphics::detail
{
class OpenGlFunctions;

// Owns the lazily-created position/color buffer for generic world rectangles.
// RenderingWorldGeometry owns CPU generation and Renderer owns draw submission.
class RenderingWorldRectangles
{
  public:
	void upload(OpenGlFunctions &gl, const std::vector<float> &vertices);
	void destroy(OpenGlFunctions &gl);

	[[nodiscard]] GLuint vertexArray() const;
	[[nodiscard]] GLsizei vertexCount() const;

  private:
	GLuint vertexArray_ = 0;
	GLuint vertexBuffer_ = 0;
	GLsizei vertexCount_ = 0;
};
} // namespace engine::graphics::detail
