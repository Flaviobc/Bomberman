/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   OpenGlFunctions.hpp                               :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

#include <SDL3/SDL_opengl.h>

namespace engine::graphics::detail
{
// Function table for the OpenGL calls used by this Renderer. SDL owns platform
// resolution; Renderer owns the context in which these pointers are valid.
class OpenGlFunctions
{
  public:
	using ClearColor = void(APIENTRYP)(GLfloat, GLfloat, GLfloat, GLfloat);
	using Clear = void(APIENTRYP)(GLbitfield);
	using Viewport = void(APIENTRYP)(GLint, GLint, GLsizei, GLsizei);
	using GenVertexArrays = void(APIENTRYP)(GLsizei, GLuint *);
	using BindVertexArray = void(APIENTRYP)(GLuint);
	using DeleteVertexArrays = void(APIENTRYP)(GLsizei, const GLuint *);
	using GenBuffers = void(APIENTRYP)(GLsizei, GLuint *);
	using BindBuffer = void(APIENTRYP)(GLenum, GLuint);
	using BufferData = void(APIENTRYP)(GLenum, GLsizeiptr, const void *,
	                                   GLenum);
	using DeleteBuffers = void(APIENTRYP)(GLsizei, const GLuint *);
	using EnableVertexAttribArray = void(APIENTRYP)(GLuint);
	using VertexAttribPointer = void(APIENTRYP)(GLuint, GLint, GLenum,
	                                            GLboolean, GLsizei,
	                                            const void *);
	using VertexAttribIPointer = void(APIENTRYP)(GLuint, GLint, GLenum, GLsizei,
	                                             const void *);
	using VertexAttribDivisor = void(APIENTRYP)(GLuint, GLuint);
	using CreateShader = GLuint(APIENTRYP)(GLenum);
	using ShaderSource = void(APIENTRYP)(GLuint, GLsizei, const GLchar *const *,
	                                     const GLint *);
	using CompileShader = void(APIENTRYP)(GLuint);
	using GetShaderiv = void(APIENTRYP)(GLuint, GLenum, GLint *);
	using GetShaderInfoLog = void(APIENTRYP)(GLuint, GLsizei, GLsizei *,
	                                         GLchar *);
	using DeleteShader = void(APIENTRYP)(GLuint);
	using CreateProgram = GLuint(APIENTRYP)();
	using AttachShader = void(APIENTRYP)(GLuint, GLuint);
	using LinkProgram = void(APIENTRYP)(GLuint);
	using GetProgramiv = void(APIENTRYP)(GLuint, GLenum, GLint *);
	using GetProgramInfoLog = void(APIENTRYP)(GLuint, GLsizei, GLsizei *,
	                                          GLchar *);
	using DeleteProgram = void(APIENTRYP)(GLuint);
	using UseProgram = void(APIENTRYP)(GLuint);
	using DrawArrays = void(APIENTRYP)(GLenum, GLint, GLsizei);
	using DrawElements = void(APIENTRYP)(GLenum, GLsizei, GLenum, const void *);
	using DrawElementsInstanced = void(APIENTRYP)(GLenum, GLsizei, GLenum,
	                                              const void *, GLsizei);
	using GetString = const GLubyte *(APIENTRYP)(GLenum);
	using Enable = void(APIENTRYP)(GLenum);
	using Disable = void(APIENTRYP)(GLenum);
	using DepthMask = void(APIENTRYP)(GLboolean);
	using GetError = GLenum(APIENTRYP)();
	using BlendFunc = void(APIENTRYP)(GLenum, GLenum);
	using DebugMessageCallback = void(APIENTRYP)(GLDEBUGPROC, const void *);
	using DebugMessageInsert = void(APIENTRYP)(GLenum, GLenum, GLuint, GLenum,
	                                           GLsizei, const GLchar *);
	using GetUniformLocation = GLint(APIENTRYP)(GLuint, const GLchar *);
	using Uniform1i = void(APIENTRYP)(GLint, GLint);
	using Uniform1f = void(APIENTRYP)(GLint, GLfloat);
	using Uniform4fv = void(APIENTRYP)(GLint, GLsizei, const GLfloat *);
	using UniformMatrix4fv = void(APIENTRYP)(GLint, GLsizei, GLboolean,
	                                         const GLfloat *);
	using UniformMatrix3fv = void(APIENTRYP)(GLint, GLsizei, GLboolean,
	                                         const GLfloat *);
	using GenFramebuffers = void(APIENTRYP)(GLsizei, GLuint *);
	using BindFramebuffer = void(APIENTRYP)(GLenum, GLuint);
	using FramebufferTexture2D = void(APIENTRYP)(GLenum, GLenum, GLenum, GLuint,
	                                             GLint);
	using CheckFramebufferStatus = GLenum(APIENTRYP)(GLenum);
	using DeleteFramebuffers = void(APIENTRYP)(GLsizei, const GLuint *);
	using GenTextures = void(APIENTRYP)(GLsizei, GLuint *);
	using BindTexture = void(APIENTRYP)(GLenum, GLuint);
	using TexImage2D = void(APIENTRYP)(GLenum, GLint, GLint, GLsizei, GLsizei,
	                                   GLint, GLenum, GLenum, const void *);
	using TexParameteri = void(APIENTRYP)(GLenum, GLenum, GLint);
	using DeleteTextures = void(APIENTRYP)(GLsizei, const GLuint *);
	using ActiveTexture = void(APIENTRYP)(GLenum);
	using DrawBuffer = void(APIENTRYP)(GLenum);
	using DrawBuffers = void(APIENTRYP)(GLsizei, const GLenum *);
	using ReadBuffer = void(APIENTRYP)(GLenum);
	using PolygonOffset = void(APIENTRYP)(GLfloat, GLfloat);

