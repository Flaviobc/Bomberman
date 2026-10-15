/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingStaticMeshes.cpp                         :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "graphics/resources/RenderingStaticMeshes.hpp"

#include "graphics/Renderer.hpp"
#include "graphics/runtime/OpenGlFunctions.hpp"

#include "assets/GlbLoader.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <type_traits>

namespace
{
constexpr std::size_t kJointCount = 2;

struct StaticMeshVertex
{
	std::array<GLfloat, 3> position;
	std::array<GLfloat, 3> color;
	std::array<GLfloat, 3> normal;
	std::array<GLfloat, 2> texCoord;
	std::array<GLuint, 4> jointIndices;
	std::array<GLfloat, 4> jointWeights;
};

static_assert(std::is_standard_layout_v<
              engine::graphics::detail::RenderingStaticMeshes::GpuInstance>);

void configureInstanceAttributes(engine::graphics::detail::OpenGlFunctions &gl,
                                 GLuint instanceBuffer,
                                 std::size_t firstInstance)
{
	using GpuInstance =
	    engine::graphics::detail::RenderingStaticMeshes::GpuInstance;
	constexpr GLsizei stride = static_cast<GLsizei>(sizeof(GpuInstance));
	const std::size_t baseOffset = firstInstance * sizeof(GpuInstance);
	gl.bindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
	gl.vertexAttribPointer(
	    6, 4, GL_FLOAT, GL_FALSE, stride,
	    reinterpret_cast<const void *>(baseOffset +
		                               offsetof(GpuInstance, positionScale)));
	gl.enableVertexAttribArray(6);
	gl.vertexAttribDivisor(6, 1);
	gl.vertexAttribPointer(7, 2, GL_FLOAT, GL_FALSE, stride,
	                       reinterpret_cast<const void *>(
	                           baseOffset + offsetof(GpuInstance, rotation)));
	gl.enableVertexAttribArray(7);
	gl.vertexAttribDivisor(7, 1);
}
} // namespace

namespace engine::graphics::detail
{
bool RenderingStaticMeshes::upload(OpenGlFunctions &gl,
                                   const std::vector<StaticMeshInput> &inputs)
{
	if (!primitives_.empty() || inputs.empty())
	{
		std::fputs("Static meshes require one initialized upload.\n", stderr);
		return false;
	}
	for (const StaticMeshInput &input : inputs)
	{
		if (!input.source || input.source->positions.empty() ||
		    input.source->normals.size() != input.source->positions.size() ||
		    input.source->indices.empty() ||
		    input.source->indices.size() >
		        static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()))
		{
			std::fputs("Static mesh input is invalid.\n", stderr);
			return false;
		}
		if (input.skinned)
		{
			if (input.source->jointIndices.size() !=
			        input.source->positions.size() ||
			    input.source->jointWeights.size() !=
			        input.source->positions.size())
			{
				std::fputs("Skinned mesh input is incomplete.\n", stderr);
				return false;
			}
			for (const assets::StaticMeshJointIndices &joints :
			     input.source->jointIndices)
			{
				if (joints.first >= kJointCount ||
				    joints.second >= kJointCount ||
				    joints.third >= kJointCount || joints.fourth >= kJointCount)
				{
					std::fputs("Skinned mesh joint index is invalid.\n",
					           stderr);
					return false;
				}
			}
		}
		if (input.baseColorImage)
		{
			const assets::DecodedImageRgba8 &image = *input.baseColorImage;
			const std::size_t maximumSize =
			    std::numeric_limits<std::size_t>::max();
			if (!input.baseColorTexCoords ||
			    input.baseColorTexCoords->size() !=
			        input.source->positions.size() ||
			    image.width == 0 || image.height == 0 || image.channels != 4 ||
			    image.width > maximumSize / image.channels ||
			    image.height > maximumSize / (image.width * image.channels) ||
			    image.width > static_cast<std::size_t>(
			                      std::numeric_limits<GLsizei>::max()) ||
			    image.height > static_cast<std::size_t>(
			                       std::numeric_limits<GLsizei>::max()) ||
			    image.pixels.size() !=
			        image.width * image.height * image.channels)
			{
				std::fputs("Static mesh base color texture input is invalid.\n",
				           stderr);
				return false;
			}
		}
	}

