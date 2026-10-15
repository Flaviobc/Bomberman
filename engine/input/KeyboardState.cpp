/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   KeyboardState.cpp                                 :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/09/25          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/09/25          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "input/KeyboardState.hpp"

namespace engine::input
{
void KeyboardState::setPressed(Key key, bool pressed)
{
	const std::size_t index = static_cast<std::size_t>(key);
	if (pressed && !pressed_[index])
	{
		pressedThisFrame_[index] = true;
	}
	pressed_[index] = pressed;
}

bool KeyboardState::isPressed(Key key) const
{
	return pressed_[static_cast<std::size_t>(key)];
}

bool KeyboardState::wasPressedThisFrame(Key key) const
{
	return pressedThisFrame_[static_cast<std::size_t>(key)];
}

void KeyboardState::beginFrame()
{
	pressedThisFrame_.fill(false);
}

void KeyboardState::clear()
{
	pressed_.fill(false);
	pressedThisFrame_.fill(false);
}
} // namespace engine::input
