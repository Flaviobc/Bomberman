/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingCamera.cpp                               :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "graphics/features/RenderingCamera.hpp"

#include "graphics/foundation/RenderingTransforms.hpp"

#include <algorithm>
#include <cmath>

namespace engine::graphics::detail
{
bool RenderingCamera::setSceneBounds(float minimumX, float maximumX,
                                     float minimumZ, float maximumZ)
{
	if (!std::isfinite(minimumX) || !std::isfinite(maximumX) ||
	    !std::isfinite(minimumZ) || !std::isfinite(maximumZ) ||
	    minimumX >= maximumX || minimumZ >= maximumZ)
	{
		return false;
	}
	const bool changed = minimumX_ != minimumX || maximumX_ != maximumX ||
	                     minimumZ_ != minimumZ || maximumZ_ != maximumZ;
	minimumX_ = minimumX;
	maximumX_ = maximumX;
	minimumZ_ = minimumZ;
	maximumZ_ = maximumZ;
	sceneCenter_ = {(minimumX + maximumX) * 0.5F, 0.0F,
	                (minimumZ + maximumZ) * 0.5F};
	sceneWidth_ = maximumX - minimumX;
	sceneDepth_ = maximumZ - minimumZ;
	if (changed)
	{
		desiredTrackingTarget_ = sceneCenter_;
		trackedTarget_ = sceneCenter_;
		trackingTargetInitialized_ = true;
	}
	return true;
}

bool RenderingCamera::setFraming(bool autoFrame, float coverage)
{
	if (!std::isfinite(coverage) || coverage < 0.6F || coverage > 1.0F)
		return false;
	autoFrame_ = autoFrame;
	coverage_ = coverage;
	return true;
}

bool RenderingCamera::setTrackingTarget(float x, float y, float z, bool reset)
{
	if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
		return false;
	const Vector3 target = clampTrackingTarget({x, y, z});
	if (reset || !trackingTargetInitialized_)
	{
		desiredTrackingTarget_ = target;
		trackedTarget_ = target;
		trackingTargetInitialized_ = true;
		return true;
	}
	desiredTrackingTarget_ = target;
	return true;
}

Vector3 RenderingCamera::clampTrackingTarget(Vector3 target) const
{
	if (!autoFrame_ || coverage_ >= 1.0F)
		return sceneCenter_;
	const float visibleHalfWidth = sceneWidth_ * coverage_ * 0.5F;
	const float visibleHalfDepth = sceneDepth_ * coverage_ * 0.5F;
	target.x = std::clamp(target.x, minimumX_ + visibleHalfWidth,
	                      maximumX_ - visibleHalfWidth);
	target.z = std::clamp(target.z, minimumZ_ + visibleHalfDepth,
	                      maximumZ_ - visibleHalfDepth);
	return target;
}

void RenderingCamera::replaceImpulse(float x, float y, float z, float amplitude,
                                     double durationSeconds, float frequencyHz)
{
	activeImpulse_ = {{x, y, z}, amplitude, durationSeconds, frequencyHz, 0.0};
}

RenderingCamera::Matrices RenderingCamera::update(int viewportWidth,
                                                  int viewportHeight,
                                                  double deltaSeconds)
{
	const Matrix4 projection = makePerspectiveProjection(
	    verticalFieldOfViewRadians_,
	    static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight),
	    nearPlane_, farPlane_);
	const float aspect =
	    static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
	constexpr float referenceAspect = 16.0F / 9.0F;
	constexpr float referenceExtent = 3.75F;
	const float horizontalScale = sceneWidth_ / referenceExtent *
	                              std::max(referenceAspect / aspect, 1.0F);
	const float completeSceneScale =
	    std::max({1.0F, horizontalScale, sceneDepth_ / referenceExtent});
	const float framingScale =
	    completeSceneScale * (autoFrame_ ? coverage_ : 1.0F);
	const Vector3 desiredTarget =
	    autoFrame_ && coverage_ < 1.0F ? desiredTrackingTarget_ : sceneCenter_;
	if (!trackingTargetInitialized_)
	{
		trackedTarget_ = desiredTarget;
		trackingTargetInitialized_ = true;
	}
	if (autoFrame_ && coverage_ < 1.0F)
	{
		const double nonNegativeDelta = std::max(deltaSeconds, 0.0);
		const float blend = static_cast<float>(
		    1.0 - std::exp(-trackingResponsePerSecond_ * nonNegativeDelta));
		trackedTarget_.x += (desiredTarget.x - trackedTarget_.x) * blend;
		trackedTarget_.y += (desiredTarget.y - trackedTarget_.y) * blend;
		trackedTarget_.z += (desiredTarget.z - trackedTarget_.z) * blend;
	}
	else
	{
		trackedTarget_ = sceneCenter_;
	}
	Vector3 cameraTarget =
	    autoFrame_ && coverage_ < 1.0F ? trackedTarget_ : sceneCenter_;
	Vector3 cameraPosition{cameraTarget.x + (basePosition_.x * framingScale),
	                       cameraTarget.y + (basePosition_.y * framingScale),
	                       cameraTarget.z + (basePosition_.z * framingScale)};

	if (activeImpulse_)
	{
		ActiveImpulse &impulse = *activeImpulse_;
		// Time is in seconds. A zero delta freezes the phase and decay.
		impulse.elapsedSeconds += std::max(deltaSeconds, 0.0);
		if (impulse.elapsedSeconds >= impulse.durationSeconds)
		{
			activeImpulse_.reset();
		}
		else
		{
			const float decay = static_cast<float>(std::clamp(
			    1.0 - impulse.elapsedSeconds / impulse.durationSeconds, 0.0,
			    1.0));
			// Frequency is cycles per second (Hz); phase is expressed in
			// radians.
			const float phase = static_cast<float>(
			    impulse.elapsedSeconds * impulse.frequencyHz * 6.283185F);
			Vector3 impactDirection{impulse.position.x, 0.0F,
			                        impulse.position.z};
			if (dotProduct(impactDirection, impactDirection) <= 0.000001F)
			{
				impactDirection = {1.0F, 0.0F, 0.0F};
			}
			else
			{
				impactDirection = normalizeVector(impactDirection);
			}
			const Vector3 lateralDirection{-impactDirection.z, 0.0F,
			                               impactDirection.x};
			const float lateralOffset =
			    std::sin(phase) * impulse.amplitude * decay;
			const float impactHeightBias =
			    std::clamp(impulse.position.y, -1.0F, 1.0F) * 0.15F;
			const float verticalOffset =
			    (std::cos(phase * 1.37F) * 0.35F + impactHeightBias) *
			    impulse.amplitude * decay;
			const float targetOffset =
			    std::sin(phase * 0.73F) * impulse.amplitude * 0.3F * decay;
			cameraPosition.x += lateralDirection.x * lateralOffset;
			cameraPosition.y += verticalOffset;
			cameraPosition.z += lateralDirection.z * lateralOffset;
			cameraTarget.x += impactDirection.x * targetOffset;
			cameraTarget.z += impactDirection.z * targetOffset;
		}
	}

	const Matrix4 view =
	    makeLookAtViewMatrix(cameraPosition, cameraTarget, {0.0F, 1.0F, 0.0F});
	return {projection, view, multiplyMatrices(projection, view)};
}
} // namespace engine::graphics::detail
