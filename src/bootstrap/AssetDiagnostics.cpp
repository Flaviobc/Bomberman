/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   AssetDiagnostics.cpp                             :+:      :+:    :+: :+:      */
/*                                                    +:+ +:+   +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+ +#+#+#+#+#+   +#+      */
/*                                                +#+#+#+#+#+   +#+           +#+#+#+#+#+   */
/*   Created: 2026/10/08          by flaviobc      #+#    #+#             */
/*                                                                                    */
/* ******************************************************************************** */

#include "AssetDiagnostics.hpp"

#include "AnimationPreparation.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <optional>

namespace
{
bool reportBaseColorTextures(const engine::assets::GlbDocument &document)
{
	for (std::size_t materialIndex = 0;
	     materialIndex < document.materials.size(); ++materialIndex)
	{
		const engine::assets::GlbMaterial &material =
		    document.materials[materialIndex];
		if (!material.baseColorTexture)
		{
			continue;
		}
		const engine::assets::GlbBaseColorTexture &reference =
		    *material.baseColorTexture;
		if (reference.textureIndex >= document.textures.size())
		{
			return false;
		}
		const engine::assets::GlbTexture &texture =
		    document.textures[reference.textureIndex];
		if (texture.imageIndex >= document.images.size())
		{
			return false;
		}
		const engine::assets::GlbImage &image =
		    document.images[texture.imageIndex];
		std::fprintf(stderr,
		             "GLB base color texture: material_index=%zu name=%s "
		             "texture_index=%zu image_index=%zu texcoord=%zu mime=%s "
		             "encoded_bytes=%zu width=%zu height=%zu channels=%zu "
		             "rgba8_bytes=%zu\n",
		             materialIndex, material.name.c_str(),
		             reference.textureIndex, texture.imageIndex,
		             reference.texCoord, image.mimeType.c_str(),
		             image.encodedBytes.size(), image.decodedImage.width,
		             image.decodedImage.height, image.decodedImage.channels,
		             image.decodedImage.pixels.size());
		if (image.decodedImage.width == 0 || image.decodedImage.height == 0 ||
		    image.decodedImage.channels != 4 ||
		    image.decodedImage.pixels.empty())
		{
			return false;
		}
		for (const engine::assets::GlbNode &node : document.nodes)
		{
			if (!node.meshIndex || *node.meshIndex >= document.meshes.size())
			{
				continue;
			}
			const engine::assets::GlbMesh &mesh =
			    document.meshes[*node.meshIndex];
			for (std::size_t primitiveIndex = 0;
			     primitiveIndex < mesh.primitives.size(); ++primitiveIndex)
			{
				const engine::assets::GlbPrimitive &primitive =
				    mesh.primitives[primitiveIndex];
				if (primitive.materialIndex != materialIndex)
				{
					continue;
				}
				const std::vector<engine::assets::StaticMeshTexCoord> &uvs =
				    reference.texCoord == 0 ? primitive.source.texCoords0
					                        : primitive.source.texCoords1;
				if (uvs.size() != primitive.source.positions.size())
				{
					return false;
				}
				std::fprintf(
				    stderr,
				    "GLB texture UVs: node=%s primitive=%zu texcoord=%zu "
				    "uvs=%zu positions=%zu\n",
				    node.name.c_str(), primitiveIndex, reference.texCoord,
				    uvs.size(), primitive.source.positions.size());
			}
		}
	}
	return true;
}

bool reportPlayerSkin(const engine::assets::GlbDocument &document)
{
	for (const engine::assets::GlbNode &node : document.nodes)
	{
		if (node.name != "Player")
		{
			continue;
		}
		if (!node.skinIndex || !node.meshIndex ||
		    *node.skinIndex >= document.skins.size() ||
		    *node.meshIndex >= document.meshes.size())
		{
			return false;
		}
		const engine::assets::GlbSkin &skin = document.skins[*node.skinIndex];
		std::fprintf(
		    stderr,
		    "GLB skin: node=%s name=%s joints=%zu inverse_bind_matrices=%zu\n",
		    node.name.c_str(), skin.name.c_str(), skin.jointNodeIndices.size(),
		    skin.inverseBindMatrices.size());
		if (skin.name != "Player_Armature" ||
		    skin.jointNodeIndices.size() != 2 ||
		    skin.inverseBindMatrices.size() != 2)
		{
			return false;
		}
		for (std::size_t jointIndex = 0;
		     jointIndex < skin.jointNodeIndices.size(); ++jointIndex)
		{
			const std::size_t jointNodeIndex =
			    skin.jointNodeIndices[jointIndex];
			if (jointNodeIndex >= document.nodes.size())
			{
				return false;
			}
			std::fprintf(stderr, "GLB skin joint: skin=%s index=%zu node=%s\n",
			             skin.name.c_str(), jointIndex,
			             document.nodes[jointNodeIndex].name.c_str());
		}
		const engine::assets::GlbMesh &mesh = document.meshes[*node.meshIndex];
		for (std::size_t primitiveIndex = 0;
		     primitiveIndex < mesh.primitives.size(); ++primitiveIndex)
		{
			const engine::assets::StaticMeshSource &source =
			    mesh.primitives[primitiveIndex].source;
			std::fprintf(
			    stderr,
			    "GLB skin attributes: node=%s primitive=%zu positions=%zu "
			    "joints=%zu weights=%zu\n",
			    node.name.c_str(), primitiveIndex, source.positions.size(),
			    source.jointIndices.size(), source.jointWeights.size());
			if (source.jointIndices.size() != source.positions.size() ||
			    source.jointWeights.size() != source.positions.size())
			{
				return false;
			}
		}
		return mesh.primitives.size() == 3;
	}
	return false;
}

void reportNodeTransform(const engine::assets::GlbNode &node)
{
	const engine::assets::GlbNodeTransform &transform = node.localTransform;
	std::fprintf(stderr,
	             "GLB node local transform: node=%s translation=(%.3f, %.3f, "
	             "%.3f) rotation=(%.3f, %.3f, %.3f, %.3f) scale=(%.3f, %.3f, "
	             "%.3f)\n",
	             node.name.c_str(), transform.translationX,
	             transform.translationY, transform.translationZ,
	             transform.rotationX, transform.rotationY, transform.rotationZ,
	             transform.rotationW, transform.scaleX, transform.scaleY,
	             transform.scaleZ);
}

bool reportJointHierarchy(const engine::assets::GlbDocument &document)
{
	std::optional<std::size_t> rootIndex;
	std::optional<std::size_t> upperIndex;
	for (std::size_t nodeIndex = 0; nodeIndex < document.nodes.size();
	     ++nodeIndex)
	{
		if (document.nodes[nodeIndex].name == "Player_Root")
		{
			rootIndex = nodeIndex;
		}
		else if (document.nodes[nodeIndex].name == "Player_Upper")
		{
			upperIndex = nodeIndex;
		}
	}
	if (!rootIndex || !upperIndex ||
	    document.nodes[*upperIndex].parentNodeIndex != rootIndex)
	{
		return false;
	}
	const engine::assets::GlbNode &root = document.nodes[*rootIndex];
	if (std::find(root.childNodeIndices.begin(), root.childNodeIndices.end(),
	              *upperIndex) == root.childNodeIndices.end())
	{
		return false;
	}
	std::fprintf(stderr, "GLB joint hierarchy: parent=%s child=%s\n",
	             root.name.c_str(), document.nodes[*upperIndex].name.c_str());
	reportNodeTransform(root);
	reportNodeTransform(document.nodes[*upperIndex]);
	const engine::assets::GlbNodeTransform &rootTransform = root.localTransform;
	return rootTransform.translationX == 0.0F &&
	       rootTransform.translationY == 0.0F &&
	       rootTransform.translationZ == 0.0F &&
	       rootTransform.rotationX == 0.0F && rootTransform.rotationY == 0.0F &&
	       rootTransform.rotationZ == 0.0F && rootTransform.rotationW == 1.0F &&
	       rootTransform.scaleX == 1.0F && rootTransform.scaleY == 1.0F &&
	       rootTransform.scaleZ == 1.0F;
}

const char *animationPathName(engine::assets::GlbAnimationPath path)
{
	return path == engine::assets::GlbAnimationPath::Translation ? "translation"
	                                                             : "rotation";
}

bool reportAnimations(const engine::assets::GlbDocument &document)
{
	std::optional<std::size_t> playerNodeIndex;
	for (std::size_t nodeIndex = 0; nodeIndex < document.nodes.size();
	     ++nodeIndex)
	{
		if (document.nodes[nodeIndex].name == "Player")
		{
			playerNodeIndex = nodeIndex;
			break;
		}
	}
	if (!playerNodeIndex)
	{
		return false;
	}
	for (const engine::assets::GlbAnimationClip &clip : document.animations)
	{
		std::fprintf(stderr,
		             "GLB animation: name=%s duration=%.3f samplers=%zu "
		             "channels=%zu\n",
		             clip.name.c_str(), clip.durationSeconds,
		             clip.samplers.size(), clip.channels.size());
		if ((clip.name != "Idle" && clip.name != "Walk") ||
		    std::fabs(clip.durationSeconds - 1.0F) > 0.001F)
		{
			return false;
		}
		bool hasPlayerTranslation = false;
		bool hasPlayerRotation = false;
		for (const engine::assets::GlbAnimationChannel &channel : clip.channels)
		{
			if (channel.samplerIndex >= clip.samplers.size() ||
			    channel.nodeIndex >= document.nodes.size())
			{
				return false;
			}
			const engine::assets::GlbAnimationSampler &sampler =
			    clip.samplers[channel.samplerIndex];
			const std::size_t valueCount =
			    channel.path == engine::assets::GlbAnimationPath::Translation
			        ? sampler.translations.size()
			        : sampler.rotations.size();
			std::fprintf(stderr,
			             "GLB animation channel: clip=%s path=%s node=%s "
			             "interpolation=LINEAR keys=%zu values=%zu\n",
			             clip.name.c_str(), animationPathName(channel.path),
			             document.nodes[channel.nodeIndex].name.c_str(),
			             sampler.times.size(), valueCount);
			if (sampler.path != channel.path ||
			    sampler.times.size() != valueCount)
			{
				return false;
			}
			if (channel.nodeIndex != *playerNodeIndex)
			{
				std::fprintf(
				    stderr,
				    "GLB animation channel retained for later joint use: "
				    "clip=%s node=%s path=%s\n",
				    clip.name.c_str(),
				    document.nodes[channel.nodeIndex].name.c_str(),
				    animationPathName(channel.path));
				continue;
			}
			bool &hasPath =
			    channel.path == engine::assets::GlbAnimationPath::Translation
			        ? hasPlayerTranslation
			        : hasPlayerRotation;
			if (hasPath)
			{
				return false;
			}
			hasPath = true;
		}
		if (!hasPlayerTranslation || !hasPlayerRotation)
		{
			return false;
		}
	}
	return document.animations.size() == 2;
}

bool reportAuraEmissiveMaterial(const engine::assets::GlbDocument &document)
{
	for (const engine::assets::GlbMaterial &material : document.materials)
	{
		const engine::assets::GlbEmissiveColor &emissive =
		    material.emissiveColor;
		if (!std::isfinite(emissive.red) || !std::isfinite(emissive.green) ||
		    !std::isfinite(emissive.blue))
		{
			return false;
		}
		if (material.name == "MAT_PowerUp_Aura_Gold")
		{
			std::fprintf(stderr,
			             "GLB emissive material: name=%s color=(%.3f, %.3f, "
			             "%.3f)\n",
			             material.name.c_str(), emissive.red, emissive.green,
			             emissive.blue);
			return true;
		}
	}
	return false;
}

} // namespace

