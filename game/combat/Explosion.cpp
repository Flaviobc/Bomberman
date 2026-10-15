/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   Explosion.cpp                                     :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/09/25          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/09/25          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "combat/Explosion.hpp"

#include <algorithm>
#include <utility>

namespace game
{
Explosion::Explosion(std::vector<ExplosionCell> cells, double durationSeconds)
    : cells_(std::move(cells)), remainingSeconds_(durationSeconds)
{
}

bool Explosion::update(double deltaSeconds)
{
	remainingSeconds_ = std::max(0.0, remainingSeconds_ - deltaSeconds);
	return remainingSeconds_ == 0.0;
}

const std::vector<ExplosionCell> &Explosion::cells() const
{
	return cells_;
}

double Explosion::remainingSeconds() const
{
	return remainingSeconds_;
}
} // namespace game
