/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   AssetPreparation.cpp                             :+:      :+:    :+: :+:      */
/*                                                    +:+ +:+   +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+ +#+#+#+#+#+   +#+      */
/*                                                +#+#+#+#+#+   +#+           +#+#+#+#+#+   */
/*   Created: 2026/10/08          by flaviobc      #+#    #+#             */
/*                                                                                    */
/* ******************************************************************************** */

#include "AssetPreparation.hpp"

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace
{
bool loadAtlas(const std::string &path, const char *label,
               std::size_t expectedWidth, std::size_t expectedHeight,
               engine::assets::DecodedImageRgba8 &image)
{
	std::ifstream input(path, std::ios::binary);
	if (!input)
	{
		std::fprintf(stderr, "HUD %s atlas could not be opened: %s\n", label,
		             path.c_str());
		return false;
	}
	const std::vector<std::uint8_t> encodedBytes{
	    std::istreambuf_iterator<char>(input),
	    std::istreambuf_iterator<char>()};
	std::string error;
	if (!engine::assets::decodePngRgba8(encodedBytes, image, error) ||
	    image.width != expectedWidth || image.height != expectedHeight ||
	    image.channels != 4)
	{
		std::fprintf(stderr, "HUD %s atlas is invalid: %s\n", label,
		             error.empty()
		                 ? (expectedWidth == 512 ? "expected 512x128 RGBA8 PNG"
						                         : "expected 256x48 RGBA8 PNG")
						 : error.c_str());
		return false;
	}
	std::fprintf(stderr,
	             "HUD %s atlas decoded: path=%s width=%zu height=%zu "
	             "channels=%zu rgba8_bytes=%zu.\n",
	             label, path.c_str(), image.width, image.height, image.channels,
	             image.pixels.size());
	return true;
}
bool loadVfxSprite(const std::string &path, const char *label,
                   engine::assets::DecodedImageRgba8 &image)
{
	std::ifstream input(path, std::ios::binary);
	if (!input)
	{
		std::fprintf(stderr, "VFX %s image could not be opened: %s\n", label,
		             path.c_str());
		return false;
	}
	const std::vector<std::uint8_t> encodedBytes{
	    std::istreambuf_iterator<char>(input),
	    std::istreambuf_iterator<char>()};
	std::string error;
	if (!engine::assets::decodePngRgba8(encodedBytes, image, error))
	{
		std::fprintf(stderr, "VFX %s PNG decode failed: path=%s error=%s\n",
		             label, path.c_str(), error.c_str());
		return false;
	}
	std::fprintf(stderr,
	             "VFX %s decoded: path=%s width=%zu height=%zu "
	             "rgba8_bytes=%zu.\n",
	             label, path.c_str(), image.width, image.height,
	             image.pixels.size());
	return true;
}
} // namespace

bool bootstrap::loadOwnedAssets(const BootstrapPaths &paths,
                                OwnedAssets &assets)
{
	engine::assets::GlbLoadResult result = engine::assets::loadGlb(paths.glb);
	if (!result)
	{
		std::fprintf(stderr, "GLB load failed: path=%s error=%s\n",
		             paths.glb.c_str(), result.error.c_str());
		return false;
	}
	assets.document = std::move(*result.document);
	return loadAtlas(paths.hudIconAtlas, "icon", 512, 128,
	                 assets.hudIconAtlas) &&
	       loadAtlas(paths.hudFontAtlas, "font", 256, 48,
	                 assets.hudFontAtlas) &&
	       loadVfxSprite(paths.explosionSmokeImage, "smoke",
	                     assets.explosionSmokeImage) &&
	       loadVfxSprite(paths.explosionFireballImage, "fireball",
	                     assets.explosionFireballImage);
}
