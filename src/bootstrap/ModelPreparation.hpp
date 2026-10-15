/* ******************************************************************************** */
/*                                                                                    */
/*                                                      :::      ::::::::   :::      */
/*   ModelPreparation.hpp                             :+:      :+:    :+: :+:      */
/*                                                    +:+ +:+   +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              +#+  +:+ +#+#+#+#+#+   +#+      */
/*                                                +#+#+#+#+#+   +#+           +#+#+#+#+#+   */
/*   Created: 2026/10/08          by flaviobc      #+#    #+#             */
/*                                                                                    */
/* ******************************************************************************** */

#pragma once

#include "assets/GlbLoader.hpp"
#include "graphics/Renderer.hpp"

#include "presentation/GamePresentation.hpp"

#include <cstddef>
#include <vector>

namespace bootstrap
{
struct ModelIds
{
	std::size_t player = 0;
	std::size_t indestructibleWall = 1;
	std::size_t labyrinthPillar = 2;
	std::size_t destructibleWall = 3;
	std::size_t bomb = 4;
	std::size_t floor = 5;
	std::size_t powerUpBombCapacity = 6;
	std::size_t powerUpAura = 7;
	std::size_t powerUpBlastRange = 8;
	std::size_t enemyBasic = 9;
	std::size_t powerUpLife = 10;
};

struct PreparedModels
{
	// Inputs borrow mesh/image storage from OwnedAssets::document.
	std::vector<engine::graphics::StaticMeshInput> inputs;
	ModelIds ids;
	game::SessionModelBounds bounds;
};

bool prepareModels(const engine::assets::GlbDocument &document,
                   PreparedModels &prepared);
} // namespace bootstrap
