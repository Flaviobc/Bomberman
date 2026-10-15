/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   AssetPreparation.hpp                             :+:      :+:    :+: :+:      */
/*                                                    +:+ +:+   +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+ +#+#+#+#+#+   +#+      */
/*                                                +#+#+#+#+#+   +#+           +#+#+#+#+#+   */
/*   Created: 2026/10/08          by flaviobc      #+#    #+#             */
/*                                                                                    */
/* ******************************************************************************** */

#pragma once

#include "BootstrapPaths.hpp"

#include "assets/GlbLoader.hpp"
#include "assets/PngDecoder.hpp"

namespace bootstrap
{
// This owner must stay at a stable address while prepared meshes borrow data
// from the document and its decoded images.
struct OwnedAssets final
{
	OwnedAssets() = default;
	OwnedAssets(const OwnedAssets &) = delete;
	OwnedAssets &operator=(const OwnedAssets &) = delete;
	OwnedAssets(OwnedAssets &&) = delete;
	OwnedAssets &operator=(OwnedAssets &&) = delete;

	engine::assets::GlbDocument document;
	engine::assets::DecodedImageRgba8 hudIconAtlas;
	engine::assets::DecodedImageRgba8 hudFontAtlas;
	engine::assets::DecodedImageRgba8 explosionSmokeImage;
	engine::assets::DecodedImageRgba8 explosionFireballImage;
};

bool loadOwnedAssets(const BootstrapPaths &paths, OwnedAssets &assets);
} // namespace bootstrap
