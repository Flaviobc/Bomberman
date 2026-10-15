/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingTransforms.hpp                           :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

#include "graphics/foundation/RenderingMath.hpp"

namespace engine::graphics::detail
{
[[nodiscard]] Matrix4 makeTranslationMatrix(float x, float y, float z);
[[nodiscard]] Matrix4 makeUniformScaleMatrix(float scale);
// Rotation angles are expressed in radians throughout Rendering.
[[nodiscard]] Matrix4 makeYRotationMatrix(float rotationRadians);
[[nodiscard]] Matrix4
makeQuaternionRotationMatrix(const Quaternion &quaternion);

[[nodiscard]] Matrix4 makeWorldModelMatrix(float x, float y, float z,
                                           float rotationRadians,
                                           float uniformScale);
[[nodiscard]] Matrix3 makeNormalMatrix(float rotationRadians);
[[nodiscard]] Matrix3 makeNormalMatrix(const Matrix4 &rotationMatrix);

// Projection matrices transform view space into OpenGL clip space.
[[nodiscard]] Matrix4
makePerspectiveProjection(float verticalFieldOfViewRadians, float aspectRatio,
                          float nearPlane, float farPlane);
[[nodiscard]] Matrix4 makeOrthographicProjection(float left, float right,
                                                 float bottom, float top,
                                                 float nearPlane,
                                                 float farPlane);
// The returned view matrix transforms world-space positions into view space.
[[nodiscard]] Matrix4 makeLookAtViewMatrix(const Vector3 &eye,
                                           const Vector3 &target,
                                           const Vector3 &up);
} // namespace engine::graphics::detail
