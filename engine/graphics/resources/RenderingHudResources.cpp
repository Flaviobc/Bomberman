/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingHudResources.cpp                         :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "graphics/resources/RenderingHudResources.hpp"

#include "graphics/runtime/OpenGlFunctions.hpp"

#include "assets/PngDecoder.hpp"

#include <cstdio>
#include <limits>

namespace
{
constexpr GLsizei kHudVertexStride = 4 * static_cast<GLsizei>(sizeof(GLfloat));

void createPositionUvBuffer(engine::graphics::detail::OpenGlFunctions &gl,
                            GLuint &vertexArray, GLuint &vertexBuffer,
                            GLenum usage)
{
	gl.genVertexArrays(1, &vertexArray);
	gl.bindVertexArray(vertexArray);
	gl.genBuffers(1, &vertexBuffer);
	gl.bindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	gl.bufferData(GL_ARRAY_BUFFER, 0, nullptr, usage);
	gl.vertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, kHudVertexStride, nullptr);
	gl.enableVertexAttribArray(0);
	gl.vertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, kHudVertexStride,
	                       reinterpret_cast<const void *>(2 * sizeof(GLfloat)));
	gl.enableVertexAttribArray(1);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.bindVertexArray(0);
}
} // namespace

namespace engine::graphics::detail
{
void RenderingHudResources::create(OpenGlFunctions &gl)
{
	createPositionUvBuffer(gl, iconVertexArray_, iconVertexBuffer_,
	                       GL_STATIC_DRAW);
	createPositionUvBuffer(gl, fontVertexArray_, fontVertexBuffer_,
	                       GL_DYNAMIC_DRAW);
}

bool RenderingHudResources::uploadIconAtlas(
    OpenGlFunctions &gl, const assets::DecodedImageRgba8 &image,
    const std::array<float, 144> &iconVertices)
{
	if (iconAtlasTexture_ != 0 || image.width == 0 || image.height == 0 ||
	    image.channels != 4 ||
	    image.width >
	        std::numeric_limits<std::size_t>::max() / image.height / 4 ||
	    image.pixels.size() != image.width * image.height * 4)
	{
		std::fputs("HUD icon atlas requires one valid RGBA8 upload.\n", stderr);
		return false;
	}
	gl.genTextures(1, &iconAtlasTexture_);
	gl.bindTexture(GL_TEXTURE_2D, iconAtlasTexture_);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	gl.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(image.width),
	              static_cast<GLsizei>(image.height), 0, GL_RGBA,
	              GL_UNSIGNED_BYTE, image.pixels.data());
	gl.bindVertexArray(iconVertexArray_);
	gl.bindBuffer(GL_ARRAY_BUFFER, iconVertexBuffer_);
	gl.bufferData(GL_ARRAY_BUFFER,
	              static_cast<GLsizeiptr>(iconVertices.size() * sizeof(float)),
	              iconVertices.data(), GL_STATIC_DRAW);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.bindVertexArray(0);
	gl.bindTexture(GL_TEXTURE_2D, 0);
	std::fprintf(stderr,
	             "HUD icon atlas uploaded once: width=%zu height=%zu "
	             "channels=%zu icons=4.\n",
	             image.width, image.height, image.channels);
	return true;
}

bool RenderingHudResources::uploadFontAtlas(
    OpenGlFunctions &gl, const assets::DecodedImageRgba8 &image)
{
	if (fontAtlasTexture_ != 0 || image.width != 256 || image.height != 48 ||
	    image.channels != 4 ||
	    image.pixels.size() != image.width * image.height * image.channels)
	{
		std::fputs("HUD font atlas requires one 256x48 RGBA8 upload.\n",
		           stderr);
		return false;
	}
	gl.genTextures(1, &fontAtlasTexture_);
	gl.bindTexture(GL_TEXTURE_2D, fontAtlasTexture_);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	gl.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(image.width),
	              static_cast<GLsizei>(image.height), 0, GL_RGBA,
	              GL_UNSIGNED_BYTE, image.pixels.data());
	gl.bindTexture(GL_TEXTURE_2D, 0);
	std::fprintf(stderr,
	             "HUD font atlas uploaded once: width=%zu height=%zu "
	             "channels=%zu glyphs=41.\n",
	             image.width, image.height, image.channels);
	return true;
}

void RenderingHudResources::updateGlyphs(OpenGlFunctions &gl,
                                         const std::vector<float> &vertices,
                                         std::size_t textCount)
{
	fontVertexCount_ = static_cast<GLsizei>(vertices.size() / 4);
	gl.bindVertexArray(fontVertexArray_);
	gl.bindBuffer(GL_ARRAY_BUFFER, fontVertexBuffer_);
	gl.bufferData(GL_ARRAY_BUFFER,
	              static_cast<GLsizeiptr>(vertices.size() * sizeof(GLfloat)),
	              vertices.empty() ? nullptr : vertices.data(),
	              GL_DYNAMIC_DRAW);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.bindVertexArray(0);
	std::fprintf(stderr, "HUD text updated: strings=%zu glyph_vertices=%d.\n",
	             textCount, fontVertexCount_);
}

void RenderingHudResources::destroy(OpenGlFunctions &gl)
{
	if (iconAtlasTexture_ != 0)
	{
		gl.deleteTextures(1, &iconAtlasTexture_);
	}
	if (fontAtlasTexture_ != 0)
	{
		gl.deleteTextures(1, &fontAtlasTexture_);
	}
	if (iconVertexBuffer_ != 0)
	{
		gl.deleteBuffers(1, &iconVertexBuffer_);
	}
	if (fontVertexBuffer_ != 0)
	{
		gl.deleteBuffers(1, &fontVertexBuffer_);
	}
	if (iconVertexArray_ != 0)
	{
		gl.deleteVertexArrays(1, &iconVertexArray_);
	}
	if (fontVertexArray_ != 0)
	{
		gl.deleteVertexArrays(1, &fontVertexArray_);
	}
	iconVertexArray_ = 0;
	iconVertexBuffer_ = 0;
	iconAtlasTexture_ = 0;
	fontVertexArray_ = 0;
	fontVertexBuffer_ = 0;
	fontAtlasTexture_ = 0;
	fontVertexCount_ = 0;
}

GLuint RenderingHudResources::iconVertexArray() const
{
	return iconVertexArray_;
}

GLuint RenderingHudResources::iconAtlasTexture() const
{
	return iconAtlasTexture_;
}

GLuint RenderingHudResources::fontVertexArray() const
{
	return fontVertexArray_;
}

GLuint RenderingHudResources::fontAtlasTexture() const
{
	return fontAtlasTexture_;
}

GLsizei RenderingHudResources::fontVertexCount() const
{
	return fontVertexCount_;
}
} // namespace engine::graphics::detail
