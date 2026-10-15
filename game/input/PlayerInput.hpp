/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   PlayerInput.hpp                                   :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/09/25          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/09/25          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

namespace engine::input
{
class KeyboardState;
}

namespace game
{
struct MoveAction
{
	float horizontal = 0.0F;
	float vertical = 0.0F;
};

struct PlayerActions
{
	MoveAction move;
	bool togglePause = false;
	bool placeBomb = false;
};

class PlayerInput
{
  public:
	PlayerActions actions(const engine::input::KeyboardState &keyboard) const;
};
} // namespace game
