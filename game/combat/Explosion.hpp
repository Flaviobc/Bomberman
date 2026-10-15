/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   Explosion.hpp                                     :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/09/25          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/09/25          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

#include <vector>

namespace game
{
struct ExplosionCell
{
	int column = 0;
	int row = 0;
};

struct ExplosionProjectionCell
{
	float x = 0.0F;
	float y = 0.0F;
	float width = 0.0F;
	float height = 0.0F;
};

class Explosion
{
  public:
	Explosion(std::vector<ExplosionCell> cells, double durationSeconds);

	bool update(double deltaSeconds);
	const std::vector<ExplosionCell> &cells() const;
	double remainingSeconds() const;

  private:
	std::vector<ExplosionCell> cells_;
	double remainingSeconds_;
};
} // namespace game
