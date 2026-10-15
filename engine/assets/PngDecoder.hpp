/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   PngDecoder.hpp                                    :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/09/28          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/09/28          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace engine::assets
{
// Decoded pixels remain CPU data. Rendering owns any future GPU upload.
struct DecodedImageRgba8
{
	std::size_t width = 0;
	std::size_t height = 0;
	std::size_t channels = 4;
	std::vector<std::uint8_t> pixels;
};

[[nodiscard]] bool decodePngRgba8(const std::vector<std::uint8_t> &encodedBytes,
                                  DecodedImageRgba8 &image, std::string &error);
} // namespace engine::assets
