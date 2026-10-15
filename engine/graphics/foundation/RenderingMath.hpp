/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingMath.hpp                                 :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

namespace engine::graphics::detail
{
// Matrices use OpenGL column-major storage and multiply column vectors.
struct Matrix4
{
	float values[16]{};
};

struct Matrix3
{
	float values[9]{};
};

struct Vector3
{
	float x = 0.0F;
	float y = 0.0F;
	float z = 0.0F;
};

struct Quaternion
{
	float x = 0.0F;
	float y = 0.0F;
	float z = 0.0F;
	float w = 1.0F;
};

[[nodiscard]] Matrix4 identityMatrix();
// With column vectors, left * right applies right before left.
[[nodiscard]] Matrix4 multiplyMatrices(const Matrix4 &left,
                                       const Matrix4 &right);

[[nodiscard]] float dotProduct(const Vector3 &left, const Vector3 &right);
[[nodiscard]] Vector3 crossProduct(const Vector3 &left, const Vector3 &right);
[[nodiscard]] Vector3 normalizeVector(const Vector3 &value);

[[nodiscard]] Quaternion normalizeQuaternion(const Quaternion &value);
[[nodiscard]] Quaternion multiplyQuaternions(const Quaternion &left,
                                             const Quaternion &right);
[[nodiscard]] Quaternion inverseUnitQuaternion(const Quaternion &value);
[[nodiscard]] Quaternion
interpolateQuaternionShortestArc(const Quaternion &from, const Quaternion &to,
                                 float factor);
} // namespace engine::graphics::detail
