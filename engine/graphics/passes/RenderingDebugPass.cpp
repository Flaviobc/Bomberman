/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingDebugPass.cpp                            :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/09          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/09          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "graphics/passes/RenderingDebugPass.hpp"

#include "graphics/features/RenderingDebugGeometry.hpp"
#include "graphics/foundation/RenderingMath.hpp"
#include "graphics/resources/RenderingPrograms.hpp"
#include "graphics/resources/RenderingTransientLines.hpp"
#include "graphics/runtime/OpenGlFunctions.hpp"

namespace engine::graphics::detail
{
void RenderingDebugPass::execute(OpenGlFunctions &gl,
                                 const RenderingPrograms &programs,
                                 RenderingTransientLines &transientLines,
                                 const DebugGeometryFrame &frame,
                                 const Matrix4 &worldViewProjection)
{
	gl.bindVertexArray(transientLines.vertexArray());
	transientLines.upload(gl, frame.world.values.data(), frame.world.count);
	gl.uniformMatrix4fv(programs.main().uniforms.modelViewProjection, 1,
	                    GL_FALSE, worldViewProjection.values);
	gl.drawArrays(GL_LINES, 0, static_cast<GLsizei>(frame.world.count));

	gl.disable(GL_DEPTH_TEST);
	transientLines.upload(gl, frame.overlay.values.data(), frame.overlay.count);
	const Matrix4 identity = identityMatrix();
	gl.uniformMatrix4fv(programs.main().uniforms.modelViewProjection, 1,
	                    GL_FALSE, identity.values);
	gl.drawArrays(GL_LINES, 0, static_cast<GLsizei>(frame.overlay.count));
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.bindVertexArray(0);
}
} // namespace engine::graphics::detail
