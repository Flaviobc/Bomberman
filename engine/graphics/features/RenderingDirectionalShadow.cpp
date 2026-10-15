#include "graphics/features/RenderingDirectionalShadow.hpp"

#include "graphics/foundation/RenderingTransforms.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace engine::graphics::detail
{
namespace
{
constexpr float kMinimumWorldMargin = 0.125F;
constexpr float kProportionalWorldMargin = 0.05F;
constexpr float kCasterMinimumY = -0.125F;
constexpr float kCasterMaximumY = 0.75F;
constexpr float kDepthMargin = 0.25F;
constexpr float kMinimumLightDistance = 4.0F;
constexpr float kEyeClearance = 1.0F;

Vector3 transformPoint(const Matrix4 &matrix, const Vector3 &point)
{
	return {matrix.values[0] * point.x + matrix.values[4] * point.y +
	            matrix.values[8] * point.z + matrix.values[12],
	        matrix.values[1] * point.x + matrix.values[5] * point.y +
	            matrix.values[9] * point.z + matrix.values[13],
	        matrix.values[2] * point.x + matrix.values[6] * point.y +
	            matrix.values[10] * point.z + matrix.values[14]};
}
} // namespace

std::optional<DirectionalShadowFrustum>
makeDirectionalShadowFrustum(float minimumX, float maximumX, float minimumZ,
                             float maximumZ, const Vector3 &directionalLight)
{
	if (!std::isfinite(minimumX) || !std::isfinite(maximumX) ||
	    !std::isfinite(minimumZ) || !std::isfinite(maximumZ) ||
	    minimumX >= maximumX || minimumZ >= maximumZ ||
	    dotProduct(directionalLight, directionalLight) <= 0.000001F)
	{
		return std::nullopt;
	}
	const Vector3 direction = normalizeVector(directionalLight);
	if (!std::isfinite(direction.x) || !std::isfinite(direction.y) ||
	    !std::isfinite(direction.z))
	{
		return std::nullopt;
	}
	const float sceneWidth = maximumX - minimumX;
	const float sceneDepth = maximumZ - minimumZ;
	const float worldMargin =
	    std::max(kMinimumWorldMargin,
		         std::max(sceneWidth, sceneDepth) * kProportionalWorldMargin);
	const float expandedMinimumX = minimumX - worldMargin;
	const float expandedMaximumX = maximumX + worldMargin;
	const float expandedMinimumZ = minimumZ - worldMargin;
	const float expandedMaximumZ = maximumZ + worldMargin;
	const Vector3 target{(minimumX + maximumX) * 0.5F, 0.0F,
	                     (minimumZ + maximumZ) * 0.5F};
	const std::array<Vector3, 8> corners{{
	    {expandedMinimumX, kCasterMinimumY, expandedMinimumZ},
	    {expandedMinimumX, kCasterMinimumY, expandedMaximumZ},
	    {expandedMaximumX, kCasterMinimumY, expandedMinimumZ},
	    {expandedMaximumX, kCasterMinimumY, expandedMaximumZ},
	    {expandedMinimumX, kCasterMaximumY, expandedMinimumZ},
	    {expandedMinimumX, kCasterMaximumY, expandedMaximumZ},
	    {expandedMaximumX, kCasterMaximumY, expandedMinimumZ},
	    {expandedMaximumX, kCasterMaximumY, expandedMaximumZ},
	}};
	float maximumForwardExtent = 0.0F;
	for (const Vector3 &corner : corners)
	{
		maximumForwardExtent = std::max(
		    maximumForwardExtent,
		    dotProduct(direction, {corner.x - target.x, corner.y - target.y,
			                       corner.z - target.z}));
	}
	const float lightDistance =
	    std::max(kMinimumLightDistance, maximumForwardExtent + kEyeClearance);
	const Vector3 eye{target.x + direction.x * lightDistance,
	                  target.y + direction.y * lightDistance,
	                  target.z + direction.z * lightDistance};
	const Matrix4 view = makeLookAtViewMatrix(eye, target, {0.0F, 1.0F, 0.0F});
	float left = std::numeric_limits<float>::max();
	float right = std::numeric_limits<float>::lowest();
	float bottom = std::numeric_limits<float>::max();
	float top = std::numeric_limits<float>::lowest();
	float minimumViewZ = std::numeric_limits<float>::max();
	float maximumViewZ = std::numeric_limits<float>::lowest();
	for (const Vector3 &corner : corners)
	{
		const Vector3 lightPoint = transformPoint(view, corner);
		left = std::min(left, lightPoint.x);
		right = std::max(right, lightPoint.x);
		bottom = std::min(bottom, lightPoint.y);
		top = std::max(top, lightPoint.y);
		minimumViewZ = std::min(minimumViewZ, lightPoint.z);
		maximumViewZ = std::max(maximumViewZ, lightPoint.z);
	}
	const float nearPlane = std::max(0.1F, -maximumViewZ - kDepthMargin);
	const float farPlane = -minimumViewZ + kDepthMargin;
	if (!(left < right && bottom < top && nearPlane < farPlane))
	{
		return std::nullopt;
	}
	const Matrix4 projection = makeOrthographicProjection(
	    left, right, bottom, top, nearPlane, farPlane);
	return DirectionalShadowFrustum{
	    view, projection, multiplyMatrices(projection, view),
	    left, right,      bottom,
	    top,  nearPlane,  farPlane};
}
} // namespace engine::graphics::detail