	std::size_t totalVertices = 0;
	std::size_t totalIndices = 0;
	std::vector<const assets::DecodedImageRgba8 *> uploadedImages;
	gl.genBuffers(1, &instanceBuffer_);
	primitives_.reserve(inputs.size());
	for (const StaticMeshInput &input : inputs)
	{
		std::vector<StaticMeshVertex> vertices;
		vertices.reserve(input.source->positions.size());
		for (std::size_t index = 0; index < input.source->positions.size();
		     ++index)
		{
			const assets::StaticMeshPosition &position =
			    input.source->positions[index];
			const assets::StaticMeshNormal &normal =
			    input.source->normals[index];
			const assets::StaticMeshTexCoord texCoord =
			    input.baseColorTexCoords ? (*input.baseColorTexCoords)[index]
				                         : assets::StaticMeshTexCoord{};
			assets::StaticMeshJointIndices jointIndices{};
			assets::StaticMeshJointWeights jointWeights{1.0F, 0.0F, 0.0F, 0.0F};
			if (input.skinned)
			{
				jointIndices = input.source->jointIndices[index];
				jointWeights = input.source->jointWeights[index];
			}
			// CPU positions are copied to the GPU; color remains a draw
			// uniform; skinned primitives also copy their four influences.
			vertices.push_back({{position.x, position.y, position.z},
			                    {1.0F, 1.0F, 1.0F},
			                    {normal.x, normal.y, normal.z},
			                    {texCoord.u, texCoord.v},
			                    {jointIndices.first, jointIndices.second,
			                     jointIndices.third, jointIndices.fourth},
			                    {jointWeights.first, jointWeights.second,
			                     jointWeights.third, jointWeights.fourth}});
		}

		Primitive resource{};
		resource.modelId = input.modelId;
		resource.skinned = input.skinned;
		resource.color = {input.red, input.green, input.blue, input.alpha};
		resource.emissiveColor = {input.emissiveRed, input.emissiveGreen,
		                          input.emissiveBlue, 0.0F};
		if (input.baseColorImage)
		{
			for (std::size_t imageIndex = 0; imageIndex < uploadedImages.size();
			     ++imageIndex)
			{
				if (uploadedImages[imageIndex] == input.baseColorImage)
				{
					resource.baseColorTexture = baseColorTextures_[imageIndex];
					break;
				}
			}
			if (resource.baseColorTexture == 0)
			{
				const assets::DecodedImageRgba8 &image = *input.baseColorImage;
				gl.genTextures(1, &resource.baseColorTexture);
				gl.bindTexture(GL_TEXTURE_2D, resource.baseColorTexture);
				gl.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
				              static_cast<GLsizei>(image.width),
				              static_cast<GLsizei>(image.height), 0, GL_RGBA,
				              GL_UNSIGNED_BYTE, image.pixels.data());
				gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
				                 GL_LINEAR);
				gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
				                 GL_LINEAR);
				gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
				                 GL_CLAMP_TO_EDGE);
				gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
				                 GL_CLAMP_TO_EDGE);
				uploadedImages.push_back(input.baseColorImage);
				baseColorTextures_.push_back(resource.baseColorTexture);
				std::fprintf(stderr,
				             "Base color texture uploaded: image=%zu width=%zu "
				             "height=%zu channels=%zu.\n",
				             baseColorTextures_.size() - 1, image.width,
				             image.height, image.channels);
			}
		}
		gl.genVertexArrays(1, &resource.vertexArray);
		gl.bindVertexArray(resource.vertexArray);
		gl.genBuffers(1, &resource.vertexBuffer);
		gl.bindBuffer(GL_ARRAY_BUFFER, resource.vertexBuffer);
		gl.bufferData(
		    GL_ARRAY_BUFFER,
		    static_cast<GLsizeiptr>(vertices.size() * sizeof(StaticMeshVertex)),
		    vertices.data(), GL_STATIC_DRAW);
		constexpr GLsizei stride =
		    static_cast<GLsizei>(sizeof(StaticMeshVertex));
		gl.vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
		                       reinterpret_cast<const void *>(
		                           offsetof(StaticMeshVertex, position)));
		gl.enableVertexAttribArray(0);
		gl.vertexAttribPointer(
		    1, 3, GL_FLOAT, GL_FALSE, stride,
		    reinterpret_cast<const void *>(offsetof(StaticMeshVertex, color)));
		gl.enableVertexAttribArray(1);
		gl.vertexAttribPointer(
		    2, 3, GL_FLOAT, GL_FALSE, stride,
		    reinterpret_cast<const void *>(offsetof(StaticMeshVertex, normal)));
		gl.enableVertexAttribArray(2);
		gl.vertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride,
		                       reinterpret_cast<const void *>(
		                           offsetof(StaticMeshVertex, texCoord)));
		gl.enableVertexAttribArray(3);
		gl.vertexAttribIPointer(4, 4, GL_UNSIGNED_INT, stride,
		                        reinterpret_cast<const void *>(
		                            offsetof(StaticMeshVertex, jointIndices)));
		gl.enableVertexAttribArray(4);
		gl.vertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, stride,
		                       reinterpret_cast<const void *>(
		                           offsetof(StaticMeshVertex, jointWeights)));
		gl.enableVertexAttribArray(5);
		gl.genBuffers(1, &resource.indexBuffer);
		gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, resource.indexBuffer);
		gl.bufferData(GL_ELEMENT_ARRAY_BUFFER,
		              static_cast<GLsizeiptr>(input.source->indices.size() *
		                                      sizeof(std::uint32_t)),
		              input.source->indices.data(), GL_STATIC_DRAW);
		configureInstanceAttributes(gl, instanceBuffer_, 0);
		resource.indexCount =
		    static_cast<GLsizei>(input.source->indices.size());
		primitives_.push_back(resource);
		totalVertices += input.source->positions.size();
		totalIndices += input.source->indices.size();
		std::fprintf(stderr,
		             "Static mesh primitive uploaded: index=%zu vertices=%zu "
		             "indices=%d color=(%.3f, %.3f, %.3f, %.3f) "
		             "emissive=(%.3f, %.3f, %.3f) texture=%s skinned=%s.\n",
		             primitives_.size() - 1, input.source->positions.size(),
		             resource.indexCount, input.red, input.green, input.blue,
		             input.alpha, input.emissiveRed, input.emissiveGreen,
		             input.emissiveBlue,
		             resource.baseColorTexture != 0 ? "yes" : "no",
		             resource.skinned ? "yes" : "no");
	}
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.bindVertexArray(0);
	std::fprintf(stderr,
	             "Static mesh collection uploaded: primitives=%zu vertices=%zu "
	             "indices=%zu.\n",
	             primitives_.size(), totalVertices, totalIndices);
	return true;
}

