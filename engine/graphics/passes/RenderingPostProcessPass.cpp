/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingPostProcessPass.cpp                      :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/09          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/09          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "graphics/passes/RenderingPostProcessPass.hpp"

#include "graphics/resources/RenderingFullscreenQuad.hpp"
#include "graphics/resources/RenderingPostProcess.hpp"
#include "graphics/resources/RenderingPrograms.hpp"
#include "graphics/runtime/OpenGlFunctions.hpp"

#include <cstddef>

namespace
{
constexpr int kBloomBlurPassCount = 4;
}

namespace engine::graphics::detail
{
void RenderingPostProcessPass::execute(
    OpenGlFunctions &gl, const RenderingPrograms &programs,
    const RenderingPostProcess &postProcess,
    const RenderingFullscreenQuad &fullscreenQuad,
    const PostProcessPassInput &input)
{
	gl.disable(GL_DEPTH_TEST);
	gl.useProgram(programs.blur().handle);
	bool horizontal = true;
	bool useBloomSource = true;
	for (int pass = 0; pass < kBloomBlurPassCount; ++pass)
	{
		const std::size_t targetIndex = horizontal ? 0 : 1;
		gl.bindFramebuffer(GL_FRAMEBUFFER,
		                   postProcess.blurFramebuffer(targetIndex));
		gl.uniform1i(programs.blur().uniforms.horizontal, horizontal ? 1 : 0);
		gl.activeTexture(GL_TEXTURE0);
		gl.bindTexture(GL_TEXTURE_2D,
		               useBloomSource
		                   ? postProcess.bloomTexture()
						   : postProcess.blurTexture(horizontal ? 1 : 0));
		gl.bindVertexArray(fullscreenQuad.vertexArray());
		gl.drawArrays(GL_TRIANGLES, 0, RenderingFullscreenQuad::vertexCount());
		horizontal = !horizontal;
		useBloomSource = false;
	}
	const GLuint blurredBloomTexture =
	    postProcess.blurTexture(horizontal ? 1 : 0);
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	gl.viewport(0, 0, input.viewportWidth, input.viewportHeight);
	gl.clearColor(0.0F, 0.0F, 0.0F, 1.0F);
	gl.clear(GL_COLOR_BUFFER_BIT);
	gl.useProgram(programs.composite().handle);
	gl.activeTexture(GL_TEXTURE0);
	gl.bindTexture(GL_TEXTURE_2D, postProcess.sceneTexture());
	gl.activeTexture(GL_TEXTURE1);
	gl.bindTexture(GL_TEXTURE_2D, blurredBloomTexture);
	gl.bindVertexArray(fullscreenQuad.vertexArray());
	gl.drawArrays(GL_TRIANGLES, 0, RenderingFullscreenQuad::vertexCount());
	gl.useProgram(programs.main().handle);
	constexpr GLfloat noEmission[] = {0.0F, 0.0F, 0.0F, 0.0F};
	gl.uniform1i(programs.main().uniforms.usePrimitiveColor, 0);
	gl.uniform1i(programs.main().uniforms.useBaseColorTexture, 0);
	gl.uniform1i(programs.main().uniforms.useSkinning, 0);
	gl.uniform4fv(programs.main().uniforms.emissiveColor, 1, noEmission);
}
} // namespace engine::graphics::detail
