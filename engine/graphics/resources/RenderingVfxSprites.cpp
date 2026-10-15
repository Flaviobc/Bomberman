#include "graphics/resources/RenderingVfxSprites.hpp"

#include "assets/PngDecoder.hpp"
#include "graphics/runtime/OpenGlFunctions.hpp"

#include <cstddef>
#include <cstdio>
#include <limits>

namespace engine::graphics::detail
{
bool RenderingVfxSprites::create(OpenGlFunctions &gl)
{
	gl.genVertexArrays(1, &vertexArray_);
	gl.genBuffers(1, &vertexBuffer_);
	if (vertexArray_ == 0 || vertexBuffer_ == 0)
	{
		std::fputs("VFX sprite VAO/VBO creation failed.\n", stderr);
		return false;
	}
	gl.bindVertexArray(vertexArray_);
	gl.bindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
	gl.bufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
	static_assert(sizeof(VfxSpriteVertex) == 9 * sizeof(float));
	gl.vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VfxSpriteVertex),
	                       nullptr);
	gl.enableVertexAttribArray(0);
	gl.vertexAttribPointer(
	    1, 2, GL_FLOAT, GL_FALSE, sizeof(VfxSpriteVertex),
	    reinterpret_cast<const void *>(offsetof(VfxSpriteVertex, u)));
	gl.enableVertexAttribArray(1);
	gl.vertexAttribPointer(
	    2, 4, GL_FLOAT, GL_FALSE, sizeof(VfxSpriteVertex),
	    reinterpret_cast<const void *>(offsetof(VfxSpriteVertex, red)));
	gl.enableVertexAttribArray(2);
	gl.bindVertexArray(0);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	const GLenum error = gl.getError();
	if (error != GL_NO_ERROR)
	{
		std::fprintf(stderr,
		             "VFX sprite buffer setup failed: OpenGL error=0x%x.\n",
		             error);
		return false;
	}
	return true;
}

namespace
{
bool uploadTexture(OpenGlFunctions &gl,
                   const engine::assets::DecodedImageRgba8 &image,
                   GLuint &texture, const char *label)
{
	const auto maxDimension =
	    static_cast<std::size_t>(std::numeric_limits<GLsizei>::max());
	if (texture != 0 || image.width == 0 || image.height == 0 ||
	    image.channels != 4 || image.width > maxDimension ||
	    image.height > maxDimension ||
	    image.width >
	        std::numeric_limits<std::size_t>::max() / image.height / 4 ||
	    image.pixels.size() != image.width * image.height * 4)
	{
		std::fprintf(stderr, "VFX %s sprite requires one valid RGBA8 upload.\n",
		             label);
		return false;
	}
	const GLenum previousError = gl.getError();
	if (previousError != GL_NO_ERROR)
	{
		std::fprintf(stderr,
		             "VFX sprite upload aborted: prior OpenGL error=0x%x.\n",
		             previousError);
		return false;
	}
	gl.genTextures(1, &texture);
	if (texture == 0)
	{
		std::fputs("VFX sprite texture creation failed.\n", stderr);
		return false;
	}
	gl.activeTexture(GL_TEXTURE0);
	gl.bindTexture(GL_TEXTURE_2D, texture);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	gl.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GLsizei>(image.width),
	              static_cast<GLsizei>(image.height), 0, GL_RGBA,
	              GL_UNSIGNED_BYTE, image.pixels.data());
	gl.bindTexture(GL_TEXTURE_2D, 0);
	const GLenum error = gl.getError();
	if (error != GL_NO_ERROR)
	{
		std::fprintf(stderr,
		             "VFX sprite image upload failed: OpenGL error=0x%x.\n",
		             error);
		gl.deleteTextures(1, &texture);
		texture = 0;
		return false;
	}
	std::fprintf(
	    stderr,
	    "VFX %s sprite uploaded once: width=%zu height=%zu rgba8_bytes=%zu.\n",
	    label, image.width, image.height, image.pixels.size());
	return true;
}
} // namespace

bool RenderingVfxSprites::uploadImages(OpenGlFunctions &gl,
                                       const VfxSpriteImages &images)
{
	if (vertexArray_ == 0 || vertexBuffer_ == 0)
	{
		std::fputs("VFX sprite images require initialized buffers.\n", stderr);
		return false;
	}
	if (!uploadTexture(gl, images.smoke, smokeTexture_, "smoke"))
	{
		return false;
	}
	if (!uploadTexture(gl, images.fireball, fireballTexture_, "fireball"))
	{
		gl.deleteTextures(1, &smokeTexture_);
		smokeTexture_ = 0;
		return false;
	}
	return true;
}

void RenderingVfxSprites::uploadVertices(
    OpenGlFunctions &gl, std::span<const VfxSpriteVertex> vertices)
{
	gl.bindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
	gl.bufferData(GL_ARRAY_BUFFER,
	              static_cast<GLsizeiptr>(vertices.size_bytes()),
	              vertices.data(), GL_DYNAMIC_DRAW);
}

void RenderingVfxSprites::destroy(OpenGlFunctions &gl)
{
	for (GLuint *texture : {&smokeTexture_, &fireballTexture_})
	{
		if (*texture != 0)
		{
			gl.deleteTextures(1, texture);
			*texture = 0;
		}
	}
	if (vertexBuffer_ != 0)
	{
		gl.deleteBuffers(1, &vertexBuffer_);
		vertexBuffer_ = 0;
	}
	if (vertexArray_ != 0)
	{
		gl.deleteVertexArrays(1, &vertexArray_);
		vertexArray_ = 0;
	}
}

GLuint RenderingVfxSprites::texture(VfxSpriteKind kind) const
{
	return kind == VfxSpriteKind::Smoke ? smokeTexture_ : fireballTexture_;
}
} // namespace engine::graphics::detail
