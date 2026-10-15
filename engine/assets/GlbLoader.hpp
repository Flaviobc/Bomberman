/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   GlbLoader.hpp                                     :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/09/26          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/09/26          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

#include "PngDecoder.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace engine::assets
{
struct GlbBaseColor
{
	float red = 1.0F;
	float green = 1.0F;
	float blue = 1.0F;
	float alpha = 1.0F;
};

// glTF omits emissiveFactor for non-emissive materials; black is the default.
struct GlbEmissiveColor
{
	float red = 0.0F;
	float green = 0.0F;
	float blue = 0.0F;
};

struct GlbBaseColorTexture
{
	std::size_t textureIndex = 0;
	std::size_t texCoord = 0;
};

struct GlbMaterial
{
	std::string name;
	GlbBaseColor baseColor;
	GlbEmissiveColor emissiveColor;
	std::optional<GlbBaseColorTexture> baseColorTexture;
};

struct GlbImage
{
	std::string name;
	std::string mimeType;
	std::size_t bufferViewIndex = 0;
	std::vector<std::uint8_t> encodedBytes;
	DecodedImageRgba8 decodedImage;
};

struct GlbTexture
{
	std::size_t imageIndex = 0;
};

// CPU-side geometry only. Rendering may later copy these values to GPU
// resources.
struct StaticMeshPosition
{
	float x = 0.0F;
	float y = 0.0F;
	float z = 0.0F;
};

struct StaticMeshNormal
{
	float x = 0.0F;
	float y = 0.0F;
	float z = 0.0F;
};

struct StaticMeshTexCoord
{
	float u = 0.0F;
	float v = 0.0F;
};

// A joint index selects one joint from the skin for a vertex influence.
struct StaticMeshJointIndices
{
	std::uint16_t first = 0;
	std::uint16_t second = 0;
	std::uint16_t third = 0;
	std::uint16_t fourth = 0;
};

// Four weights describe how strongly a vertex follows its selected joints.
struct StaticMeshJointWeights
{
	float first = 0.0F;
	float second = 0.0F;
	float third = 0.0F;
	float fourth = 0.0F;
};

struct StaticMeshSource
{
	std::vector<StaticMeshPosition> positions;
	std::vector<StaticMeshNormal> normals;
	std::vector<StaticMeshTexCoord> texCoords0;
	std::vector<StaticMeshTexCoord> texCoords1;
	std::vector<StaticMeshJointIndices> jointIndices;
	std::vector<StaticMeshJointWeights> jointWeights;
	std::vector<std::uint32_t> indices;
};

struct GlbPrimitive
{
	std::optional<std::size_t> materialIndex;
	StaticMeshSource source;
};

struct GlbMesh
{
	std::string name;
	std::vector<GlbPrimitive> primitives;
};

// Missing glTF TRS fields use these local identity values.
struct GlbNodeTransform
{
	float translationX = 0.0F;
	float translationY = 0.0F;
	float translationZ = 0.0F;
	float rotationX = 0.0F;
	float rotationY = 0.0F;
	float rotationZ = 0.0F;
	float rotationW = 1.0F;
	float scaleX = 1.0F;
	float scaleY = 1.0F;
	float scaleZ = 1.0F;
};

struct GlbNode
{
	std::string name;
	std::optional<std::size_t> meshIndex;
	std::optional<std::size_t> skinIndex;
	std::optional<std::size_t> parentNodeIndex;
	std::vector<std::size_t> childNodeIndices;
	GlbNodeTransform localTransform;
};

// An inverse bind matrix converts a joint from mesh bind space to joint space.
struct GlbInverseBindMatrix
{
	std::array<float, 16> elements{};
};

struct GlbSkin
{
	std::string name;
	std::vector<std::size_t> jointNodeIndices;
	std::vector<GlbInverseBindMatrix> inverseBindMatrices;
};

struct GlbScene
{
	std::string name;
	std::vector<std::size_t> nodeIndices;
};

struct GlbAnimationTranslation
{
	float x = 0.0F;
	float y = 0.0F;
	float z = 0.0F;
};

struct GlbAnimationRotation
{
	float x = 0.0F;
	float y = 0.0F;
	float z = 0.0F;
	float w = 1.0F;
};

enum class GlbAnimationPath
{
	Translation,
	Rotation,
};

enum class GlbAnimationInterpolation
{
	Linear,
};

// A sampler owns one supported output type and its matching key times.
struct GlbAnimationSampler
{
	GlbAnimationPath path = GlbAnimationPath::Translation;
	GlbAnimationInterpolation interpolation = GlbAnimationInterpolation::Linear;
	std::vector<float> times;
	std::vector<GlbAnimationTranslation> translations;
	std::vector<GlbAnimationRotation> rotations;
};

struct GlbAnimationChannel
{
	std::size_t samplerIndex = 0;
	std::size_t nodeIndex = 0;
	GlbAnimationPath path = GlbAnimationPath::Translation;
};

struct GlbAnimationClip
{
	std::string name;
	float durationSeconds = 0.0F;
	std::vector<GlbAnimationSampler> samplers;
	std::vector<GlbAnimationChannel> channels;
};

struct GlbDocument
{
	std::vector<GlbScene> scenes;
	std::vector<GlbNode> nodes;
	std::vector<GlbMesh> meshes;
	std::vector<GlbMaterial> materials;
	std::vector<GlbTexture> textures;
	std::vector<GlbImage> images;
	std::vector<GlbAnimationClip> animations;
	std::vector<GlbSkin> skins;
	std::size_t activeSceneIndex = 0;
};

struct GlbLoadResult
{
	std::optional<GlbDocument> document;
	std::string error;

	[[nodiscard]] explicit operator bool() const;
};

[[nodiscard]] GlbLoadResult loadGlb(const std::filesystem::path &path);
} // namespace engine::assets
