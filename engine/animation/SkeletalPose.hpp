/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   SkeletalPose.hpp                                  :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+#+#+#+#+ +#+#+#+#+#+   +#+        */
/*   Created: 2026/10/01          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/10/01          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace engine::assets
{
struct GlbDocument;
struct GlbSkin;
} // namespace engine::assets

namespace engine::animation
{
// Column-major matrix suitable for glTF's local and inverse bind transforms.
struct JointMatrix
{
	std::array<float, 16> elements{};
};

struct SkeletalNodeSource
{
	std::optional<std::size_t> parentNodeIndex;
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

enum class SkeletalAnimationPath
{
	Translation,
	Rotation,
};

struct SkeletalAnimationChannel
{
	std::size_t nodeIndex = 0;
	SkeletalAnimationPath path = SkeletalAnimationPath::Translation;
	std::vector<float> times;
	std::vector<std::array<float, 4>> values;
};

struct SkeletalPoseClip
{
	std::string name;
	std::vector<SkeletalAnimationChannel> channels;
};

// This source owns every input required by skeletal pose evaluation.
struct SkeletalPoseSource
{
	std::vector<SkeletalNodeSource> nodes;
	std::vector<std::size_t> jointNodeIndices;
	std::vector<JointMatrix> inverseBindMatrices;
	std::vector<SkeletalPoseClip> clips;
};

struct SkeletalPoseSourceResult
{
	std::optional<SkeletalPoseSource> source;
	std::string error;

	[[nodiscard]] explicit operator bool() const;
};

struct SkeletalPoseResult
{
	std::vector<JointMatrix> jointPalette;
	std::string error;

	[[nodiscard]] explicit operator bool() const;
};

// Copies GLB data at the integration boundary; the resulting source is
// independent.
[[nodiscard]] SkeletalPoseSourceResult
buildSkeletalPoseSource(const engine::assets::GlbDocument &document,
                        const engine::assets::GlbSkin &skin);

// clip is optional: a null pointer evaluates only the skin's rest pose.
[[nodiscard]] SkeletalPoseResult
evaluateSkeletalPose(const SkeletalPoseSource &source,
                     const SkeletalPoseClip *clip, float timeSeconds);

// Samples both clips, blends their local poses, then composes the final
// palette.
[[nodiscard]] SkeletalPoseResult evaluateBlendedSkeletalPose(
    const SkeletalPoseSource &source, const SkeletalPoseClip &sourceClip,
    float sourceTimeSeconds, const SkeletalPoseClip &targetClip,
    float targetTimeSeconds, float blendFactor);

[[nodiscard]] const SkeletalPoseClip *
findSkeletalPoseClip(const SkeletalPoseSource &source, const char *name);
} // namespace engine::animation
