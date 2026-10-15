/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingStaticMeshes.hpp                         :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

#include <SDL3/SDL_opengl.h>

#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace engine::graphics
{
struct StaticMeshInput;

namespace detail
{
class OpenGlFunctions;

// Owns the immutable GPU geometry and optional base-color textures uploaded
// synchronously from the public Renderer input. CPU asset references are not
// retained after upload.
class RenderingStaticMeshes
{
  public:
	struct GpuInstance
	{
		// Public transforms are translation, Y rotation and uniform scale, so
		// the shader can reconstruct the matrices from six floats without
		// losing data.
		std::array<GLfloat, 4> positionScale{};
		std::array<GLfloat, 2> rotation{1.0F, 0.0F};
	};

	struct Primitive
	{
		GLuint vertexArray = 0;
		GLuint vertexBuffer = 0;
		GLuint indexBuffer = 0;
		GLuint baseColorTexture = 0;
		GLsizei indexCount = 0;
		std::size_t modelId = 0;
		bool skinned = false;
		std::array<GLfloat, 4> color{};
		std::array<GLfloat, 4> emissiveColor{};
	};

	[[nodiscard]] bool upload(OpenGlFunctions &gl,
	                          const std::vector<StaticMeshInput> &inputs);
	void destroy(OpenGlFunctions &gl);

	[[nodiscard]] bool empty() const;
	[[nodiscard]] const std::vector<Primitive> &primitives() const;
	[[nodiscard]] std::size_t textureCount() const;
	[[nodiscard]] GLsizei
	uploadInstances(OpenGlFunctions &gl,
	                std::span<const GpuInstance> instances) const;
	void bindInstancedPrimitive(OpenGlFunctions &gl, const Primitive &primitive,
	                            std::size_t firstInstance) const;

  private:
	std::vector<Primitive> primitives_;
	std::vector<GLuint> baseColorTextures_;
	GLuint instanceBuffer_ = 0;
};
} // namespace detail
} // namespace engine::graphics
