#pragma once

#include "graphics/foundation/RenderingMath.hpp"

#include <array>
#include <vector>

namespace engine::graphics
{
struct ParticleBurst;
}

namespace engine::graphics::detail
{
// Immutable parameters for the single smoke effect. Distances are world units,
// times are seconds, and drag is inverse seconds; no gameplay data lives here.
struct SmokeDefinition
{
	float spreadSpeed = 0.28F;
	float riseSpeed = 0.05F;
	float drag = 2.0F;
	float initialSize = 0.10F;
	float growthPerSecond = 0.24F;
	float opacity = 0.62F;
};

struct SmokeParticle
{
	Vector3 position;
	Vector3 velocity;
	double ageSeconds = 0.0;
	double durationSeconds = 0.0;
	float size = 0.0F;
	std::array<float, 3> color{};
	float alpha = 0.0F;
	float sizeScale = 1.0F;
	float rotationRadians = 0.0F;
};

class SmokeEmitter
{
  public:
	explicit SmokeEmitter(const ParticleBurst &request);
	void update(double deltaSeconds);
	[[nodiscard]] const std::vector<SmokeParticle> &particles() const;
	[[nodiscard]] bool expired() const;

  private:
	void spawn(const ParticleBurst &request);
	static constexpr SmokeDefinition definition_{};
	std::vector<SmokeParticle> particles_;
};
} // namespace engine::graphics::detail
