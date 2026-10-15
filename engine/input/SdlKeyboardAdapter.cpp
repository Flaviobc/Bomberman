/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   SdlKeyboardAdapter.cpp                            :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/09/25          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/09/25          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "input/SdlKeyboardAdapter.hpp"

#include <SDL3/SDL_keycode.h>

namespace engine::input
{
std::optional<Key> keyFromSdlKeycode(SDL_Keycode keycode)
{
	switch (keycode)
	{
	case SDLK_A:
		return Key::A;
	case SDLK_D:
		return Key::D;
	case SDLK_S:
		return Key::S;
	case SDLK_W:
		return Key::W;
	case SDLK_LEFT:
		return Key::Left;
	case SDLK_RIGHT:
		return Key::Right;
	case SDLK_UP:
		return Key::Up;
	case SDLK_DOWN:
		return Key::Down;
	case SDLK_P:
		return Key::P;
	case SDLK_SPACE:
		return Key::Space;
	default:
		return std::nullopt;
	}
}
} // namespace engine::input
