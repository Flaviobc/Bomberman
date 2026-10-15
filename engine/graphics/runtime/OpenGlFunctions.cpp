/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   OpenGlFunctions.cpp                               :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "graphics/runtime/OpenGlFunctions.hpp"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_video.h>

#include <cstdio>

namespace
{
SDL_FunctionPointer loadFunction(const char *name)
{
	SDL_FunctionPointer function = SDL_GL_GetProcAddress(name);
	if (!function)
	{
		std::fprintf(stderr, "SDL_GL_GetProcAddress(%s) failed: %s\n", name,
		             SDL_GetError());
	}
	return function;
}

#ifndef NDEBUG
void APIENTRY debugCallback(GLenum source, GLenum type, GLuint identifier,
                            GLenum severity, GLsizei, const GLchar *message,
                            const void *)
{
	std::fprintf(
	    stderr, "OpenGL debug: source=0x%x type=0x%x id=%u severity=0x%x: %s\n",
	    source, type, identifier, severity, message);
}
#endif
} // namespace

namespace engine::graphics::detail
{
bool OpenGlFunctions::load()
{
#define LOAD_GL(name, member, symbol)                                          \
	member = reinterpret_cast<name>(loadFunction(symbol));                     \
	if (!member)                                                               \
	{                                                                          \
		return false;                                                          \
	}
	LOAD_GL(ClearColor, clearColor, "glClearColor");
	LOAD_GL(Clear, clear, "glClear");
	LOAD_GL(Viewport, viewport, "glViewport");
	LOAD_GL(GenVertexArrays, genVertexArrays, "glGenVertexArrays");
	LOAD_GL(BindVertexArray, bindVertexArray, "glBindVertexArray");
	LOAD_GL(DeleteVertexArrays, deleteVertexArrays, "glDeleteVertexArrays");
	LOAD_GL(GenBuffers, genBuffers, "glGenBuffers");
	LOAD_GL(BindBuffer, bindBuffer, "glBindBuffer");
	LOAD_GL(BufferData, bufferData, "glBufferData");
	LOAD_GL(DeleteBuffers, deleteBuffers, "glDeleteBuffers");
	LOAD_GL(EnableVertexAttribArray, enableVertexAttribArray,
	        "glEnableVertexAttribArray");
	LOAD_GL(VertexAttribPointer, vertexAttribPointer, "glVertexAttribPointer");
	LOAD_GL(VertexAttribIPointer, vertexAttribIPointer,
	        "glVertexAttribIPointer");
	LOAD_GL(VertexAttribDivisor, vertexAttribDivisor, "glVertexAttribDivisor");
	LOAD_GL(CreateShader, createShader, "glCreateShader");
	LOAD_GL(ShaderSource, shaderSource, "glShaderSource");
	LOAD_GL(CompileShader, compileShader, "glCompileShader");
	LOAD_GL(GetShaderiv, getShaderiv, "glGetShaderiv");
	LOAD_GL(GetShaderInfoLog, getShaderInfoLog, "glGetShaderInfoLog");
	LOAD_GL(DeleteShader, deleteShader, "glDeleteShader");
	LOAD_GL(CreateProgram, createProgram, "glCreateProgram");
	LOAD_GL(AttachShader, attachShader, "glAttachShader");
	LOAD_GL(LinkProgram, linkProgram, "glLinkProgram");
	LOAD_GL(GetProgramiv, getProgramiv, "glGetProgramiv");
	LOAD_GL(GetProgramInfoLog, getProgramInfoLog, "glGetProgramInfoLog");
	LOAD_GL(DeleteProgram, deleteProgram, "glDeleteProgram");
	LOAD_GL(UseProgram, useProgram, "glUseProgram");
	LOAD_GL(DrawArrays, drawArrays, "glDrawArrays");
	LOAD_GL(DrawElements, drawElements, "glDrawElements");
	LOAD_GL(DrawElementsInstanced, drawElementsInstanced,
	        "glDrawElementsInstanced");
	LOAD_GL(Enable, enable, "glEnable");
	LOAD_GL(Disable, disable, "glDisable");
	LOAD_GL(BlendFunc, blendFunc, "glBlendFunc");
	LOAD_GL(DepthMask, depthMask, "glDepthMask");
	LOAD_GL(GetError, getError, "glGetError");
	LOAD_GL(GetString, getString, "glGetString");
	LOAD_GL(GetUniformLocation, getUniformLocation, "glGetUniformLocation");
	LOAD_GL(Uniform1i, uniform1i, "glUniform1i");
	LOAD_GL(Uniform1f, uniform1f, "glUniform1f");
	LOAD_GL(Uniform4fv, uniform4fv, "glUniform4fv");
	LOAD_GL(UniformMatrix4fv, uniformMatrix4fv, "glUniformMatrix4fv");
	LOAD_GL(UniformMatrix3fv, uniformMatrix3fv, "glUniformMatrix3fv");
	LOAD_GL(GenFramebuffers, genFramebuffers, "glGenFramebuffers");
	LOAD_GL(BindFramebuffer, bindFramebuffer, "glBindFramebuffer");
	LOAD_GL(FramebufferTexture2D, framebufferTexture2D,
	        "glFramebufferTexture2D");
	LOAD_GL(CheckFramebufferStatus, checkFramebufferStatus,
	        "glCheckFramebufferStatus");
	LOAD_GL(DeleteFramebuffers, deleteFramebuffers, "glDeleteFramebuffers");
	LOAD_GL(GenTextures, genTextures, "glGenTextures");
	LOAD_GL(BindTexture, bindTexture, "glBindTexture");
	LOAD_GL(TexImage2D, texImage2D, "glTexImage2D");
	LOAD_GL(TexParameteri, texParameteri, "glTexParameteri");
	LOAD_GL(DeleteTextures, deleteTextures, "glDeleteTextures");
	LOAD_GL(ActiveTexture, activeTexture, "glActiveTexture");
	LOAD_GL(DrawBuffer, drawBuffer, "glDrawBuffer");
	LOAD_GL(DrawBuffers, drawBuffers, "glDrawBuffers");
	LOAD_GL(ReadBuffer, readBuffer, "glReadBuffer");
	LOAD_GL(PolygonOffset, polygonOffset, "glPolygonOffset");
#undef LOAD_GL

	const char *version = reinterpret_cast<const char *>(getString(GL_VERSION));
	const char *renderer =
	    reinterpret_cast<const char *>(getString(GL_RENDERER));
	const char *shadingLanguage =
	    reinterpret_cast<const char *>(getString(GL_SHADING_LANGUAGE_VERSION));
	std::fprintf(stderr, "OpenGL: version=%s renderer=%s GLSL=%s\n",
	             version ? version : "unavailable",
	             renderer ? renderer : "unavailable",
	             shadingLanguage ? shadingLanguage : "unavailable");

#ifndef NDEBUG
	if (!SDL_GL_ExtensionSupported("GL_KHR_debug"))
	{
		std::fputs(
		    "OpenGL debug callback unavailable: GL_KHR_debug not reported.\n",
		    stderr);
		return true;
	}
	debugMessageCallback = reinterpret_cast<DebugMessageCallback>(
	    loadFunction("glDebugMessageCallback"));
	debugMessageInsert = reinterpret_cast<DebugMessageInsert>(
	    loadFunction("glDebugMessageInsert"));
	if (!debugMessageCallback || !debugMessageInsert)
	{
		return false;
	}
	enable(GL_DEBUG_OUTPUT);
	enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	debugMessageCallback(debugCallback, nullptr);
	debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 1,
	                   GL_DEBUG_SEVERITY_NOTIFICATION, -1,
	                   "Renderer debug callback initialized.");
	std::fputs("OpenGL debug callback enabled.\n", stderr);
#endif
	return true;
}
} // namespace engine::graphics::detail
