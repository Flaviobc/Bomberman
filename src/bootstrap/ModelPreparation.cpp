/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   ModelPreparation.cpp                             :+:      :+:    :+: :+:      */
/*                                                    +:+ +:+   +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+ +#+#+#+#+#+   +#+      */
/*                                                +#+#+#+#+#+   +#+           +#+#+#+#+#+   */
/*   Created: 2026/10/08          by flaviobc      #+#    #+#             */
/*                                                                                    */
/* ******************************************************************************** */

#include "ModelPreparation.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>

namespace
{
using MeshBounds = game::ModelBounds;

bool collectMeshInputs(const engine::assets::GlbDocument &document,
                       const char *nodeName, std::size_t modelId,
                       std::vector<engine::graphics::StaticMeshInput> &inputs)
{
	for (const engine::assets::GlbNode &node : document.nodes)
	{
		if (node.name != nodeName || !node.meshIndex ||
		    *node.meshIndex >= document.meshes.size())
		{
			continue;
		}
		const engine::assets::GlbMesh &mesh = document.meshes[*node.meshIndex];
		for (const engine::assets::GlbPrimitive &primitive : mesh.primitives)
		{
			if (primitive.source.positions.empty() ||
			    primitive.source.indices.empty() ||
			    primitive.source.normals.empty() ||
			    primitive.source.normals.size() !=
			        primitive.source.positions.size() ||
			    !primitive.materialIndex ||
			    *primitive.materialIndex >= document.materials.size())
			{
				std::fprintf(stderr,
				             "GLB mesh selection failed: node=%s has invalid "
				             "normal data.\n",
				             nodeName);
				return false;
			}
			const engine::assets::GlbBaseColor &color =
			    document.materials[*primitive.materialIndex].baseColor;
			const engine::assets::GlbMaterial &material =
			    document.materials[*primitive.materialIndex];
			const engine::assets::GlbEmissiveColor &emissive =
			    material.emissiveColor;
			const engine::assets::DecodedImageRgba8 *baseColorImage = nullptr;
			const std::vector<engine::assets::StaticMeshTexCoord>
			    *baseColorTexCoords = nullptr;
			if (material.baseColorTexture)
			{
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
				baseColorImage =
				    &document.images[texture.imageIndex].decodedImage;
				baseColorTexCoords = reference.texCoord == 0
				                         ? &primitive.source.texCoords0
				                         : &primitive.source.texCoords1;
			}
			std::fprintf(stderr,
			             "GLB selected primitive: node=%s positions=%zu "
			             "normals=%zu indices=%zu\n",
			             nodeName, primitive.source.positions.size(),
			             primitive.source.normals.size(),
			             primitive.source.indices.size());
			inputs.push_back({&primitive.source, modelId,
			                  node.skinIndex.has_value(), color.red,
			                  color.green, color.blue, color.alpha,
			                  emissive.red, emissive.green, emissive.blue,
			                  baseColorImage, baseColorTexCoords});
		}
		return !inputs.empty();
	}
	return false;
}

} // namespace

bool findMeshBounds(const engine::assets::GlbDocument &document,
                    const char *nodeName, MeshBounds &bounds)
{
	for (const engine::assets::GlbNode &node : document.nodes)
	{
		if (node.name != nodeName || !node.meshIndex ||
		    *node.meshIndex >= document.meshes.size())
		{
			continue;
		}
		float minimumX = std::numeric_limits<float>::max();
		float maximumX = std::numeric_limits<float>::lowest();
		float minimumY = std::numeric_limits<float>::max();
		float maximumY = std::numeric_limits<float>::lowest();
		float minimumZ = std::numeric_limits<float>::max();
		float maximumZ = std::numeric_limits<float>::lowest();
		float maximumHorizontalRadiusSquared = 0.0F;
		for (const engine::assets::GlbPrimitive &primitive :
		     document.meshes[*node.meshIndex].primitives)
		{
			for (const engine::assets::StaticMeshPosition &position :
			     primitive.source.positions)
			{
				minimumX = std::min(minimumX, position.x);
				maximumX = std::max(maximumX, position.x);
				minimumY = std::min(minimumY, position.y);
				maximumY = std::max(maximumY, position.y);
				minimumZ = std::min(minimumZ, position.z);
				maximumZ = std::max(maximumZ, position.z);
				maximumHorizontalRadiusSquared =
				    std::max(maximumHorizontalRadiusSquared,
					         position.x * position.x + position.z * position.z);
			}
		}
		if (minimumX > maximumX || minimumZ > maximumZ)
		{
			return false;
		}
		bounds = {minimumX,
		          maximumX,
		          minimumY,
		          maximumY,
		          minimumZ,
		          maximumZ,
		          std::sqrt(maximumHorizontalRadiusSquared)};
		return maximumX > minimumX && maximumZ > minimumZ;
	}
	return false;
}

