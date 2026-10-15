/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingScenePass.cpp                            :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/09          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/09          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "graphics/passes/RenderingScenePass.hpp"

#include "graphics/resources/RenderingPostProcess.hpp"
#include "graphics/resources/RenderingPrograms.hpp"
#include "graphics/resources/RenderingShadowMap.hpp"
#include "graphics/resources/RenderingStaticMeshes.hpp"
#include "graphics/resources/RenderingWorldRectangles.hpp"
#include "graphics/runtime/OpenGlFunctions.hpp"

#include <algorithm>

namespace
{
using engine::graphics::detail::OpenGlFunctions;
using engine::graphics::detail::RenderingPrograms;
using engine::graphics::detail::RenderingStaticMeshes;

using MainProgram = RenderingPrograms::Main;

void drawPrimitive(OpenGlFunctions &gl, const MainProgram &program,
                   const RenderingStaticMeshes::Primitive &mesh,
                   const std::array<float, 4> &instanceEmission,
                   GLuint &boundBaseColorTexture, GLsizei instanceCount = 0)
{
	gl.uniform1i(program.uniforms.useSkinning, mesh.skinned ? 1 : 0);
	gl.uniform4fv(program.uniforms.primitiveColor, 1, mesh.color.data());
	std::array<GLfloat, 4> effectiveEmission = mesh.emissiveColor;
	if (instanceEmission[3] > 0.0F)
	{
		effectiveEmission = {instanceEmission[0] * instanceEmission[3],
		                     instanceEmission[1] * instanceEmission[3],
		                     instanceEmission[2] * instanceEmission[3], 0.0F};
	}
	gl.uniform4fv(program.uniforms.emissiveColor, 1, effectiveEmission.data());
	gl.uniform1i(program.uniforms.useBaseColorTexture,
	             mesh.baseColorTexture != 0 ? 1 : 0);
	if (mesh.baseColorTexture != 0 &&
	    mesh.baseColorTexture != boundBaseColorTexture)
	{
		gl.activeTexture(GL_TEXTURE1);
		gl.bindTexture(GL_TEXTURE_2D, mesh.baseColorTexture);
		boundBaseColorTexture = mesh.baseColorTexture;
	}
	gl.bindVertexArray(mesh.vertexArray);
	if (instanceCount > 0)
	{
		gl.drawElementsInstanced(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT,
		                         nullptr, instanceCount);
	}
	else
	{
		gl.drawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT,
		                nullptr);
	}
}

bool firstPrimitiveForModel(
    std::span<const RenderingStaticMeshes::Primitive> primitives,
    std::size_t primitiveIndex)
{
	return std::none_of(primitives.begin(), primitives.begin() + primitiveIndex,
	                    [modelId = primitives[primitiveIndex].modelId](
	                        const RenderingStaticMeshes::Primitive &primitive)
	                    { return primitive.modelId == modelId; });
}

bool modelSupportsInstancing(
    std::span<const RenderingStaticMeshes::Primitive> primitives,
    std::size_t modelId)
{
	return std::none_of(
	    primitives.begin(), primitives.end(),
	    [modelId](const RenderingStaticMeshes::Primitive &primitive)
	    { return primitive.modelId == modelId && primitive.skinned; });
}

RenderingStaticMeshes::GpuInstance
makeGpuInstance(const engine::graphics::detail::ScenePassInstance &instance)
{
	RenderingStaticMeshes::GpuInstance result;
	result.positionScale = {
	    instance.modelMatrix.values[12], instance.modelMatrix.values[13],
	    instance.modelMatrix.values[14], instance.modelMatrix.values[5]};
	result.rotation = {instance.normalMatrix.values[0],
	                   instance.normalMatrix.values[6]};
	return result;
}

bool sameAppearance(const engine::graphics::detail::ScenePassInstance &left,
                    const engine::graphics::detail::ScenePassInstance &right)
{
	return left.modelId == right.modelId &&
	       left.colorization == right.colorization &&
	       left.emission == right.emission;
}
} // namespace

