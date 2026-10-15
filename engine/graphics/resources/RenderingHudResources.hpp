/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingHudResources.hpp                         :+:        :+:    :+: :+:    : */
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
#include <vector>

namespace engine::assets
{
struct DecodedImageRgba8;
}

namespace engine::graphics::detail
{
class OpenGlFunctions;

// Owns only HUD resources stored by OpenGL. RenderingHud retains all CPU state
// and geometry; Renderer retains program state and draw submission.
class RenderingHudResources
{
  public:
	void create(OpenGlFunctions &gl);
	[[nodiscard]] bool
	uploadIconAtlas(OpenGlFunctions &gl, const assets::DecodedImageRgba8 &image,
	                const std::array<float, 144> &iconVertices);
	[[nodiscard]] bool uploadFontAtlas(OpenGlFunctions &gl,
	                                   const assets::DecodedImageRgba8 &image);
	void updateGlyphs(OpenGlFunctions &gl, const std::vector<float> &vertices,
	                  std::size_t textCount);
	void destroy(OpenGlFunctions &gl);

	[[nodiscard]] GLuint iconVertexArray() const;
	[[nodiscard]] GLuint iconAtlasTexture() const;
	[[nodiscard]] GLuint fontVertexArray() const;
	[[nodiscard]] GLuint fontAtlasTexture() const;
	[[nodiscard]] GLsizei fontVertexCount() const;

  private:
	GLuint iconVertexArray_ = 0;
	GLuint iconVertexBuffer_ = 0;
	GLuint iconAtlasTexture_ = 0;
	GLuint fontVertexArray_ = 0;
	GLuint fontVertexBuffer_ = 0;
	GLuint fontAtlasTexture_ = 0;
	GLsizei fontVertexCount_ = 0;
};
} // namespace engine::graphics::detail
