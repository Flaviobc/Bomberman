#pragma once

#include "graphics/Renderer.hpp"

#include <optional>

namespace engine::graphics::detail
{
struct ActivePointLight
{
	PointLightBurst burst;
	double remainingSeconds = 0.0;
};

// Owns one CPU request by value; the next request replaces it completely.
class RenderingPointLight
{
  public:
	void emit(const PointLightBurst &light);
	// Seconds. Expire before Renderer reads uniforms, including at delta zero.
	void advance(double deltaSeconds);
	[[nodiscard]] const std::optional<ActivePointLight> &active() const;

  private:
	std::optional<ActivePointLight> active_;
};
} // namespace engine::graphics::detail
