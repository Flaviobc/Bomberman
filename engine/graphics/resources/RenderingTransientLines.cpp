/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingTransientLines.cpp                       :+:        :+: :+:    :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                                                    */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "graphics/resources/RenderingTransientLines.hpp"

#include "graphics/runtime/OpenGlFunctions.hpp"

namespace
{
constexpr GLsizei kTransientLineStride =
    6 * static_cast<GLsizei>(sizeof(GLfloat));
} // namespace

namespace engine::graphics::detail
{
void RenderingTransientLines::create(OpenGlFunctions &gl)
{
	gl.genVertexArrays(1, &vertexArray_);
	gl.bindVertexArray(vertexArray_);
	gl.genBuffers(1, &vertexBuffer_);
	gl.bindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
	gl.bufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
	gl.vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, kTransientLineStride,
	                       nullptr);
	gl.enableVertexAttribArray(0);
	gl.vertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, kTransientLineStride,
	                       reinterpret_cast<const void *>(3 * sizeof(GLfloat)));
	gl.enableVertexAttribArray(1);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.bindVertexArray(0);
}

void RenderingTransientLines::upload(OpenGlFunctions &gl, const float *vertices,
                                     std::size_t vertexCount)
{
	gl.bindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
	gl.bufferData(GL_ARRAY_BUFFER,
	              static_cast<GLsizeiptr>(vertexCount) * 6 * sizeof(GLfloat),
	              vertexCount == 0 ? nullptr : vertices, GL_DYNAMIC_DRAW);
}

void RenderingTransientLines::destroy(OpenGlFunctions &gl)
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

GLuint RenderingTransientLines::vertexArray() const
{
	return vertexArray_;
}
} // namespace engine::graphics::detail
