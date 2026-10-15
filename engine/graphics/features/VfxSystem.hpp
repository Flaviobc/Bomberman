#pragma once

#include "graphics/features/FireballEmitter.hpp"
#include "graphics/features/SmokeEmitter.hpp"

#include <span>
#include <vector>

namespace engine::graphics::detail
{
struct VfxSpriteVertex
{
	float x, y, z;
	float u, v;
	float red, green, blue, alpha;
};

// CPU-only instances and reusable frame data for the two approved effects.
// There is no emitter registry or dynamic effect graph.
class VfxSystem
{
  public:
	struct SpriteBatches
	{
		std::span<const VfxSpriteVertex> smoke;
		std::span<const VfxSpriteVertex> fireball;
	};

	void emit(const ParticleBurst &request);
	void update(double deltaSeconds);
	void clear();
	[[nodiscard]] SpriteBatches buildSprites(const Matrix4 &view);
	[[nodiscard]] const std::vector<SmokeEmitter> &emitters() const;
	[[nodiscard]] const std::vector<FireballEmitter> &fireballEmitters() const;

  private:
	struct SortedParticle
	{
		SmokeParticle particle;
		float viewDepth;
	};
	std::vector<SmokeEmitter> emitters_;
	std::vector<FireballEmitter> fireballEmitters_;
	std::vector<SortedParticle> sortedSmoke_;
	std::vector<SortedParticle> sortedFireball_;
	std::vector<VfxSpriteVertex> smokeVertices_;
	std::vector<VfxSpriteVertex> fireballVertices_;
};
} // namespace engine::graphics::detail
