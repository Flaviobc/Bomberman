/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   PlayerInput.cpp                                   :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/09/25          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/09/25          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "input/PlayerInput.hpp"

#include "input/KeyboardState.hpp"

namespace game
{
PlayerActions
PlayerInput::actions(const engine::input::KeyboardState &keyboard) const
{
	const bool left = keyboard.isPressed(engine::input::Key::A) ||
	                  keyboard.isPressed(engine::input::Key::Left);
	const bool right = keyboard.isPressed(engine::input::Key::D) ||
	                   keyboard.isPressed(engine::input::Key::Right);
	const bool up = keyboard.isPressed(engine::input::Key::W) ||
	                keyboard.isPressed(engine::input::Key::Up);
	const bool down = keyboard.isPressed(engine::input::Key::S) ||
	                  keyboard.isPressed(engine::input::Key::Down);
	return {{(right ? 1.0F : 0.0F) - (left ? 1.0F : 0.0F),
	         (up ? 1.0F : 0.0F) - (down ? 1.0F : 0.0F)},
	        keyboard.wasPressedThisFrame(engine::input::Key::P),
	        keyboard.wasPressedThisFrame(engine::input::Key::Space)};
}
} // namespace game
