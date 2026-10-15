/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   BootstrapPaths.cpp                               :+:      :+:    :+: :+:      */
/*                                                    +:+ +:+   +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+ +#+#+#+#+#+   +#+      */
/*                                                +#+#+#+#+#+   +#+           +#+#+#+#+#+   */
/*   Created: 2026/10/08          by flaviobc      #+#    #+#             */
/*                                                                                    */
/* ******************************************************************************** */

#include "BootstrapPaths.hpp"

#include <cstdlib>

namespace
{
std::string environmentPath(const char *name, const char *defaultPath)
{
	const char *configuredPath = std::getenv(name);
	return configuredPath ? configuredPath : defaultPath;
}
} // namespace

bootstrap::BootstrapPaths bootstrap::resolveBootstrapPaths()
{
	return {environmentPath("BOMBERMAN_GLB_PATH", BOMBERMAN_DEFAULT_GLB_PATH),
	        "assets/ui/hud_icon_atlas.png",
	        "assets/ui/hud_font_atlas.png",
	        "assets/textures/explosion_smoke.png",
	        "assets/textures/explosion_fireball.png",
	        environmentPath("BOMBERMAN_GAMEPLAY_TUNING_PATH",
	                        BOMBERMAN_DEFAULT_GAMEPLAY_TUNING_PATH),
	        {environmentPath("BOMBERMAN_AUDIO_CLIP_0_PATH",
	                         BOMBERMAN_DEFAULT_AUDIO_CLIP_0_PATH),
	         environmentPath("BOMBERMAN_AUDIO_CLIP_1_PATH",
	                         BOMBERMAN_DEFAULT_AUDIO_CLIP_1_PATH),
	         environmentPath("BOMBERMAN_AUDIO_CLIP_2_PATH",
	                         BOMBERMAN_DEFAULT_AUDIO_CLIP_2_PATH),
	         environmentPath("BOMBERMAN_AUDIO_CLIP_3_PATH",
	                         BOMBERMAN_DEFAULT_AUDIO_CLIP_3_PATH),
	         environmentPath("BOMBERMAN_AUDIO_CLIP_4_PATH",
	                         BOMBERMAN_DEFAULT_AUDIO_CLIP_4_PATH),
	         environmentPath("BOMBERMAN_AUDIO_CLIP_5_PATH",
	                         BOMBERMAN_DEFAULT_AUDIO_CLIP_5_PATH)}};
}
