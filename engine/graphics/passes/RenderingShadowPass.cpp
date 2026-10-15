/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingShadowPass.cpp                           :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "graphics/passes/RenderingShadowPass.hpp"

#include "graphics/resources/RenderingPrograms.hpp"
#include "graphics/resources/RenderingShadowMap.hpp"
#include "graphics/resources/RenderingStaticMeshes.hpp"
#include "graphics/runtime/OpenGlFunctions.hpp"

#include <algorithm>

namespace engine::graphics::detail
{
void RenderingShadowPass::execute(
    OpenGlFunctions &gl, const RenderingPrograms &programs,
    const RenderingShadowMap &shadowMap, RenderingStaticMeshes &staticMeshes,
    const std::vector<ShadowPassInstance> &instances,
    const Matrix4 &lightSpaceMatrix, const Matrix4 &playerModelViewProjection,
    const std::array<float, RenderingAnimation::jointCount * 16> &jointPalette,
    std::vector<RenderingStaticMeshes::GpuInstance> &scratch)
{
	gl.bindFramebuffer(GL_FRAMEBUFFER, shadowMap.framebuffer());
	gl.viewport(0, 0, RenderingShadowMap::dimension(),
	            RenderingShadowMap::dimension());
	gl.clear(GL_DEPTH_BUFFER_BIT);
	gl.enable(GL_DEPTH_TEST);
	gl.enable(GL_POLYGON_OFFSET_FILL);
	gl.polygonOffset(2.0F, 4.0F);
	gl.useProgram(programs.shadow().handle);
	gl.uniform1i(programs.shadow().uniforms.useInstancing, 0);
	gl.uniformMatrix4fv(programs.shadow().uniforms.lightSpaceMatrix, 1,
	                    GL_FALSE, lightSpaceMatrix.values);
	gl.uniformMatrix4fv(programs.shadow().uniforms.jointPalette,
	                    static_cast<GLsizei>(RenderingAnimation::jointCount),
	                    GL_FALSE, jointPalette.data());
	if (!staticMeshes.empty())
	{
		gl.uniformMatrix4fv(programs.shadow().uniforms.modelViewProjection, 1,
		                    GL_FALSE, playerModelViewProjection.values);
		for (const RenderingStaticMeshes::Primitive &mesh :
		     staticMeshes.primitives())
		{
			if (mesh.modelId != 0)
			{
				continue;
			}
			gl.uniform1i(programs.shadow().uniforms.useSkinning,
			             mesh.skinned ? 1 : 0);
			gl.bindVertexArray(mesh.vertexArray);
			gl.drawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT,
			                nullptr);
		}
	}
	const auto &primitives = staticMeshes.primitives();
	scratch.clear();
	for (std::size_t primitiveIndex = 0; primitiveIndex < primitives.size();
	     ++primitiveIndex)
	{
		const std::size_t modelId = primitives[primitiveIndex].modelId;
		const bool alreadyVisited = std::any_of(
		    primitives.begin(), primitives.begin() + primitiveIndex,
		    [modelId](const RenderingStaticMeshes::Primitive &primitive)
		    { return primitive.modelId == modelId; });
		const bool hasSkinnedPrimitive = std::any_of(
		    primitives.begin(), primitives.end(),
		    [modelId](const RenderingStaticMeshes::Primitive &primitive)
		    { return primitive.modelId == modelId && primitive.skinned; });
		if (alreadyVisited || hasSkinnedPrimitive)
		{
			continue;
		}
		for (const ShadowPassInstance &instance : instances)
		{
			if (instance.modelId == modelId)
			{
				RenderingStaticMeshes::GpuInstance gpuInstance;
				const float scale = instance.modelMatrix.values[5];
				gpuInstance.positionScale = {instance.modelMatrix.values[12],
				                             instance.modelMatrix.values[13],
				                             instance.modelMatrix.values[14],
				                             scale};
				if (scale != 0.0F)
				{
					gpuInstance.rotation = {
					    instance.modelMatrix.values[0] / scale,
					    instance.modelMatrix.values[8] / scale};
				}
				scratch.push_back(gpuInstance);
			}
		}
	}
	const bool instancesUploaded =
	    staticMeshes.uploadInstances(gl, scratch) > 0;
	std::size_t firstInstance = 0;
	for (std::size_t primitiveIndex = 0; primitiveIndex < primitives.size();
	     ++primitiveIndex)
	{
		const std::size_t modelId = primitives[primitiveIndex].modelId;
		const bool alreadyVisited = std::any_of(
		    primitives.begin(), primitives.begin() + primitiveIndex,
		    [modelId](const RenderingStaticMeshes::Primitive &primitive)
		    { return primitive.modelId == modelId; });
		if (alreadyVisited)
		{
			continue;
		}
		const bool hasSkinnedPrimitive = std::any_of(
		    primitives.begin(), primitives.end(),
		    [modelId](const RenderingStaticMeshes::Primitive &primitive)
		    { return primitive.modelId == modelId && primitive.skinned; });
		if (hasSkinnedPrimitive || !instancesUploaded)
		{
			for (const ShadowPassInstance &instance : instances)
			{
				if (instance.modelId != modelId)
				{
					continue;
				}
				const Matrix4 modelViewProjection =
				    multiplyMatrices(lightSpaceMatrix, instance.modelMatrix);
				gl.uniform1i(programs.shadow().uniforms.useInstancing, 0);
				gl.uniformMatrix4fv(
				    programs.shadow().uniforms.modelViewProjection, 1, GL_FALSE,
				    modelViewProjection.values);
				for (const RenderingStaticMeshes::Primitive &mesh : primitives)
				{
					if (mesh.modelId == modelId)
					{
						gl.uniform1i(programs.shadow().uniforms.useSkinning,
						             mesh.skinned ? 1 : 0);
						gl.bindVertexArray(mesh.vertexArray);
						gl.drawElements(GL_TRIANGLES, mesh.indexCount,
						                GL_UNSIGNED_INT, nullptr);
					}
				}
			}
			continue;
		}
		const std::size_t modelInstanceCount = static_cast<std::size_t>(
		    std::count_if(instances.begin(), instances.end(),
			              [modelId](const ShadowPassInstance &instance)
			              { return instance.modelId == modelId; }));
		if (modelInstanceCount == 0)
		{
			firstInstance += modelInstanceCount;
			continue;
		}
		const GLsizei instanceCount = static_cast<GLsizei>(modelInstanceCount);
		gl.uniform1i(programs.shadow().uniforms.useInstancing, 1);
		gl.uniform1i(programs.shadow().uniforms.useSkinning, 0);
		for (const RenderingStaticMeshes::Primitive &mesh : primitives)
		{
			if (mesh.modelId == modelId)
			{
				staticMeshes.bindInstancedPrimitive(gl, mesh, firstInstance);
				gl.drawElementsInstanced(GL_TRIANGLES, mesh.indexCount,
				                         GL_UNSIGNED_INT, nullptr,
				                         instanceCount);
			}
		}
		firstInstance += modelInstanceCount;
	}
	gl.uniform1i(programs.shadow().uniforms.useInstancing, 0);
	gl.disable(GL_POLYGON_OFFSET_FILL);
}
} // namespace engine::graphics::detail
