#pragma once

#include "graphics/features/VfxSystem.hpp"

namespace engine::graphics::detail
{
class OpenGlFunctions;
class RenderingPrograms;
class RenderingPostProcess;
class RenderingVfxSprites;

class RenderingVfxPass
{
  public:
	static void execute(OpenGlFunctions &gl, const RenderingPrograms &programs,
	                    const RenderingPostProcess &postProcess,
	                    RenderingVfxSprites &sprites,
	                    const Matrix4 &worldViewProjection,
	                    const VfxSystem::SpriteBatches &batches);
};
} // namespace engine::graphics::detail
