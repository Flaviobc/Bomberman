/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   PngDecoder.cpp                                    :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/09/28          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/09/28          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "PngDecoder.hpp"

#include <climits>
#include <limits>
#include <utility>

#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#include <stb_image.h>
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

namespace engine::assets
{
bool decodePngRgba8(const std::vector<std::uint8_t> &encodedBytes,
                    DecodedImageRgba8 &image, std::string &error)
{
	if (encodedBytes.empty())
	{
		error = "PNG has no encoded bytes";
		return false;
	}
	if (encodedBytes.size() > static_cast<std::size_t>(INT_MAX))
	{
		error = "PNG encoded byte count exceeds stb_image input limit";
		return false;
	}

	int width = 0;
	int height = 0;
	int sourceChannels = 0;
	// Requesting four channels makes the owned representation consistently
	// RGBA8.
	stbi_uc *decoded = stbi_load_from_memory(
	    encodedBytes.data(), static_cast<int>(encodedBytes.size()), &width,
	    &height, &sourceChannels, STBI_rgb_alpha);
	if (!decoded)
	{
		const char *reason = stbi_failure_reason();
		error = reason ? reason : "stb_image could not decode PNG";
		return false;
	}

	const std::size_t decodedWidth = static_cast<std::size_t>(width);
	const std::size_t decodedHeight = static_cast<std::size_t>(height);
	constexpr std::size_t channels = 4;
	if (width <= 0 || height <= 0 ||
	    decodedWidth >
	        std::numeric_limits<std::size_t>::max() / decodedHeight / channels)
	{
		stbi_image_free(decoded);
		error = "PNG dimensions overflow the RGBA8 byte count";
		return false;
	}

	const std::size_t byteCount = decodedWidth * decodedHeight * channels;
	DecodedImageRgba8 decodedImage;
	decodedImage.width = decodedWidth;
	decodedImage.height = decodedHeight;
	decodedImage.pixels.assign(decoded, decoded + byteCount);
	stbi_image_free(decoded);
	image = std::move(decodedImage);
	return true;
}
} // namespace engine::assets
