#include "graphics/passes/RenderingVfxPass.hpp"

#include "graphics/resources/RenderingPostProcess.hpp"
#include "graphics/resources/RenderingPrograms.hpp"
#include "graphics/resources/RenderingVfxSprites.hpp"
#include "graphics/runtime/OpenGlFunctions.hpp"

#include <array>

namespace engine::graphics::detail
{
namespace
{
void drawBatch(OpenGlFunctions &gl, const RenderingPrograms::Sprite &program,
               RenderingVfxSprites &sprites, VfxSpriteKind kind,
               std::span<const VfxSpriteVertex> vertices, float bloomIntensity,
               const std::array<float, 4> &colorMultiplier)
{
	if (vertices.empty())
	{
		return;
	}
	if (bloomIntensity > 0.0F)
	{
		constexpr GLenum attachments[] = {GL_COLOR_ATTACHMENT0,
		                                  GL_COLOR_ATTACHMENT1};
		gl.drawBuffers(2, attachments);
	}
	else
	{
		gl.drawBuffer(GL_COLOR_ATTACHMENT0);
	}
	gl.uniform1f(program.uniforms.bloomIntensity, bloomIntensity);
	gl.uniform4fv(program.uniforms.colorMultiplier, 1, colorMultiplier.data());
	gl.bindTexture(GL_TEXTURE_2D, sprites.texture(kind));
	sprites.uploadVertices(gl, vertices);
	gl.drawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
}
} // namespace

void RenderingVfxPass::execute(OpenGlFunctions &gl,
                               const RenderingPrograms &programs,
                               const RenderingPostProcess &postProcess,
                               RenderingVfxSprites &sprites,
                               const Matrix4 &worldViewProjection,
                               const VfxSystem::SpriteBatches &batches)
{
	if (batches.smoke.empty() && batches.fireball.empty())
	{
		return;
	}
	gl.bindFramebuffer(GL_FRAMEBUFFER, postProcess.hdrFramebuffer());
	gl.enable(GL_DEPTH_TEST);
	gl.depthMask(GL_FALSE);
	gl.enable(GL_BLEND);
	gl.blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	const auto &program = programs.sprite();
	gl.useProgram(program.handle);
	gl.uniformMatrix4fv(program.uniforms.worldViewProjection, 1, GL_FALSE,
	                    worldViewProjection.values);
	gl.activeTexture(GL_TEXTURE0);
	gl.bindVertexArray(sprites.vertexArray());
	// The dark underlay remains visible through soft alpha edges and gives the
	// irregular fire texture a readable outline without another asset.
	constexpr std::array<float, 4> fireUnderlay{0.18F, 0.055F, 0.025F, 0.85F};
	constexpr std::array<float, 4> unchangedColor{1.0F, 1.0F, 1.0F, 1.0F};
	drawBatch(gl, program, sprites, VfxSpriteKind::Fireball, batches.fireball,
	          0.0F, fireUnderlay);
	// Fire writes a bright HDR contribution; smoke leaves bloom untouched.
	drawBatch(gl, program, sprites, VfxSpriteKind::Fireball, batches.fireball,
	          3.2F, unchangedColor);
	drawBatch(gl, program, sprites, VfxSpriteKind::Smoke, batches.smoke, 0.0F,
	          unchangedColor);

	// Restore the opaque pass contract before post-process, debug and HUD.
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.bindVertexArray(0);
	gl.bindTexture(GL_TEXTURE_2D, 0);
	gl.disable(GL_BLEND);
	gl.blendFunc(GL_ONE, GL_ZERO);
	gl.depthMask(GL_TRUE);
	constexpr GLenum attachments[] = {GL_COLOR_ATTACHMENT0,
	                                  GL_COLOR_ATTACHMENT1};
	gl.drawBuffers(2, attachments);
	gl.useProgram(programs.main().handle);
}
} // namespace engine::graphics::detail
