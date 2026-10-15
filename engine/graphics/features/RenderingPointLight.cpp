#include "graphics/features/RenderingPointLight.hpp"

namespace engine::graphics::detail
{
void RenderingPointLight::emit(const PointLightBurst &light)
{
	active_ = {{light, light.durationSeconds}};
}

void RenderingPointLight::advance(double deltaSeconds)
{
	if (active_)
	{
		active_->remainingSeconds -= deltaSeconds;
		if (active_->remainingSeconds <= 0.0)
		{
			active_.reset();
		}
	}
}

const std::optional<ActivePointLight> &RenderingPointLight::active() const
{
	return active_;
}
} // namespace engine::graphics::detail
