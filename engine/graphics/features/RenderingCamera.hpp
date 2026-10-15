/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingCamera.hpp                               :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

#include "graphics/foundation/RenderingMath.hpp"

#include <optional>

namespace engine::graphics::detail
{
class RenderingCamera
{
  public:
	struct Matrices
	{
		// World-to-view and view-to-clip compose as projection * view.
		Matrix4 projection;
		Matrix4 view;
		Matrix4 worldViewProjection;
	};

	void replaceImpulse(float x, float y, float z, float amplitude,
	                    double durationSeconds, float frequencyHz);
	bool setSceneBounds(float minimumX, float maximumX, float minimumZ,
	                    float maximumZ);
	bool setFraming(bool autoFrame, float coverage);
	bool setTrackingTarget(float x, float y, float z, bool reset);
	[[nodiscard]] Matrices update(int viewportWidth, int viewportHeight,
	                              double deltaSeconds);

  private:
	struct ActiveImpulse
	{
		Vector3 position;
		float amplitude = 0.0F;
		double durationSeconds = 0.0;
		float frequencyHz = 0.0F;
		double elapsedSeconds = 0.0;
	};

	Vector3 clampTrackingTarget(Vector3 target) const;
	// The base camera is stable; an active impulse adds a temporary offset.
	Vector3 basePosition_{0.0F, 4.9F, 2.75F};
	Vector3 sceneCenter_{0.0F, 0.0F, 0.0F};
	Vector3 desiredTrackingTarget_{0.0F, 0.0F, 0.0F};
	Vector3 trackedTarget_{0.0F, 0.0F, 0.0F};
	float minimumX_ = -1.875F;
	float maximumX_ = 1.875F;
	float minimumZ_ = -1.875F;
	float maximumZ_ = 1.875F;
	float sceneWidth_ = 3.75F;
	float sceneDepth_ = 3.75F;
	bool autoFrame_ = true;
	float coverage_ = 1.0F;
	bool trackingTargetInitialized_ = true;
	float trackingResponsePerSecond_ = 7.5F;
	float verticalFieldOfViewRadians_ = 0.68F;
	float nearPlane_ = 0.1F;
	float farPlane_ = 100.0F;
	std::optional<ActiveImpulse> activeImpulse_;
};
} // namespace engine::graphics::detail