	[[nodiscard]] bool load();

	ClearColor clearColor{};
	Clear clear{};
	Viewport viewport{};
	GenVertexArrays genVertexArrays{};
	BindVertexArray bindVertexArray{};
	DeleteVertexArrays deleteVertexArrays{};
	GenBuffers genBuffers{};
	BindBuffer bindBuffer{};
	BufferData bufferData{};
	DeleteBuffers deleteBuffers{};
	EnableVertexAttribArray enableVertexAttribArray{};
	VertexAttribPointer vertexAttribPointer{};
	VertexAttribIPointer vertexAttribIPointer{};
	VertexAttribDivisor vertexAttribDivisor{};
	CreateShader createShader{};
	ShaderSource shaderSource{};
	CompileShader compileShader{};
	GetShaderiv getShaderiv{};
	GetShaderInfoLog getShaderInfoLog{};
	DeleteShader deleteShader{};
	CreateProgram createProgram{};
	AttachShader attachShader{};
	LinkProgram linkProgram{};
	GetProgramiv getProgramiv{};
	GetProgramInfoLog getProgramInfoLog{};
	DeleteProgram deleteProgram{};
	UseProgram useProgram{};
	DrawArrays drawArrays{};
	DrawElements drawElements{};
	DrawElementsInstanced drawElementsInstanced{};
	GetString getString{};
	Enable enable{};
	Disable disable{};
	BlendFunc blendFunc{};
	DepthMask depthMask{};
	GetError getError{};
	DebugMessageCallback debugMessageCallback{};
	DebugMessageInsert debugMessageInsert{};
	GetUniformLocation getUniformLocation{};
	Uniform1i uniform1i{};
	Uniform1f uniform1f{};
	Uniform4fv uniform4fv{};
	UniformMatrix4fv uniformMatrix4fv{};
	UniformMatrix3fv uniformMatrix3fv{};
	GenFramebuffers genFramebuffers{};
	BindFramebuffer bindFramebuffer{};
	FramebufferTexture2D framebufferTexture2D{};
	CheckFramebufferStatus checkFramebufferStatus{};
	DeleteFramebuffers deleteFramebuffers{};
	GenTextures genTextures{};
	BindTexture bindTexture{};
	TexImage2D texImage2D{};
	TexParameteri texParameteri{};
	DeleteTextures deleteTextures{};
	ActiveTexture activeTexture{};
	DrawBuffer drawBuffer{};
	DrawBuffers drawBuffers{};
	ReadBuffer readBuffer{};
	PolygonOffset polygonOffset{};
};
} // namespace engine::graphics::detail
