#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include "core/Vec3.h"
#include "particle/Particle.h"

namespace tinyray {

class ParticleSystem
{
public:
    using CollisionPredicate = std::function<bool(const Particle&, Vec3& impactPoint)>;
    using ImpactCallback = std::function<void(const Vec3& impactPoint)>;

    explicit ParticleSystem(std::size_t maxParticles = 5000);

    bool addParticle(const Particle& particle);
    void update(double deltaTimeSeconds);
    void handleRainCollisions(const CollisionPredicate& predicate,
                              const ImpactCallback& impactCallback);
    void clear();
    void setMaxParticles(std::size_t maxParticles);

    std::size_t maxParticles() const;
    std::size_t particleCount() const;
    bool empty() const;
    const std::vector<Particle>& particles() const;

private:
    void compactDeadParticles();

    std::vector<Particle> particles_;
    std::size_t maxParticles_ = 5000;
};

} // namespace tinyray
