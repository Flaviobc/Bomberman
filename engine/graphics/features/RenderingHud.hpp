/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingHud.hpp                                  :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

#include "graphics/foundation/RenderingMath.hpp"

#include "graphics/Renderer.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace engine::graphics::detail
{
constexpr std::size_t hudLineVertexCapacity = 256;

struct HudLineGeometry
{
	std::array<float, hudLineVertexCapacity * 6> values{};
	std::size_t vertexCount = 0;
};

struct HudAtlasDrawCommand
{
	Matrix4 transform;
	std::array<float, 4> tint{};
	int firstVertex = 0;
	int vertexCount = 0;
	bool useAtlas = false;
};

struct HudDrawPlan
{
	HudLineGeometry lines;
	Matrix4 lineTransform;
	std::array<HudAtlasDrawCommand, 3> atlasDraws{};
	std::size_t atlasDrawCount = 0;
	bool gameplayHudVisible = false;
};

enum class HudTextUpdateError
{
	None,
	NonPositiveGlyphSize,
	UnsupportedGlyph,
};

struct HudTextUpdateResult
{
	HudTextUpdateError error = HudTextUpdateError::None;
	char unsupportedGlyph = '\0';
};

// Owns HUD state and produces CPU geometry/draw intent only. OpenGL resources
// and submission remain in Renderer.
class RenderingHud
{
  public:
	void setState(const HudState &state);
	void setTerminalState(TerminalHudState state);
	[[nodiscard]] TerminalHudState terminalState() const;

	[[nodiscard]] HudTextUpdateResult
	setTexts(const std::vector<HudText> &texts);
	[[nodiscard]] const std::vector<HudText> &texts() const;
	[[nodiscard]] const std::vector<float> &glyphVertices() const;

	[[nodiscard]] HudDrawPlan buildDrawPlan() const;
	[[nodiscard]] static const std::array<float, 144> &iconAtlasVertices();

  private:
	HudState state_;
	TerminalHudState terminalState_ = TerminalHudState::None;
	std::vector<HudText> texts_;
	std::vector<float> glyphVertices_;
};
} // namespace engine::graphics::detail
