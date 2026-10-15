/* **********************************************************************************
 */
/*                                                                                    */
/*                                                      :::::::::: :::::::::
 * :::::: */
/*   RenderingPrograms.hpp                             :+:        :+:    :+: :+:
 * : */
/*                                                    +:+        +:+    +:+ +:+
 */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+
 */
/*                                                  +#+        +#+    +#+ +#+ */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+# #+#
 */
/*   Updated: 2026/10/08          by flaviobc     ###        ######### ########
 */
/*                                                                                    */
/* **********************************************************************************
 */

#pragma once

#include <SDL3/SDL_opengl.h>

namespace engine::graphics::detail
{
class OpenGlFunctions;

struct MainProgramUniforms
{
	GLint modelViewProjection = -1;
	GLint worldViewProjection = -1;
	GLint lightSpaceMatrix = -1;
	GLint shadowMap = -1;
	GLint normalMatrix = -1;
	GLint modelMatrix = -1;
	GLint pointLightActive = -1;
	GLint pointLightPositionIntensity = -1;
	GLint pointLightColorRadius = -1;
	GLint usePrimitiveColor = -1;
	GLint primitiveColor = -1;
	GLint instanceColorization = -1;
	GLint emissiveColor = -1;
	GLint useBaseColorTexture = -1;
	GLint baseColorTexture = -1;
	GLint useSkinning = -1;
	GLint useInstancing = -1;
	GLint jointPalette = -1;
};

struct ShadowProgramUniforms
{
	GLint modelViewProjection = -1;
	GLint lightSpaceMatrix = -1;
	GLint useSkinning = -1;
	GLint useInstancing = -1;
	GLint jointPalette = -1;
};

struct BlurProgramUniforms
{
	GLint image = -1;
	GLint horizontal = -1;
};

struct CompositeProgramUniforms
{
	GLint scene = -1;
	GLint bloom = -1;
};

struct SpriteProgramUniforms
{
	GLint image = -1;
	GLint worldViewProjection = -1;
	GLint bloomIntensity = -1;
	GLint colorMultiplier = -1;
};

struct HudProgramUniforms
{
	GLint atlas = -1;
	GLint transform = -1;
	GLint useAtlas = -1;
	GLint tint = -1;
};

template <typename Uniforms> struct RenderingProgram
{
	GLuint handle = 0;
	Uniforms uniforms;
};

class RenderingPrograms
{
  public:
	using Main = RenderingProgram<MainProgramUniforms>;
	using Shadow = RenderingProgram<ShadowProgramUniforms>;
	using Blur = RenderingProgram<BlurProgramUniforms>;
	using Composite = RenderingProgram<CompositeProgramUniforms>;
	using Sprite = RenderingProgram<SpriteProgramUniforms>;
	using Hud = RenderingProgram<HudProgramUniforms>;

	[[nodiscard]] bool create(OpenGlFunctions &gl);
	void destroy(OpenGlFunctions &gl);

	[[nodiscard]] const Main &main() const;
	[[nodiscard]] const Shadow &shadow() const;
	[[nodiscard]] const Blur &blur() const;
	[[nodiscard]] const Composite &composite() const;
	[[nodiscard]] const Hud &hud() const;
	[[nodiscard]] const Sprite &sprite() const;

  private:
	Main main_;
	Shadow shadow_;
	Blur blur_;
	Composite composite_;
	Hud hud_;
	Sprite sprite_;
};
} // namespace engine::graphics::detail
