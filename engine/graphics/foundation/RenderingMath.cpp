/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingMath.cpp                                 :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "graphics/foundation/RenderingMath.hpp"

#include <cmath>

namespace engine::graphics::detail
{
Matrix4 identityMatrix()
{
	Matrix4 result{};
	result.values[0] = 1.0F;
	result.values[5] = 1.0F;
	result.values[10] = 1.0F;
	result.values[15] = 1.0F;
	return result;
}

Matrix4 multiplyMatrices(const Matrix4 &left, const Matrix4 &right)
{
	Matrix4 result{};
	for (int column = 0; column < 4; ++column)
	{
		for (int row = 0; row < 4; ++row)
		{
			for (int sharedIndex = 0; sharedIndex < 4; ++sharedIndex)
			{
				result.values[column * 4 + row] +=
				    left.values[sharedIndex * 4 + row] *
				    right.values[column * 4 + sharedIndex];
			}
		}
	}
	return result;
}

float dotProduct(const Vector3 &left, const Vector3 &right)
{
	return left.x * right.x + left.y * right.y + left.z * right.z;
}

Vector3 crossProduct(const Vector3 &left, const Vector3 &right)
{
	return {
	    left.y * right.z - left.z * right.y,
	    left.z * right.x - left.x * right.z,
	    left.x * right.y - left.y * right.x,
	};
}

Vector3 normalizeVector(const Vector3 &value)
{
	const float length = std::sqrt(dotProduct(value, value));
	return {value.x / length, value.y / length, value.z / length};
}

Quaternion normalizeQuaternion(const Quaternion &value)
{
	const float length = std::sqrt(value.x * value.x + value.y * value.y +
	                               value.z * value.z + value.w * value.w);
	if (length <= 0.000001F)
	{
		return {};
	}
	return {value.x / length, value.y / length, value.z / length,
	        value.w / length};
}

Quaternion multiplyQuaternions(const Quaternion &left, const Quaternion &right)
{
	return {left.w * right.x + left.x * right.w + left.y * right.z -
	            left.z * right.y,
	        left.w * right.y - left.x * right.z + left.y * right.w +
	            left.z * right.x,
	        left.w * right.z + left.x * right.y - left.y * right.x +
	            left.z * right.w,
	        left.w * right.w - left.x * right.x - left.y * right.y -
	            left.z * right.z};
}

Quaternion inverseUnitQuaternion(const Quaternion &value)
{
	return {-value.x, -value.y, -value.z, value.w};
}

Quaternion interpolateQuaternionShortestArc(const Quaternion &from,
                                            const Quaternion &to, float factor)
{
	Quaternion destination = to;
	const float dotProduct =
	    from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;
	if (dotProduct < 0.0F)
	{
		destination = {-to.x, -to.y, -to.z, -to.w};
	}
	return normalizeQuaternion({from.x + (destination.x - from.x) * factor,
	                            from.y + (destination.y - from.y) * factor,
	                            from.z + (destination.z - from.z) * factor,
	                            from.w + (destination.w - from.w) * factor});
}
} // namespace engine::graphics::detail
