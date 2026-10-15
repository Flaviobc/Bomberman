/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingWorldRectangles.cpp                      :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                                                    */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "graphics/resources/RenderingWorldRectangles.hpp"

#include "graphics/runtime/OpenGlFunctions.hpp"

namespace
{
constexpr GLsizei kWorldRectangleStride =
    6 * static_cast<GLsizei>(sizeof(GLfloat));
} // namespace

namespace engine::graphics::detail
{
void RenderingWorldRectangles::upload(OpenGlFunctions &gl,
                                      const std::vector<float> &vertices)
{
	if (vertexArray_ == 0)
	{
		gl.genVertexArrays(1, &vertexArray_);
		gl.bindVertexArray(vertexArray_);
		gl.genBuffers(1, &vertexBuffer_);
		gl.bindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
		gl.vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, kWorldRectangleStride,
		                       nullptr);
		gl.enableVertexAttribArray(0);
		gl.vertexAttribPointer(
		    1, 3, GL_FLOAT, GL_FALSE, kWorldRectangleStride,
		    reinterpret_cast<const void *>(3 * sizeof(GLfloat)));
		gl.enableVertexAttribArray(1);
	}
	else
	{
		gl.bindVertexArray(vertexArray_);
	}
	gl.bindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
	gl.bufferData(GL_ARRAY_BUFFER,
	              static_cast<GLsizeiptr>(vertices.size() * sizeof(GLfloat)),
	              vertices.empty() ? nullptr : vertices.data(),
	              GL_DYNAMIC_DRAW);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.bindVertexArray(0);
	vertexCount_ = static_cast<GLsizei>(vertices.size() / 6);
}

void RenderingWorldRectangles::destroy(OpenGlFunctions &gl)
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
	vertexCount_ = 0;
}

GLuint RenderingWorldRectangles::vertexArray() const
{
	return vertexArray_;
}

GLsizei RenderingWorldRectangles::vertexCount() const
{
	return vertexCount_;
}
} // namespace engine::graphics::detail
