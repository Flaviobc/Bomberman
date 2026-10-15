/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingHud.cpp                                  :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "graphics/features/RenderingHud.hpp"

#include <algorithm>
#include <iterator>
#include <utility>

namespace
{
using engine::graphics::detail::HudAtlasDrawCommand;
using engine::graphics::detail::HudLineGeometry;
using engine::graphics::detail::identityMatrix;
using engine::graphics::detail::Matrix4;

void addLine(HudLineGeometry &geometry, float x0, float y0, float z0, float x1,
             float y1, float z1, float red, float green, float blue)
{
	if (geometry.vertexCount + 2 >
	    engine::graphics::detail::hudLineVertexCapacity)
	{
		return;
	}
	const std::size_t offset = geometry.vertexCount * 6;
	geometry.values[offset] = x0;
	geometry.values[offset + 1] = y0;
	geometry.values[offset + 2] = z0;
	geometry.values[offset + 3] = red;
	geometry.values[offset + 4] = green;
	geometry.values[offset + 5] = blue;
	geometry.values[offset + 6] = x1;
	geometry.values[offset + 7] = y1;
	geometry.values[offset + 8] = z1;
	geometry.values[offset + 9] = red;
	geometry.values[offset + 10] = green;
	geometry.values[offset + 11] = blue;
	geometry.vertexCount += 2;
}

void appendDigit(HudLineGeometry &geometry, int digit, float x, float y,
                 float red, float green, float blue, float scale)
{
	static constexpr bool segments[10][7] = {
	    {true, true, true, true, true, true, false},
	    {false, true, true, false, false, false, false},
	    {true, true, false, true, true, false, true},
	    {true, true, true, true, false, false, true},
	    {false, true, true, false, false, true, true},
	    {true, false, true, true, false, true, true},
	    {true, false, true, true, true, true, true},
	    {true, true, true, false, false, false, false},
	    {true, true, true, true, true, true, true},
	    {true, true, true, true, false, true, true},
	};
	static constexpr float endpoints[7][4] = {
	    {0.0F, 0.08F, 0.04F, 0.08F}, {0.04F, 0.08F, 0.04F, 0.04F},
	    {0.04F, 0.04F, 0.04F, 0.0F}, {0.0F, 0.0F, 0.04F, 0.0F},
	    {0.0F, 0.04F, 0.0F, 0.0F},   {0.0F, 0.08F, 0.0F, 0.04F},
	    {0.0F, 0.04F, 0.04F, 0.04F},
	};
	for (int segment = 0; segment < 7; ++segment)
	{
		if (!segments[digit][segment])
		{
			continue;
		}
		const float *line = endpoints[segment];
		addLine(geometry, x + line[0] * scale, y + line[1] * scale, 0.0F,
		        x + line[2] * scale, y + line[3] * scale, 0.0F, red, green,
		        blue);
	}
}

void appendCounter(HudLineGeometry &geometry, int value, float x, float y,
                   float red, float green, float blue)
{
	const int clampedValue = std::clamp(value, 0, 99);
	constexpr float digitScale = 0.85F;
	if (clampedValue >= 10)
	{
		appendDigit(geometry, clampedValue / 10, x, y, red, green, blue,
		            digitScale);
		x += 0.042F;
	}
	appendDigit(geometry, clampedValue % 10, x, y, red, green, blue,
	            digitScale);
}

void appendRectangleOutline(HudLineGeometry &geometry, float left, float bottom,
                            float right, float top, float red, float green,
                            float blue)
{
	addLine(geometry, left, bottom, 0.0F, right, bottom, 0.0F, red, green,
	        blue);
	addLine(geometry, right, bottom, 0.0F, right, top, 0.0F, red, green, blue);
	addLine(geometry, right, top, 0.0F, left, top, 0.0F, red, green, blue);
	addLine(geometry, left, top, 0.0F, left, bottom, 0.0F, red, green, blue);
}

int glyphIndex(char glyph)
{
	if (glyph >= 'A' && glyph <= 'Z')
	{
		return glyph - 'A';
	}
	if (glyph >= '0' && glyph <= '9')
	{
		return 26 + glyph - '0';
	}
	switch (glyph)
	{
	case ' ':
		return 36;
	case ':':
		return 37;
	case '!':
		return 38;
	case '-':
		return 39;
	case '/':
		return 40;
	default:
		return -1;
	}
}

HudAtlasDrawCommand makeAtlasDraw(const Matrix4 &transform,
                                  std::array<float, 4> tint, int firstVertex,
                                  int vertexCount, bool useAtlas)
{
	return {transform, tint, firstVertex, vertexCount, useAtlas};
}
} // namespace

