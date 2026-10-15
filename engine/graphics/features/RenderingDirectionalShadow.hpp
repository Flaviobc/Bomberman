#pragma once

#include "graphics/foundation/RenderingMath.hpp"

#include <optional>

namespace engine::graphics::detail
{
struct DirectionalShadowFrustum
{
	Matrix4 view;
	Matrix4 projection;
	Matrix4 lightSpace;
	float left = 0.0F;
	float right = 0.0F;
	float bottom = 0.0F;
	float top = 0.0F;
	float nearPlane = 0.0F;
	float farPlane = 0.0F;
};

// Fits the approved arena footprint and conservative caster height in the
// directional light's view space. Scene coordinates are world X/Z units.
[[nodiscard]] std::optional<DirectionalShadowFrustum>
makeDirectionalShadowFrustum(float minimumX, float maximumX, float minimumZ,
                             float maximumZ, const Vector3 &directionalLight);
} // namespace engine::graphics::detail
