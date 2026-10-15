#include "graphics/features/FireballEmitter.hpp"

#include "graphics/Renderer.hpp"

#include <algorithm>
#include <cmath>

namespace engine::graphics::detail
{
FireballEmitter::FireballEmitter(const ParticleBurst &request)
{
	particles_.reserve(static_cast<std::size_t>(request.count));
	for (int index = 0; index < request.count; ++index)
	{
		const float angle = static_cast<float>(index) * 2.39996323F;
		const float variation = 0.68F + 0.11F * static_cast<float>(index % 5);
		const float radialOffset = 0.012F + 0.006F * (index % 3);
		const float sizeScale = 0.72F + 0.12F * (index % 5);
		particles_.push_back(
		    {{request.transform.x + std::cos(angle) * radialOffset,
			  request.transform.y + 0.006F * static_cast<float>(index % 3),
			  request.transform.z + std::sin(angle) * radialOffset},
			 {std::cos(angle) * definition_.spreadSpeed * variation,
			  definition_.riseSpeed * variation,
			  std::sin(angle) * definition_.spreadSpeed * variation},
			 0.0,
			 request.durationSeconds *
			     (0.82 + 0.045 * static_cast<double>(index % 5)),
			 definition_.initialSize,
			 {request.red, request.green, request.blue},
			 0.0F,
			 sizeScale,
			 angle * 0.57F});
	}
}

void FireballEmitter::update(double deltaSeconds)
{
	for (SmokeParticle &particle : particles_)
	{
		const double step = std::min(deltaSeconds, particle.durationSeconds -
		                                               particle.ageSeconds);
		particle.ageSeconds += step;
		const float decay =
		    std::exp(-definition_.drag * static_cast<float>(step));
		const float travel = (1.0F - decay) / definition_.drag;
		particle.position.x += particle.velocity.x * travel;
		particle.position.y += particle.velocity.y * travel;
		particle.position.z += particle.velocity.z * travel;
		particle.velocity.x *= decay;
		particle.velocity.y *= decay;
		particle.velocity.z *= decay;
		particle.size = (definition_.initialSize +
		                 definition_.growthPerSecond *
		                     static_cast<float>(particle.ageSeconds)) *
		                particle.sizeScale;
		const float age =
		    static_cast<float>(particle.ageSeconds / particle.durationSeconds);
		particle.alpha = definition_.opacity * std::min(age / 0.06F, 1.0F) *
		                 std::min((1.0F - age) / 0.30F, 1.0F);
	}
	std::erase_if(particles_, [](const SmokeParticle &particle)
	              { return particle.ageSeconds >= particle.durationSeconds; });
}

const std::vector<SmokeParticle> &FireballEmitter::particles() const
{
	return particles_;
}

bool FireballEmitter::expired() const
{
	return particles_.empty();
}
} // namespace engine::graphics::detail
