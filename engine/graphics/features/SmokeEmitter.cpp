#include "graphics/features/SmokeEmitter.hpp"

#include "graphics/Renderer.hpp"

#include <algorithm>
#include <cmath>

namespace engine::graphics::detail
{
SmokeEmitter::SmokeEmitter(const ParticleBurst &request)
{
	spawn(request);
}

void SmokeEmitter::spawn(const ParticleBurst &request)
{
	particles_.reserve(static_cast<std::size_t>(request.count));
	for (int index = 0; index < request.count; ++index)
	{
		// A deterministic spiral varies direction/speed without mutable RNG
		// state.
		const float angle = static_cast<float>(index) * 2.39996323F;
		const float variation = 0.65F + 0.35F * (index % 4) / 3.0F;
		particles_.push_back(
		    {{request.transform.x, request.transform.y, request.transform.z},
			 {std::cos(angle) * definition_.spreadSpeed * variation,
			  definition_.riseSpeed * variation,
			  std::sin(angle) * definition_.spreadSpeed * variation},
			 -request.delaySeconds,
			 request.durationSeconds,
			 definition_.initialSize,
			 {request.red, request.green, request.blue},
			 0.0F,
			 1.0F,
			 0.0F});
	}
}

void SmokeEmitter::update(double deltaSeconds)
{
	for (SmokeParticle &particle : particles_)
	{
		double remainingSeconds = deltaSeconds;
		if (particle.ageSeconds < 0.0)
		{
			const double waitingStep =
			    std::min(remainingSeconds, -particle.ageSeconds);
			particle.ageSeconds += waitingStep;
			remainingSeconds -= waitingStep;
			if (remainingSeconds <= 0.0)
			{
				continue;
			}
		}
		const double step = std::min(
		    remainingSeconds, particle.durationSeconds - particle.ageSeconds);
		particle.ageSeconds += step;
		// Integrate visual drag analytically so splitting a frame keeps the
		// path.
		const float decay =
		    std::exp(-definition_.drag * static_cast<float>(step));
		const float travel = (1.0F - decay) / definition_.drag;
		particle.position.x += particle.velocity.x * travel;
		particle.position.y += particle.velocity.y * travel;
		particle.position.z += particle.velocity.z * travel;
		particle.velocity.x *= decay;
		particle.velocity.y *= decay;
		particle.velocity.z *= decay;
		particle.size = definition_.initialSize +
		                definition_.growthPerSecond *
		                    static_cast<float>(particle.ageSeconds);
		const float age =
		    static_cast<float>(particle.ageSeconds / particle.durationSeconds);
		particle.alpha =
		    definition_.opacity * std::min((1.0F - age) / 0.28F, 1.0F);
	}
	std::erase_if(particles_, [](const SmokeParticle &particle)
	              { return particle.ageSeconds >= particle.durationSeconds; });
}

const std::vector<SmokeParticle> &SmokeEmitter::particles() const
{
	return particles_;
}

bool SmokeEmitter::expired() const
{
	return particles_.empty();
}
} // namespace engine::graphics::detail
