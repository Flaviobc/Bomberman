/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   KeyboardState.hpp                                 :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/09/25          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/09/25          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

#include <array>
#include <cstddef>

namespace engine::input
{
enum class Key : std::size_t
{
	A,
	D,
	S,
	W,
	Left,
	Right,
	Up,
	Down,
	P,
	Space,
	Count,
};

class KeyboardState
{
  public:
	void setPressed(Key key, bool pressed);
	bool isPressed(Key key) const;
	bool wasPressedThisFrame(Key key) const;
	void beginFrame();
	void clear();

  private:
	std::array<bool, static_cast<std::size_t>(Key::Count)> pressed_{};
	std::array<bool, static_cast<std::size_t>(Key::Count)> pressedThisFrame_{};
};
} // namespace engine::input
