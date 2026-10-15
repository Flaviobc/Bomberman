#include "graphics/features/VfxSystem.hpp"

#include "graphics/Renderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace engine::graphics::detail
{
void VfxSystem::emit(const ParticleBurst &request)
{
	// The existing generic burst also accepts an empty request to cancel VFX.
	if (request.count == 0 && request.durationSeconds == 0.0)
	{
		clear();
		return;
	}
	if (request.count <= 0 || request.durationSeconds <= 0.0 ||
	    !std::isfinite(request.durationSeconds) ||
	    !std::isfinite(request.delaySeconds) || request.delaySeconds < 0.0 ||
	    !std::isfinite(request.transform.x) ||
	    !std::isfinite(request.transform.y) ||
	    !std::isfinite(request.transform.z) || !std::isfinite(request.red) ||
	    !std::isfinite(request.green) || !std::isfinite(request.blue))
	{
		std::fputs(
		    "VFX burst rejected: invalid position, color, count or duration.\n",
		    stderr);
		return;
	}
	const bool isFireball = request.sprite == VfxSpriteKind::Fireball;
	if (isFireball)
	{
		fireballEmitters_.emplace_back(request);
	}
	else
	{
		emitters_.emplace_back(request);
	}
	std::fprintf(stderr,
	             "VFX %s spawned: particles=%d duration=%.3f "
	             "position=(%.3f, %.3f, %.3f).\n",
	             isFireball ? "fireball" : "smoke", request.count,
	             request.durationSeconds, request.transform.x,
	             request.transform.y, request.transform.z);
}

void VfxSystem::update(double deltaSeconds)
{
	if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0.0)
	{
		return;
	}
	for (SmokeEmitter &emitter : emitters_)
	{
		emitter.update(deltaSeconds);
	}
	for (FireballEmitter &emitter : fireballEmitters_)
	{
		emitter.update(deltaSeconds);
	}
	const std::size_t removed =
	    std::erase_if(emitters_, [](const SmokeEmitter &emitter)
		              { return emitter.expired(); });
	if (removed != 0)
	{
		std::fprintf(stderr, "VFX smoke expired: emitters=%zu remaining=%zu.\n",
		             removed, emitters_.size());
	}
	const std::size_t fireballsRemoved =
	    std::erase_if(fireballEmitters_, [](const FireballEmitter &emitter)
		              { return emitter.expired(); });
	if (fireballsRemoved != 0)
	{
		std::fprintf(stderr,
		             "VFX fireball expired: emitters=%zu remaining=%zu.\n",
		             fireballsRemoved, fireballEmitters_.size());
	}
}

void VfxSystem::clear()
{
	emitters_.clear();
	fireballEmitters_.clear();
	sortedSmoke_.clear();
	sortedFireball_.clear();
	smokeVertices_.clear();
	fireballVertices_.clear();
	std::fputs("VFX active instances cleared.\n", stderr);
}

VfxSystem::SpriteBatches VfxSystem::buildSprites(const Matrix4 &view)
{
	const auto collect =
	    [&view](const auto &emitters, std::vector<SortedParticle> &sorted)
	{
		sorted.clear();
		for (const auto &emitter : emitters)
		{
			for (const SmokeParticle &particle : emitter.particles())
			{
				if (particle.ageSeconds < 0.0)
				{
					continue;
				}
				const Vector3 &p = particle.position;
				sorted.push_back(
				    {particle, view.values[2] * p.x + view.values[6] * p.y +
					               view.values[10] * p.z + view.values[14]});
			}
		}
		std::sort(sorted.begin(), sorted.end(),
		          [](const SortedParticle &a, const SortedParticle &b)
		          { return a.viewDepth < b.viewDepth; });
	};
	const auto build = [&view](const std::vector<SortedParticle> &sorted,
	                           std::vector<VfxSpriteVertex> &vertices)
	{
		vertices.clear();
		vertices.reserve(sorted.size() * 6);
		const Vector3 right{view.values[0], view.values[4], view.values[8]};
		const Vector3 up{view.values[1], view.values[5], view.values[9]};
		constexpr float corners[6][2] = {{0, 0}, {1, 0}, {1, 1},
		                                 {0, 0}, {1, 1}, {0, 1}};
		for (const SortedParticle &entry : sorted)
		{
			const SmokeParticle &p = entry.particle;
			const float cosine = std::cos(p.rotationRadians);
			const float sine = std::sin(p.rotationRadians);
			for (const auto &corner : corners)
			{
				const float localX = (corner[0] - 0.5F) * p.size;
				const float localY = (corner[1] - 0.5F) * p.size;
				const float x = localX * cosine - localY * sine;
				const float y = localX * sine + localY * cosine;
				vertices.push_back({p.position.x + right.x * x + up.x * y,
				                    p.position.y + right.y * x + up.y * y,
				                    p.position.z + right.z * x + up.z * y,
				                    corner[0], 1.0F - corner[1], p.color[0],
				                    p.color[1], p.color[2], p.alpha});
			}
		}
	};
	collect(emitters_, sortedSmoke_);
	collect(fireballEmitters_, sortedFireball_);
	build(sortedSmoke_, smokeVertices_);
	build(sortedFireball_, fireballVertices_);
	return {smokeVertices_, fireballVertices_};
}

const std::vector<SmokeEmitter> &VfxSystem::emitters() const
{
	return emitters_;
}

const std::vector<FireballEmitter> &VfxSystem::fireballEmitters() const
{
	return fireballEmitters_;
}
} // namespace engine::graphics::detail
