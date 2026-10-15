/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingHudPass.hpp                              :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/09          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/09          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

namespace engine::graphics::detail
{
struct HudDrawPlan;
class OpenGlFunctions;
class RenderingHudResources;
class RenderingPrograms;
class RenderingTransientLines;

struct HudTitleFrameInput
{
	int viewportWidth = 0;
	int viewportHeight = 0;
};

// Submits a CPU-prepared HUD plan. The Title entry also prepares its exclusive
// default-framebuffer frame; neither entry owns resources or presents a window.
class RenderingHudPass
{
  public:
	static void executeTitleFrame(OpenGlFunctions &gl,
	                              const RenderingPrograms &programs,
	                              const RenderingHudResources &resources,
	                              RenderingTransientLines &transientLines,
	                              const HudDrawPlan &plan,
	                              const HudTitleFrameInput &input);
	static void executeOverlay(OpenGlFunctions &gl,
	                           const RenderingPrograms &programs,
	                           const RenderingHudResources &resources,
	                           RenderingTransientLines &transientLines,
	                           const HudDrawPlan &plan);
};
} // namespace engine::graphics::detail