namespace engine::graphics::detail
{
void RenderingScenePass::execute(
    OpenGlFunctions &gl, const RenderingPrograms &programs,
    const RenderingPostProcess &postProcess,
    const RenderingShadowMap &shadowMap, RenderingStaticMeshes &staticMeshes,
    const RenderingWorldRectangles &worldRectangles,
    const ScenePassInput &input,
    std::vector<RenderingStaticMeshes::GpuInstance> &scratch)
{
	const MainProgram &program = programs.main();
	// Only this pass binds unit 1 until its last mesh draw. Start unknown
	// each invocation; never rely on texture state left by another pass.
	GLuint boundBaseColorTexture = 0;
	gl.bindFramebuffer(GL_FRAMEBUFFER, postProcess.hdrFramebuffer());
	gl.viewport(0, 0, input.viewportWidth, input.viewportHeight);
	gl.clearColor(0.04F, 0.06F, 0.1F, 1.0F);
	gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	gl.useProgram(program.handle);
	gl.uniform1i(program.uniforms.useInstancing, 0);
	gl.uniformMatrix4fv(program.uniforms.worldViewProjection, 1, GL_FALSE,
	                    input.worldViewProjection.values);
	gl.uniformMatrix4fv(program.uniforms.jointPalette,
	                    static_cast<GLsizei>(RenderingAnimation::jointCount),
	                    GL_FALSE, input.jointPalette.data());
	gl.uniformMatrix4fv(program.uniforms.lightSpaceMatrix, 1, GL_FALSE,
	                    input.lightSpaceMatrix.values);
	gl.activeTexture(GL_TEXTURE0);
	gl.bindTexture(GL_TEXTURE_2D, shadowMap.depthTexture());
	gl.uniform1i(program.uniforms.pointLightActive,
	             input.pointLightActive ? 1 : 0);
	if (input.pointLightActive)
	{
		gl.uniform4fv(program.uniforms.pointLightPositionIntensity, 1,
		              input.pointLightPositionIntensity.data());
		gl.uniform4fv(program.uniforms.pointLightColorRadius, 1,
		              input.pointLightColorRadius.data());
	}
	gl.enable(GL_DEPTH_TEST);
	gl.uniform1i(program.uniforms.usePrimitiveColor, 0);
	gl.uniform1i(program.uniforms.useBaseColorTexture, 0);
	constexpr GLfloat neutralColorization[] = {1.0F, 1.0F, 1.0F, 0.0F};
	gl.uniform4fv(program.uniforms.instanceColorization, 1,
	              neutralColorization);
	gl.uniformMatrix4fv(program.uniforms.modelViewProjection, 1, GL_FALSE,
	                    input.worldViewProjection.values);
	if (!staticMeshes.empty())
	{
		gl.uniform1i(program.uniforms.usePrimitiveColor, 1);
		gl.uniformMatrix4fv(program.uniforms.modelViewProjection, 1, GL_FALSE,
		                    input.playerModelViewProjection.values);
		gl.uniformMatrix4fv(program.uniforms.modelMatrix, 1, GL_FALSE,
		                    input.playerModelMatrix.values);
		gl.uniformMatrix3fv(program.uniforms.normalMatrix, 1, GL_FALSE,
		                    input.playerNormalMatrix.values);
		for (const RenderingStaticMeshes::Primitive &mesh :
		     staticMeshes.primitives())
		{
			if (mesh.modelId != input.animatedModelId)
			{
				continue;
			}
			drawPrimitive(gl, program, mesh, {}, boundBaseColorTexture);
		}
	}
	const std::span<const RenderingStaticMeshes::Primitive> primitives =
	    staticMeshes.primitives();
	// This path is opaque and depth-tested. Grouping by model and exact
	// appearance therefore preserves the result while keeping distinct tint and
	// emission values in separate batches.
	scratch.clear();
	for (std::size_t primitiveIndex = 0; primitiveIndex < primitives.size();
	     ++primitiveIndex)
	{
		if (!firstPrimitiveForModel(primitives, primitiveIndex))
		{
			continue;
		}
		const std::size_t modelId = primitives[primitiveIndex].modelId;
		if (!modelSupportsInstancing(primitives, modelId))
		{
			continue;
		}
		for (std::size_t instanceIndex = 0;
		     instanceIndex < input.instances.size(); ++instanceIndex)
		{
			const ScenePassInstance &instance = input.instances[instanceIndex];
			if (instance.modelId != modelId)
			{
				continue;
			}
			const bool appearanceAlreadyVisited =
			    std::any_of(input.instances.begin(),
				            input.instances.begin() + instanceIndex,
				            [&instance](const ScenePassInstance &candidate)
				            { return sameAppearance(candidate, instance); });
			if (appearanceAlreadyVisited)
			{
				continue;
			}
			for (const ScenePassInstance &candidate : input.instances)
			{
				if (sameAppearance(candidate, instance))
				{
					scratch.push_back(makeGpuInstance(candidate));
				}
			}
		}
	}
	const bool instancesUploaded =
	    staticMeshes.uploadInstances(gl, scratch) > 0;
	std::size_t firstInstance = 0;
	for (std::size_t primitiveIndex = 0; primitiveIndex < primitives.size();
	     ++primitiveIndex)
	{
		if (!firstPrimitiveForModel(primitives, primitiveIndex))
		{
			continue;
		}
		const std::size_t modelId = primitives[primitiveIndex].modelId;
		if (!modelSupportsInstancing(primitives, modelId) || !instancesUploaded)
		{
			for (const ScenePassInstance &instance : input.instances)
			{
				if (instance.modelId != modelId)
				{
					continue;
				}
				const Matrix4 modelViewProjection = multiplyMatrices(
				    input.worldViewProjection, instance.modelMatrix);
				gl.uniform1i(program.uniforms.useInstancing, 0);
				gl.uniform1i(program.uniforms.usePrimitiveColor, 1);
				gl.uniform4fv(program.uniforms.instanceColorization, 1,
				              instance.colorization.data());
				gl.uniformMatrix4fv(program.uniforms.modelViewProjection, 1,
				                    GL_FALSE, modelViewProjection.values);
				gl.uniformMatrix4fv(program.uniforms.modelMatrix, 1, GL_FALSE,
				                    instance.modelMatrix.values);
				gl.uniformMatrix3fv(program.uniforms.normalMatrix, 1, GL_FALSE,
				                    instance.normalMatrix.values);
				for (const RenderingStaticMeshes::Primitive &mesh : primitives)
				{
					if (mesh.modelId == modelId)
					{
						drawPrimitive(gl, program, mesh, instance.emission,
						              boundBaseColorTexture);
					}
				}
			}
			continue;
		}
		for (std::size_t instanceIndex = 0;
		     instanceIndex < input.instances.size(); ++instanceIndex)
		{
			const ScenePassInstance &instance = input.instances[instanceIndex];
			if (instance.modelId != modelId)
			{
				continue;
			}
			const bool appearanceAlreadyVisited =
			    std::any_of(input.instances.begin(),
				            input.instances.begin() + instanceIndex,
				            [&instance](const ScenePassInstance &candidate)
				            { return sameAppearance(candidate, instance); });
			if (appearanceAlreadyVisited)
			{
				continue;
			}
			const std::size_t batchSize = static_cast<std::size_t>(
			    std::count_if(input.instances.begin(), input.instances.end(),
				              [&instance](const ScenePassInstance &candidate)
				              { return sameAppearance(candidate, instance); }));
			if (batchSize == 0)
			{
				continue;
			}
			const GLsizei instanceCount = static_cast<GLsizei>(batchSize);
			gl.uniform1i(program.uniforms.useInstancing, 1);
			gl.uniform1i(program.uniforms.usePrimitiveColor, 1);
			gl.uniform4fv(program.uniforms.instanceColorization, 1,
			              instance.colorization.data());
			for (const RenderingStaticMeshes::Primitive &mesh : primitives)
			{
				if (mesh.modelId == modelId)
				{
					staticMeshes.bindInstancedPrimitive(gl, mesh,
					                                    firstInstance);
					drawPrimitive(gl, program, mesh, instance.emission,
					              boundBaseColorTexture, instanceCount);
				}
			}
			firstInstance += batchSize;
		}
	}
	gl.uniform1i(program.uniforms.useInstancing, 0);
	gl.uniform1i(program.uniforms.useSkinning, 0);
	gl.uniform1i(program.uniforms.useBaseColorTexture, 0);
	gl.uniform1i(program.uniforms.usePrimitiveColor, 0);
	gl.uniform4fv(program.uniforms.instanceColorization, 1,
	              neutralColorization);
	constexpr GLfloat noEmission[] = {0.0F, 0.0F, 0.0F, 0.0F};
	gl.uniform4fv(program.uniforms.emissiveColor, 1, noEmission);
	if (worldRectangles.vertexCount() > 0)
	{
		gl.uniformMatrix4fv(program.uniforms.modelViewProjection, 1, GL_FALSE,
		                    input.worldViewProjection.values);
		gl.uniformMatrix4fv(program.uniforms.modelMatrix, 1, GL_FALSE,
		                    input.worldRectangleModelMatrix.values);
		gl.uniformMatrix3fv(program.uniforms.normalMatrix, 1, GL_FALSE,
		                    input.worldRectangleNormalMatrix.values);
		gl.bindVertexArray(worldRectangles.vertexArray());
		gl.drawArrays(GL_TRIANGLES, 0, worldRectangles.vertexCount());
	}
}
} // namespace engine::graphics::detail
