#pragma once

#include "graphics/features/SmokeEmitter.hpp"

#include <vector>

namespace engine::graphics::detail
{
struct FireballDefinition
{
	float spreadSpeed = 0.17F;
	float riseSpeed = 0.13F;
	float drag = 2.6F;
	float initialSize = 0.18F;
	float growthPerSecond = 0.36F;
	float opacity = 1.0F;
};

class FireballEmitter
{
  public:
	explicit FireballEmitter(const ParticleBurst &request);
	void update(double deltaSeconds);
	[[nodiscard]] const std::vector<SmokeParticle> &particles() const;
	[[nodiscard]] bool expired() const;

  private:
	static constexpr FireballDefinition definition_{};
	std::vector<SmokeParticle> particles_;
};
} // namespace engine::graphics::detail
