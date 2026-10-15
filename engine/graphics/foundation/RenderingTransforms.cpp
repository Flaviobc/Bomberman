/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingTransforms.cpp                           :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "graphics/foundation/RenderingTransforms.hpp"

#include <cmath>

namespace engine::graphics::detail
{
Matrix4 makeTranslationMatrix(float x, float y, float z)
{
	Matrix4 result = identityMatrix();
	result.values[12] = x;
	result.values[13] = y;
	result.values[14] = z;
	return result;
}

Matrix4 makeUniformScaleMatrix(float scale)
{
	Matrix4 result{};
	result.values[0] = scale;
	result.values[5] = scale;
	result.values[10] = scale;
	result.values[15] = 1.0F;
	return result;
}

Matrix4 makeYRotationMatrix(float rotationRadians)
{
	Matrix4 result = identityMatrix();
	const float cosine = std::cos(rotationRadians);
	const float sine = std::sin(rotationRadians);
	result.values[0] = cosine;
	result.values[2] = -sine;
	result.values[8] = sine;
	result.values[10] = cosine;
	return result;
}

Matrix4 makeQuaternionRotationMatrix(const Quaternion &input)
{
	const Quaternion value = normalizeQuaternion(input);
	const float xx = value.x * value.x;
	const float yy = value.y * value.y;
	const float zz = value.z * value.z;
	const float xy = value.x * value.y;
	const float xz = value.x * value.z;
	const float yz = value.y * value.z;
	const float xw = value.x * value.w;
	const float yw = value.y * value.w;
	const float zw = value.z * value.w;
	Matrix4 result = identityMatrix();
	result.values[0] = 1.0F - 2.0F * (yy + zz);
	result.values[1] = 2.0F * (xy + zw);
	result.values[2] = 2.0F * (xz - yw);
	result.values[4] = 2.0F * (xy - zw);
	result.values[5] = 1.0F - 2.0F * (xx + zz);
	result.values[6] = 2.0F * (yz + xw);
	result.values[8] = 2.0F * (xz + yw);
	result.values[9] = 2.0F * (yz - xw);
	result.values[10] = 1.0F - 2.0F * (xx + yy);
	return result;
}

Matrix4 makeWorldModelMatrix(float x, float y, float z, float rotationRadians,
                             float uniformScale)
{
	Matrix4 result{};
	const float cosine = std::cos(rotationRadians);
	const float sine = std::sin(rotationRadians);
	result.values[0] = cosine * uniformScale;
	result.values[2] = -sine * uniformScale;
	result.values[5] = uniformScale;
	result.values[8] = sine * uniformScale;
	result.values[10] = cosine * uniformScale;
	result.values[12] = x;
	result.values[13] = y;
	result.values[14] = z;
	result.values[15] = 1.0F;
	return result;
}

Matrix3 makeNormalMatrix(float rotationRadians)
{
	const float cosine = std::cos(rotationRadians);
	const float sine = std::sin(rotationRadians);
	return {{cosine, 0.0F, -sine, 0.0F, 1.0F, 0.0F, sine, 0.0F, cosine}};
}

Matrix3 makeNormalMatrix(const Matrix4 &rotationMatrix)
{
	return {{rotationMatrix.values[0], rotationMatrix.values[1],
	         rotationMatrix.values[2], rotationMatrix.values[4],
	         rotationMatrix.values[5], rotationMatrix.values[6],
	         rotationMatrix.values[8], rotationMatrix.values[9],
	         rotationMatrix.values[10]}};
}

Matrix4 makePerspectiveProjection(float verticalFieldOfViewRadians,
                                  float aspectRatio, float nearPlane,
                                  float farPlane)
{
	Matrix4 result{};
	const float inverseTangent =
	    1.0F / std::tan(verticalFieldOfViewRadians * 0.5F);
	result.values[0] = inverseTangent / aspectRatio;
	result.values[5] = inverseTangent;
	result.values[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
	result.values[11] = -1.0F;
	result.values[14] = (2.0F * farPlane * nearPlane) / (nearPlane - farPlane);
	return result;
}

Matrix4 makeOrthographicProjection(float left, float right, float bottom,
                                   float top, float nearPlane, float farPlane)
{
	Matrix4 result = identityMatrix();
	result.values[0] = 2.0F / (right - left);
	result.values[5] = 2.0F / (top - bottom);
	result.values[10] = -2.0F / (farPlane - nearPlane);
	result.values[12] = -(right + left) / (right - left);
	result.values[13] = -(top + bottom) / (top - bottom);
	result.values[14] = -(farPlane + nearPlane) / (farPlane - nearPlane);
	return result;
}

Matrix4 makeLookAtViewMatrix(const Vector3 &eye, const Vector3 &target,
                             const Vector3 &up)
{
	const Vector3 forward =
	    normalizeVector({target.x - eye.x, target.y - eye.y, target.z - eye.z});
	const Vector3 right = normalizeVector(crossProduct(forward, up));
	const Vector3 cameraUp = crossProduct(right, forward);
	Matrix4 result = identityMatrix();
	result.values[0] = right.x;
	result.values[1] = cameraUp.x;
	result.values[2] = -forward.x;
	result.values[4] = right.y;
	result.values[5] = cameraUp.y;
	result.values[6] = -forward.y;
	result.values[8] = right.z;
	result.values[9] = cameraUp.z;
	result.values[10] = -forward.z;
	result.values[12] = -dotProduct(right, eye);
	result.values[13] = -dotProduct(cameraUp, eye);
	result.values[14] = dotProduct(forward, eye);
	return result;
}
} // namespace engine::graphics::detail