void RenderingStaticMeshes::destroy(OpenGlFunctions &gl)
{
	if (instanceBuffer_ != 0)
	{
		gl.deleteBuffers(1, &instanceBuffer_);
		instanceBuffer_ = 0;
	}
	for (GLuint &texture : baseColorTextures_)
	{
		if (texture != 0)
		{
			gl.deleteTextures(1, &texture);
			texture = 0;
		}
	}
	for (Primitive &primitive : primitives_)
	{
		if (primitive.vertexBuffer != 0)
		{
			gl.deleteBuffers(1, &primitive.vertexBuffer);
			primitive.vertexBuffer = 0;
		}
		if (primitive.indexBuffer != 0)
		{
			gl.deleteBuffers(1, &primitive.indexBuffer);
			primitive.indexBuffer = 0;
		}
		if (primitive.vertexArray != 0)
		{
			gl.deleteVertexArrays(1, &primitive.vertexArray);
			primitive.vertexArray = 0;
		}
		primitive.baseColorTexture = 0;
	}
	baseColorTextures_.clear();
	primitives_.clear();
}

bool RenderingStaticMeshes::empty() const
{
	return primitives_.empty();
}

const std::vector<RenderingStaticMeshes::Primitive> &
RenderingStaticMeshes::primitives() const
{
	return primitives_;
}

std::size_t RenderingStaticMeshes::textureCount() const
{
	return baseColorTextures_.size();
}

GLsizei RenderingStaticMeshes::uploadInstances(
    OpenGlFunctions &gl, std::span<const GpuInstance> instances) const
{
	if (instanceBuffer_ == 0 || instances.empty() ||
	    instances.size() >
	        static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()) ||
	    instances.size() >
	        static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max()) /
	            sizeof(GpuInstance))
	{
		return 0;
	}
	gl.bindBuffer(GL_ARRAY_BUFFER, instanceBuffer_);
	gl.bufferData(
	    GL_ARRAY_BUFFER,
	    static_cast<GLsizeiptr>(instances.size() * sizeof(GpuInstance)),
	    instances.data(), GL_STREAM_DRAW);
	return static_cast<GLsizei>(instances.size());
}

void RenderingStaticMeshes::bindInstancedPrimitive(
    OpenGlFunctions &gl, const Primitive &primitive,
    std::size_t firstInstance) const
{
	gl.bindVertexArray(primitive.vertexArray);
	configureInstanceAttributes(gl, instanceBuffer_, firstInstance);
}
} // namespace engine::graphics::detail