bool bootstrap::prepareModels(const engine::assets::GlbDocument &document,
                              PreparedModels &prepared)
{
	const ModelIds &ids = prepared.ids;
	MeshBounds playerBounds{};
	MeshBounds indestructibleWallBounds{};
	MeshBounds labyrinthPillarBounds{};
	MeshBounds destructibleWallBounds{};
	MeshBounds bombBounds{};
	MeshBounds floorBounds{};
	MeshBounds powerUpBombCapacityBounds{};
	MeshBounds powerUpAuraBounds{};
	MeshBounds powerUpBlastRangeBounds{};
	MeshBounds powerUpLifeBounds{};
	MeshBounds enemyBasicBounds{};
	if (!collectMeshInputs(document, "Player", ids.player, prepared.inputs) ||
	    !collectMeshInputs(document, "Wall_Indestructible",
	                       ids.indestructibleWall, prepared.inputs) ||
	    !collectMeshInputs(document, "Wall_Labyrinth_Pillar",
	                       ids.labyrinthPillar, prepared.inputs) ||
	    !collectMeshInputs(document, "Wall_Destructible", ids.destructibleWall,
	                       prepared.inputs) ||
	    !collectMeshInputs(document, "Bomb", ids.bomb, prepared.inputs) ||
	    !collectMeshInputs(document, "Floor_Tile", ids.floor,
	                       prepared.inputs) ||
	    !collectMeshInputs(document, "PowerUp_BombCapacity",
	                       ids.powerUpBombCapacity, prepared.inputs) ||
	    !collectMeshInputs(document, "PowerUp_Aura", ids.powerUpAura,
	                       prepared.inputs) ||
	    !collectMeshInputs(document, "PowerUp_BlastRange",
	                       ids.powerUpBlastRange, prepared.inputs) ||
	    !collectMeshInputs(document, "PowerUp_Life", ids.powerUpLife,
	                       prepared.inputs) ||
	    !collectMeshInputs(document, "Enemy_Basic", ids.enemyBasic,
	                       prepared.inputs) ||
	    !findMeshBounds(document, "Player", playerBounds) ||
	    !findMeshBounds(document, "Wall_Indestructible",
	                    indestructibleWallBounds) ||
	    !findMeshBounds(document, "Wall_Labyrinth_Pillar",
	                    labyrinthPillarBounds) ||
	    !findMeshBounds(document, "Wall_Destructible",
	                    destructibleWallBounds) ||
	    !findMeshBounds(document, "Bomb", bombBounds) ||
	    !findMeshBounds(document, "Floor_Tile", floorBounds) ||
	    !findMeshBounds(document, "PowerUp_BombCapacity",
	                    powerUpBombCapacityBounds) ||
	    !findMeshBounds(document, "PowerUp_Aura", powerUpAuraBounds) ||
	    !findMeshBounds(document, "PowerUp_BlastRange",
	                    powerUpBlastRangeBounds) ||
	    !findMeshBounds(document, "PowerUp_Life", powerUpLifeBounds) ||
	    !findMeshBounds(document, "Enemy_Basic", enemyBasicBounds))
	{
		std::fprintf(
		    stderr,
		    "GLB mesh selection failed: required model is incomplete.\n");
		return false;
	}
	std::size_t playerVertexCount = 0;
	std::size_t playerIndexCount = 0;
	std::size_t bombPrimitiveCount = 0;
	std::size_t bombVertexCount = 0;
	std::size_t bombIndexCount = 0;
	std::size_t floorPrimitiveCount = 0;
	std::size_t floorVertexCount = 0;
	std::size_t floorIndexCount = 0;
	std::size_t powerUpPrimitiveCount = 0;
	std::size_t powerUpVertexCount = 0;
	std::size_t powerUpIndexCount = 0;
	std::size_t powerUpAuraPrimitiveCount = 0;
	std::size_t powerUpAuraVertexCount = 0;
	std::size_t powerUpAuraIndexCount = 0;
	std::size_t powerUpBlastRangePrimitiveCount = 0;
	std::size_t powerUpBlastRangeVertexCount = 0;
	std::size_t powerUpBlastRangeIndexCount = 0;
	std::size_t powerUpLifePrimitiveCount = 0;
	std::size_t powerUpLifeVertexCount = 0;
	std::size_t powerUpLifeIndexCount = 0;
	for (const engine::graphics::StaticMeshInput &input : prepared.inputs)
	{
		playerVertexCount += input.source->positions.size();
		playerIndexCount += input.source->indices.size();
		if (input.modelId == ids.bomb)
		{
			++bombPrimitiveCount;
			bombVertexCount += input.source->positions.size();
			bombIndexCount += input.source->indices.size();
		}
		if (input.modelId == ids.floor)
		{
			++floorPrimitiveCount;
			floorVertexCount += input.source->positions.size();
			floorIndexCount += input.source->indices.size();
		}
		if (input.modelId == ids.powerUpBombCapacity)
		{
			++powerUpPrimitiveCount;
			powerUpVertexCount += input.source->positions.size();
			powerUpIndexCount += input.source->indices.size();
		}
		if (input.modelId == ids.powerUpAura)
		{
			++powerUpAuraPrimitiveCount;
			powerUpAuraVertexCount += input.source->positions.size();
			powerUpAuraIndexCount += input.source->indices.size();
		}
		if (input.modelId == ids.powerUpBlastRange)
		{
			++powerUpBlastRangePrimitiveCount;
			powerUpBlastRangeVertexCount += input.source->positions.size();
			powerUpBlastRangeIndexCount += input.source->indices.size();
		}
		if (input.modelId == ids.powerUpLife)
		{
			++powerUpLifePrimitiveCount;
			powerUpLifeVertexCount += input.source->positions.size();
			powerUpLifeIndexCount += input.source->indices.size();
		}
	}
	std::fprintf(stderr,
	             "GLB mesh selection: models=Player,Wall_Indestructible,"
	             "Wall_Labyrinth_Pillar,Wall_Destructible,Bomb,Floor_Tile,"
	             "PowerUp_BombCapacity,"
	             "PowerUp_Aura,PowerUp_BlastRange,Enemy_Basic,PowerUp_Life "
	             "primitives=%zu "
	             "vertices=%zu indices=%zu\n",
	             prepared.inputs.size(), playerVertexCount, playerIndexCount);
	std::fprintf(
	    stderr, "GLB Bomb selection: primitives=%zu vertices=%zu indices=%zu\n",
	    bombPrimitiveCount, bombVertexCount, bombIndexCount);
	std::fprintf(stderr,
	             "GLB Player footprint: radius=%.3f scale=%.3f "
	             "world_half_extent=%.3f\n",
	             playerBounds.maximumHorizontalRadius, 0.25F * 0.8F,
	             playerBounds.maximumHorizontalRadius * 0.25F * 0.8F);
	std::fprintf(stderr,
	             "GLB Floor_Tile selection: primitives=%zu vertices=%zu "
	             "indices=%zu\n",
	             floorPrimitiveCount, floorVertexCount, floorIndexCount);
	std::fprintf(stderr,
	             "GLB PowerUp_BombCapacity selection: primitives=%zu "
	             "vertices=%zu indices=%zu bounds=(minimum_y=%.3f width=%.3f "
	             "depth=%.3f).\n",
	             powerUpPrimitiveCount, powerUpVertexCount, powerUpIndexCount,
	             powerUpBombCapacityBounds.minimumY,
	             powerUpBombCapacityBounds.maximumX -
	                 powerUpBombCapacityBounds.minimumX,
	             powerUpBombCapacityBounds.maximumZ -
	                 powerUpBombCapacityBounds.minimumZ);
	std::fprintf(stderr,
	             "GLB PowerUp_Aura selection: primitives=%zu vertices=%zu "
	             "indices=%zu bounds=(minimum_y=%.3f width=%.3f depth=%.3f).\n",
	             powerUpAuraPrimitiveCount, powerUpAuraVertexCount,
	             powerUpAuraIndexCount, powerUpAuraBounds.minimumY,
	             powerUpAuraBounds.maximumX - powerUpAuraBounds.minimumX,
	             powerUpAuraBounds.maximumZ - powerUpAuraBounds.minimumZ);
	std::fprintf(
	    stderr,
	    "GLB PowerUp_BlastRange selection: primitives=%zu vertices=%zu "
	    "indices=%zu bounds=(minimum_y=%.3f width=%.3f depth=%.3f).\n",
	    powerUpBlastRangePrimitiveCount, powerUpBlastRangeVertexCount,
	    powerUpBlastRangeIndexCount, powerUpBlastRangeBounds.minimumY,
	    powerUpBlastRangeBounds.maximumX - powerUpBlastRangeBounds.minimumX,
	    powerUpBlastRangeBounds.maximumZ - powerUpBlastRangeBounds.minimumZ);
	std::fprintf(stderr,
	             "GLB PowerUp_Life selection: primitives=%zu vertices=%zu "
	             "indices=%zu bounds=(minimum_y=%.3f width=%.3f depth=%.3f).\n",
	             powerUpLifePrimitiveCount, powerUpLifeVertexCount,
	             powerUpLifeIndexCount, powerUpLifeBounds.minimumY,
	             powerUpLifeBounds.maximumX - powerUpLifeBounds.minimumX,
	             powerUpLifeBounds.maximumZ - powerUpLifeBounds.minimumZ);
	const auto reportBounds = [](const char *name, const MeshBounds &bounds)
	{
		std::fprintf(stderr,
		             "GLB bounds: model=%s min=(%.3f, %.3f, %.3f) "
		             "max=(%.3f, %.3f, %.3f) center=(%.3f, %.3f, %.3f).\n",
		             name, bounds.minimumX, bounds.minimumY, bounds.minimumZ,
		             bounds.maximumX, bounds.maximumY, bounds.maximumZ,
		             (bounds.minimumX + bounds.maximumX) * 0.5F,
		             (bounds.minimumY + bounds.maximumY) * 0.5F,
		             (bounds.minimumZ + bounds.maximumZ) * 0.5F);
	};
	reportBounds("Player", playerBounds);
	reportBounds("Wall_Indestructible", indestructibleWallBounds);
	reportBounds("Wall_Labyrinth_Pillar", labyrinthPillarBounds);
	reportBounds("Wall_Destructible", destructibleWallBounds);
	reportBounds("Bomb", bombBounds);
	reportBounds("Floor_Tile", floorBounds);
	reportBounds("PowerUp_BombCapacity", powerUpBombCapacityBounds);
	reportBounds("PowerUp_Aura", powerUpAuraBounds);
	reportBounds("PowerUp_BlastRange", powerUpBlastRangeBounds);
	reportBounds("PowerUp_Life", powerUpLifeBounds);
	reportBounds("Enemy_Basic", enemyBasicBounds);
	prepared.bounds = {playerBounds,
	                   indestructibleWallBounds,
	                   labyrinthPillarBounds,
	                   destructibleWallBounds,
	                   bombBounds,
	                   floorBounds,
	                   powerUpBombCapacityBounds,
	                   powerUpAuraBounds,
	                   powerUpBlastRangeBounds,
	                   powerUpLifeBounds,
	                   enemyBasicBounds};
	return true;
}
