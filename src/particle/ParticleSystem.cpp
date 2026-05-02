#include "particle/ParticleSystem.h"

#include <algorithm>
#include <cmath>

namespace tinyray {

ParticleSystem::ParticleSystem(std::size_t maxParticles)
    : maxParticles_(std::max<std::size_t>(1, maxParticles))
{
    particles_.reserve(maxParticles_);
}

bool ParticleSystem::addParticle(const Particle& particle)
{
    if (!particle.alive || !particle.isFinite()) {
        return false;
    }

    if (particles_.size() >= maxParticles_) {
        auto dead = std::find_if(particles_.begin(), particles_.end(), [](const Particle& item) {
            return !item.alive;
        });
        if (dead == particles_.end()) {
            return false;
        }
        *dead = particle;
        return true;
    }

    particles_.push_back(particle);
    return true;
}

void ParticleSystem::update(double deltaTimeSeconds)
{
    if (!std::isfinite(deltaTimeSeconds) || deltaTimeSeconds <= 0.0) {
        return;
    }

    const double dt = std::clamp(deltaTimeSeconds, 0.0, 0.05);
    for (Particle& particle : particles_) {
        if (!particle.alive) {
            continue;
        }

        particle.age += dt;
        if (particle.age >= particle.lifetime) {
            particle.alive = false;
            continue;
        }

        particle.velocity += particle.acceleration * dt;
        particle.position += particle.velocity * dt;
        if (!particle.isFinite()) {
            particle.alive = false;
        }
    }

    compactDeadParticles();
}

void ParticleSystem::handleRainCollisions(const CollisionPredicate& predicate,
                                          const ImpactCallback& impactCallback)
{
    if (!predicate) {
        return;
    }

    std::vector<Vec3> impactPoints;
    for (Particle& particle : particles_) {
        if (!particle.alive || particle.type != ParticleType::Rain) {
            continue;
        }

        Vec3 impactPoint;
        if (!predicate(particle, impactPoint)) {
            continue;
        }

        particle.alive = false;
        impactPoints.push_back(impactPoint);
    }

    compactDeadParticles();
    if (impactCallback) {
        for (const Vec3& impactPoint : impactPoints) {
            impactCallback(impactPoint);
        }
    }
}

void ParticleSystem::clear()
{
    particles_.clear();
}

void ParticleSystem::setMaxParticles(std::size_t maxParticles)
{
    maxParticles_ = std::max<std::size_t>(1, maxParticles);
    if (particles_.size() > maxParticles_) {
        particles_.resize(maxParticles_);
    }
    particles_.reserve(maxParticles_);
}

std::size_t ParticleSystem::maxParticles() const
{
    return maxParticles_;
}

std::size_t ParticleSystem::particleCount() const
{
    return particles_.size();
}

bool ParticleSystem::empty() const
{
    return particles_.empty();
}

const std::vector<Particle>& ParticleSystem::particles() const
{
    return particles_;
}

void ParticleSystem::compactDeadParticles()
{
    particles_.erase(
        std::remove_if(particles_.begin(), particles_.end(), [](const Particle& particle) {
            return !particle.alive;
        }),
        particles_.end());
}

} // namespace tinyray
