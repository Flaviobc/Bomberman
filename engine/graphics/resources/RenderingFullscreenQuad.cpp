/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingFullscreenQuad.cpp                       :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "graphics/resources/RenderingFullscreenQuad.hpp"

#include "graphics/runtime/OpenGlFunctions.hpp"

#include <array>

namespace
{
constexpr std::array<GLfloat, 24> kFullscreenVertices = {
    -1.0F, -1.0F, 0.0F, 0.0F, 1.0F, -1.0F, 1.0F, 0.0F, 1.0F,  1.0F, 1.0F, 1.0F,
    -1.0F, -1.0F, 0.0F, 0.0F, 1.0F, 1.0F,  1.0F, 1.0F, -1.0F, 1.0F, 0.0F, 1.0F,
};

constexpr GLsizei kFullscreenVertexStride =
    4 * static_cast<GLsizei>(sizeof(GLfloat));
} // namespace

namespace engine::graphics::detail
{
void RenderingFullscreenQuad::create(OpenGlFunctions &gl)
{
	gl.genVertexArrays(1, &vertexArray_);
	gl.bindVertexArray(vertexArray_);
	gl.genBuffers(1, &vertexBuffer_);
	gl.bindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
	gl.bufferData(
	    GL_ARRAY_BUFFER,
	    static_cast<GLsizeiptr>(kFullscreenVertices.size() * sizeof(GLfloat)),
	    kFullscreenVertices.data(), GL_STATIC_DRAW);
	gl.vertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, kFullscreenVertexStride,
	                       nullptr);
	gl.enableVertexAttribArray(0);
	gl.vertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, kFullscreenVertexStride,
	                       reinterpret_cast<const void *>(2 * sizeof(GLfloat)));
	gl.enableVertexAttribArray(1);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.bindVertexArray(0);
}

void RenderingFullscreenQuad::destroy(OpenGlFunctions &gl)
{
	if (vertexBuffer_ != 0)
	{
		gl.deleteBuffers(1, &vertexBuffer_);
	}
	if (vertexArray_ != 0)
	{
		gl.deleteVertexArrays(1, &vertexArray_);
	}
	vertexArray_ = 0;
	vertexBuffer_ = 0;
}

GLuint RenderingFullscreenQuad::vertexArray() const
{
	return vertexArray_;
}
} // namespace engine::graphics::detail
