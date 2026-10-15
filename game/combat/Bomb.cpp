/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   Bomb.cpp                                          :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/09/25          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/09/25          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "combat/Bomb.hpp"

#include <algorithm>

namespace game
{
Bomb::Bomb(float x, float y, double fuseSeconds)
    : x_(x), y_(y), remainingSeconds_(fuseSeconds)
{
}

bool Bomb::update(double deltaSeconds)
{
	remainingSeconds_ = std::max(0.0, remainingSeconds_ - deltaSeconds);
	return remainingSeconds_ == 0.0;
}

float Bomb::x() const
{
	return x_;
}

float Bomb::y() const
{
	return y_;
}

double Bomb::remainingSeconds() const
{
	return remainingSeconds_;
}
} // namespace game
