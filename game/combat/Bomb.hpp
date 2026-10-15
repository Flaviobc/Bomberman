/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   Bomb.hpp                                          :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/09/25          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/09/25          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

namespace game
{
class Bomb
{
  public:
	Bomb(float x, float y, double fuseSeconds);

	bool update(double deltaSeconds);
	float x() const;
	float y() const;
	double remainingSeconds() const;

  private:
	float x_;
	float y_;
	double remainingSeconds_;
};
} // namespace game