bool bootstrap::diagnosePrototypeArena(
    const engine::assets::GlbDocument &document, const std::string &path,
    engine::animation::SkeletalPoseSource &skeletalSource)
{
	if (document.scenes.empty())
	{
		std::fprintf(stderr, "GLB load failed: path=%s error=no scenes\n",
		             path.c_str());
		return false;
	}
	const engine::assets::GlbScene &scene =
	    document.scenes[document.activeSceneIndex];
	std::fprintf(stderr,
	             "GLB loaded: path=%s scene=%s scene_nodes=%zu nodes=%zu "
	             "meshes=%zu materials=%zu\n",
	             path.c_str(), scene.name.c_str(), scene.nodeIndices.size(),
	             document.nodes.size(), document.meshes.size(),
	             document.materials.size());
	std::size_t primitiveCount = 0;
	std::size_t vertexCount = 0;
	std::size_t normalCount = 0;
	std::size_t indexCount = 0;
	for (const engine::assets::GlbMesh &mesh : document.meshes)
	{
		for (const engine::assets::GlbPrimitive &primitive : mesh.primitives)
		{
			++primitiveCount;
			vertexCount += primitive.source.positions.size();
			normalCount += primitive.source.normals.size();
			indexCount += primitive.source.indices.size();
		}
	}
	std::fprintf(stderr,
	             "GLB geometry: primitives=%zu vertices=%zu normals=%zu "
	             "indices=%zu\n",
	             primitiveCount, vertexCount, normalCount, indexCount);
	for (const engine::assets::GlbNode &node : document.nodes)
	{
		if (node.meshIndex)
		{
			std::fprintf(stderr, "GLB node: name=%s mesh=%s\n",
			             node.name.c_str(),
			             document.meshes[*node.meshIndex].name.c_str());
		}
	}
	for (const engine::assets::GlbNode &node : document.nodes)
	{
		if (node.name != "Player" || !node.meshIndex)
		{
			continue;
		}
		const engine::assets::GlbMesh &playerMesh =
		    document.meshes[*node.meshIndex];
		for (std::size_t primitiveIndex = 0;
		     primitiveIndex < playerMesh.primitives.size(); ++primitiveIndex)
		{
			const engine::assets::GlbPrimitive &primitive =
			    playerMesh.primitives[primitiveIndex];
			if (!primitive.materialIndex)
			{
				std::fprintf(stderr,
				             "GLB Player primitive: index=%zu material=none\n",
				             primitiveIndex);
				continue;
			}
			const engine::assets::GlbMaterial &material =
			    document.materials[*primitive.materialIndex];
			std::fprintf(
			    stderr,
			    "GLB Player primitive: index=%zu material_index=%zu name=%s "
			    "base_color=(%.3f, %.3f, %.3f, %.3f)\n",
			    primitiveIndex, *primitive.materialIndex, material.name.c_str(),
			    material.baseColor.red, material.baseColor.green,
			    material.baseColor.blue, material.baseColor.alpha);
		}
	}
	return reportBaseColorTextures(document) && reportPlayerSkin(document) &&
	       reportJointHierarchy(document) &&
	       prepareSkeletalPoseSource(document, skeletalSource) &&
	       reportAuraEmissiveMaterial(document) && reportAnimations(document);
}