namespace engine::graphics::detail
{
void RenderingHud::setState(const HudState &state)
{
	state_ = state;
}

void RenderingHud::setTerminalState(TerminalHudState state)
{
	terminalState_ = state;
}

TerminalHudState RenderingHud::terminalState() const
{
	return terminalState_;
}

HudTextUpdateResult RenderingHud::setTexts(const std::vector<HudText> &texts)
{
	std::vector<float> vertices;
	for (const HudText &text : texts)
	{
		if (text.glyphSize <= 0.0F)
		{
			return {HudTextUpdateError::NonPositiveGlyphSize, '\0'};
		}
		for (std::size_t characterIndex = 0;
		     characterIndex < text.content.size(); ++characterIndex)
		{
			const int index = glyphIndex(text.content[characterIndex]);
			if (index < 0)
			{
				return {HudTextUpdateError::UnsupportedGlyph,
				        text.content[characterIndex]};
			}
			const float left =
			    text.x + static_cast<float>(characterIndex) * text.glyphSize;
			const float right = left + text.glyphSize;
			const float top = text.y;
			const float bottom = top - text.glyphSize;
			const float u0 = static_cast<float>(index % 16) / 16.0F;
			const float u1 = static_cast<float>((index % 16) + 1) / 16.0F;
			const float v0 = static_cast<float>(index / 16) / 3.0F;
			const float v1 = static_cast<float>((index / 16) + 1) / 3.0F;
			const float quad[] = {
			    left, bottom, u0, v1, left,  top, u0, v0, right, top,    u1, v0,
			    left, bottom, u0, v1, right, top, u1, v0, right, bottom, u1, v1,
			};
			vertices.insert(vertices.end(), std::begin(quad), std::end(quad));
		}
	}
	texts_ = texts;
	glyphVertices_ = std::move(vertices);
	return {};
}

const std::vector<HudText> &RenderingHud::texts() const
{
	return texts_;
}

const std::vector<float> &RenderingHud::glyphVertices() const
{
	return glyphVertices_;
}

HudDrawPlan RenderingHud::buildDrawPlan() const
{
	HudDrawPlan plan;
	plan.gameplayHudVisible = terminalState_ != TerminalHudState::Title;
	constexpr float barRed = 0.16F;
	constexpr float barGreen = 0.74F;
	constexpr float barBlue = 0.82F;
	if (plan.gameplayHudVisible)
	{
		appendRectangleOutline(plan.lines, -0.94F, -0.96F, 0.94F, -0.72F,
		                       barRed, barGreen, barBlue);
		addLine(plan.lines, -0.48F, -0.96F, 0.0F, -0.48F, -0.72F, 0.0F, barRed,
		        barGreen, barBlue);
		addLine(plan.lines, -0.02F, -0.96F, 0.0F, -0.02F, -0.72F, 0.0F, barRed,
		        barGreen, barBlue);
		addLine(plan.lines, 0.44F, -0.96F, 0.0F, 0.44F, -0.72F, 0.0F, barRed,
		        barGreen, barBlue);
		appendCounter(plan.lines, state_.lifeCount, -0.73F, -0.89F, 1.0F, 0.8F,
		              0.82F);
		appendCounter(plan.lines, state_.bombCapacity, -0.27F, -0.89F, 1.0F,
		              0.9F, 0.58F);
		appendCounter(plan.lines, state_.activeBombCount, 0.19F, -0.89F, 1.0F,
		              0.72F, 0.5F);
		appendCounter(plan.lines, state_.blastRange, 0.65F, -0.89F, 0.76F, 1.0F,
		              0.8F);
	}

	plan.lineTransform = identityMatrix();
	plan.lineTransform.values[0] = 0.25F;
	plan.lineTransform.values[5] = 0.25F;
	plan.lineTransform.values[13] = -0.695F;
	constexpr std::array<float, 4> hudTint = {0.01F, 0.02F, 0.04F, 0.88F};
	constexpr std::array<float, 4> titleTint = {0.015F, 0.08F, 0.18F, 0.96F};
	constexpr std::array<float, 4> victoryTint = {0.02F, 0.16F, 0.08F, 0.42F};
	constexpr std::array<float, 4> defeatTint = {0.20F, 0.025F, 0.035F, 0.42F};
	constexpr std::array<float, 4> pauseTint = {0.28F, 0.20F, 0.015F, 0.34F};
	if (plan.gameplayHudVisible)
	{
		plan.atlasDraws[plan.atlasDrawCount++] =
		    makeAtlasDraw(plan.lineTransform, hudTint, 0, 6, false);
		if (terminalState_ != TerminalHudState::None)
		{
			const std::array<float, 4> terminalTint =
			    terminalState_ == TerminalHudState::Success
			        ? victoryTint
			        : (terminalState_ == TerminalHudState::Paused ? pauseTint
			                                                      : defeatTint);
			plan.atlasDraws[plan.atlasDrawCount++] =
			    makeAtlasDraw(identityMatrix(), terminalTint, 0, 6, false);
		}
		plan.atlasDraws[plan.atlasDrawCount++] =
		    makeAtlasDraw(plan.lineTransform, hudTint, 6, 24, true);
	}
	else
	{
		Matrix4 titleTransform = identityMatrix();
		titleTransform.values[0] = 0.55F;
		titleTransform.values[5] = 1.80F;
		titleTransform.values[13] = 1.512F;
		plan.atlasDraws[plan.atlasDrawCount++] =
		    makeAtlasDraw(titleTransform, titleTint, 0, 6, false);
		plan.atlasDraws[plan.atlasDrawCount++] =
		    makeAtlasDraw(titleTransform, titleTint, 30, 6, true);
	}
	return plan;
}

const std::array<float, 144> &RenderingHud::iconAtlasVertices()
{
	static constexpr std::array<float, 144> vertices = {
	    -0.94F, -0.96F, 0.00F,  0.00F,  -0.94F, -0.72F, 0.00F,  1.00F,  0.94F,
	    -0.72F, 0.00F,  1.00F,  0.94F,  -0.72F, 0.00F,  1.00F,  0.94F,  -0.96F,
	    0.00F,  0.00F,  -0.94F, -0.96F, 0.00F,  0.00F,  -0.88F, -0.91F, 0.00F,
	    1.00F,  -0.88F, -0.79F, 0.00F,  0.00F,  -0.76F, -0.79F, 0.25F,  0.00F,
	    -0.88F, -0.91F, 0.00F,  1.00F,  -0.76F, -0.79F, 0.25F,  0.00F,  -0.76F,
	    -0.91F, 0.25F,  1.00F,  -0.42F, -0.91F, 0.25F,  1.00F,  -0.42F, -0.79F,
	    0.25F,  0.00F,  -0.30F, -0.79F, 0.50F,  0.00F,  -0.42F, -0.91F, 0.25F,
	    1.00F,  -0.30F, -0.79F, 0.50F,  0.00F,  -0.30F, -0.91F, 0.50F,  1.00F,
	    0.04F,  -0.91F, 0.50F,  1.00F,  0.04F,  -0.79F, 0.50F,  0.00F,  0.16F,
	    -0.79F, 0.75F,  0.00F,  0.04F,  -0.91F, 0.50F,  1.00F,  0.16F,  -0.79F,
	    0.75F,  0.00F,  0.16F,  -0.91F, 0.75F,  1.00F,  0.50F,  -0.91F, 0.75F,
	    1.00F,  0.50F,  -0.79F, 0.75F,  0.00F,  0.62F,  -0.79F, 1.00F,  0.00F,
	    0.50F,  -0.91F, 0.75F,  1.00F,  0.62F,  -0.79F, 1.00F,  0.00F,  0.62F,
	    -0.91F, 1.00F,  1.00F,  -0.18F, -0.91F, 0.25F,  1.00F,  -0.18F, -0.77F,
	    0.25F,  0.00F,  0.18F,  -0.77F, 0.50F,  0.00F,  -0.18F, -0.91F, 0.25F,
	    1.00F,  0.18F,  -0.77F, 0.50F,  0.00F,  0.18F,  -0.91F, 0.50F,  1.00F,
	};
	return vertices;
}
} // namespace engine::graphics::detail
