/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   BootstrapPaths.hpp                               :+:      :+:    :+: :+:      */
/*                                                    +:+ +:+   +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+ +#+#+#+#+#+   +#+      */
/*                                                +#+#+#+#+#+   +#+           +#+#+#+#+#+   */
/*   Created: 2026/10/08          by flaviobc      #+#    #+#             */
/*                                                                                    */
/* ******************************************************************************** */

#pragma once

#include <string>
#include <vector>

namespace bootstrap
{
struct BootstrapPaths
{
	std::string glb;
	std::string hudIconAtlas;
	std::string hudFontAtlas;
	std::string explosionSmokeImage;
	std::string explosionFireballImage;
	std::string gameplayTuning;
	std::vector<std::string> audioClips;
};

BootstrapPaths resolveBootstrapPaths();
} // namespace bootstrap
