/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingHudPass.cpp                              :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/09          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/09          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "graphics/passes/RenderingHudPass.hpp"

#include "graphics/features/RenderingHud.hpp"
#include "graphics/foundation/RenderingMath.hpp"
#include "graphics/resources/RenderingHudResources.hpp"
#include "graphics/resources/RenderingPrograms.hpp"
#include "graphics/resources/RenderingTransientLines.hpp"
#include "graphics/runtime/OpenGlFunctions.hpp"

namespace
{
void executePlan(
    engine::graphics::detail::OpenGlFunctions &gl,
    const engine::graphics::detail::RenderingPrograms &programs,
    const engine::graphics::detail::RenderingHudResources &resources,
    engine::graphics::detail::RenderingTransientLines &transientLines,
    const engine::graphics::detail::HudDrawPlan &plan)
{
	if (resources.iconAtlasTexture() != 0)
	{
		gl.enable(GL_BLEND);
		gl.blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		gl.useProgram(programs.hud().handle);
		gl.activeTexture(GL_TEXTURE0);
		gl.bindTexture(GL_TEXTURE_2D, resources.iconAtlasTexture());
		gl.bindVertexArray(resources.iconVertexArray());
		for (std::size_t index = 0; index < plan.atlasDrawCount; ++index)
		{
			const engine::graphics::detail::HudAtlasDrawCommand &draw =
			    plan.atlasDraws[index];
			gl.uniformMatrix4fv(programs.hud().uniforms.transform, 1, GL_FALSE,
			                    draw.transform.values);
			gl.uniform1i(programs.hud().uniforms.useAtlas,
			             draw.useAtlas ? 1 : 0);
			gl.uniform4fv(programs.hud().uniforms.tint, 1, draw.tint.data());
			gl.drawArrays(GL_TRIANGLES, draw.firstVertex, draw.vertexCount);
		}
		gl.bindVertexArray(0);
		gl.bindTexture(GL_TEXTURE_2D, 0);
		gl.disable(GL_BLEND);
		gl.useProgram(programs.main().handle);
	}
	if (resources.fontAtlasTexture() != 0 && resources.fontVertexCount() > 0)
	{
		const engine::graphics::detail::Matrix4 identity =
		    engine::graphics::detail::identityMatrix();
		gl.enable(GL_BLEND);
		gl.blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		gl.useProgram(programs.hud().handle);
		gl.uniformMatrix4fv(programs.hud().uniforms.transform, 1, GL_FALSE,
		                    identity.values);
		gl.uniform1i(programs.hud().uniforms.useAtlas, 1);
		gl.activeTexture(GL_TEXTURE0);
		gl.bindTexture(GL_TEXTURE_2D, resources.fontAtlasTexture());
		gl.bindVertexArray(resources.fontVertexArray());
		gl.drawArrays(GL_TRIANGLES, 0, resources.fontVertexCount());
		gl.bindVertexArray(0);
		gl.bindTexture(GL_TEXTURE_2D, 0);
		gl.disable(GL_BLEND);
		gl.useProgram(programs.main().handle);
	}
	if (plan.gameplayHudVisible)
	{
		gl.bindVertexArray(transientLines.vertexArray());
		transientLines.upload(gl, plan.lines.values.data(),
		                      plan.lines.vertexCount);
		gl.uniformMatrix4fv(programs.main().uniforms.modelViewProjection, 1,
		                    GL_FALSE, plan.lineTransform.values);
		gl.drawArrays(GL_LINES, 0,
		              static_cast<GLsizei>(plan.lines.vertexCount));
		gl.bindBuffer(GL_ARRAY_BUFFER, 0);
		gl.bindVertexArray(0);
	}
}
} // namespace

namespace engine::graphics::detail
{
void RenderingHudPass::executeTitleFrame(
    OpenGlFunctions &gl, const RenderingPrograms &programs,
    const RenderingHudResources &resources,
    RenderingTransientLines &transientLines, const HudDrawPlan &plan,
    const HudTitleFrameInput &input)
{
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	gl.viewport(0, 0, input.viewportWidth, input.viewportHeight);
	gl.clearColor(0.01F, 0.02F, 0.04F, 1.0F);
	gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	gl.disable(GL_DEPTH_TEST);
	gl.useProgram(programs.main().handle);
	gl.uniform1i(programs.main().uniforms.usePrimitiveColor, 0);
	gl.uniform1i(programs.main().uniforms.useBaseColorTexture, 0);
	gl.uniform1i(programs.main().uniforms.useSkinning, 0);
	constexpr GLfloat noEmission[] = {0.0F, 0.0F, 0.0F, 0.0F};
	gl.uniform4fv(programs.main().uniforms.emissiveColor, 1, noEmission);
	executePlan(gl, programs, resources, transientLines, plan);
	gl.bindVertexArray(0);
}

void RenderingHudPass::executeOverlay(OpenGlFunctions &gl,
                                      const RenderingPrograms &programs,
                                      const RenderingHudResources &resources,
                                      RenderingTransientLines &transientLines,
                                      const HudDrawPlan &plan)
{
	executePlan(gl, programs, resources, transientLines, plan);
	gl.bindVertexArray(0);
}
} // namespace engine::graphics::detail
