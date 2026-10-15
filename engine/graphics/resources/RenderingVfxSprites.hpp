#pragma once

#include "graphics/Renderer.hpp"
#include "graphics/features/VfxSystem.hpp"

#include <SDL3/SDL_opengl.h>

namespace engine::assets
{
struct DecodedImageRgba8;
}

namespace engine::graphics::detail
{
class OpenGlFunctions;

// GPU-only ownership; borrowed CPU pixels/vertices are consumed synchronously.
// Explicit destroy must run before the OpenGL context is destroyed.
class RenderingVfxSprites
{
  public:
	[[nodiscard]] bool create(OpenGlFunctions &gl);
	[[nodiscard]] bool uploadImages(OpenGlFunctions &gl,
	                                const VfxSpriteImages &images);
	void uploadVertices(OpenGlFunctions &gl,
	                    std::span<const VfxSpriteVertex> vertices);
	void destroy(OpenGlFunctions &gl);
	[[nodiscard]] GLuint vertexArray() const
	{
		return vertexArray_;
	}
	[[nodiscard]] GLuint texture(VfxSpriteKind kind) const;

  private:
	GLuint vertexArray_ = 0;
	GLuint vertexBuffer_ = 0;
	GLuint smokeTexture_ = 0;
	GLuint fireballTexture_ = 0;
};
} // namespace engine::graphics::detail
