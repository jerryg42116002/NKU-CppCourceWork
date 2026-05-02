#include "particle/Emitter.h"

#include <algorithm>
#include <cmath>

namespace tinyray {

namespace {

double safePositive(double value, double fallback, double minValue, double maxValue)
{
    if (!std::isfinite(value)) {
        return fallback;
    }
    return std::clamp(value, minValue, maxValue);
}

} // namespace

void Emitter::setEnabled(bool enabled)
{
    enabled_ = enabled;
    if (!enabled_) {
        accumulator_ = 0.0;
    }
}

bool Emitter::enabled() const
{
    return enabled_;
}

void Emitter::setRate(double particlesPerSecond)
{
    rate_ = safePositive(particlesPerSecond, 100.0, 0.0, 5000.0);
}

double Emitter::rate() const
{
    return rate_;
}

double Emitter::consumeEmissionCount(double deltaTimeSeconds)
{
    if (!enabled_ || !std::isfinite(deltaTimeSeconds) || deltaTimeSeconds <= 0.0 || rate_ <= 0.0) {
        return 0.0;
    }

    accumulator_ += rate_ * std::clamp(deltaTimeSeconds, 0.0, 0.05);
    const double count = std::floor(accumulator_);
    accumulator_ -= count;
    return count;
}

double Emitter::randomReal(double minValue, double maxValue)
{
    std::uniform_real_distribution<double> distribution(minValue, maxValue);
    return distribution(random_);
}

RainEmitter::RainEmitter()
{
    setSettings(settings_);
}

void RainEmitter::setSettings(const RainSettings& settings)
{
    settings_ = settings;
    settings_.rainRate = safePositive(settings_.rainRate, 260.0, 0.0, 3000.0);
    settings_.dropSpeed = safePositive(settings_.dropSpeed, 15.0, 0.1, 80.0);
    settings_.gravity = safePositive(settings_.gravity, 18.0, 0.0, 120.0);
    settings_.spawnAreaSize = safePositive(settings_.spawnAreaSize, 7.5, 0.5, 80.0);
    settings_.spawnHeight = safePositive(settings_.spawnHeight, 5.0, 0.5, 80.0);
    settings_.randomness = safePositive(settings_.randomness, 0.75, 0.0, 8.0);
    settings_.splashIntensity = safePositive(settings_.splashIntensity, 1.0, 0.0, 5.0);
    settings_.particleSize = safePositive(settings_.particleSize, 0.012, 0.002, 0.18);
    setEnabled(settings_.rainEnabled);
    setRate(settings_.rainRate);
}

const RainSettings& RainEmitter::settings() const
{
    return settings_;
}

void RainEmitter::emitParticles(ParticleSystem& system, double deltaTimeSeconds)
{
    const int count = static_cast<int>(consumeEmissionCount(deltaTimeSeconds));
    if (count <= 0) {
        return;
    }

    const double halfArea = settings_.spawnAreaSize * 0.5;
    for (int index = 0; index < count; ++index) {
        Particle particle;
        particle.type = ParticleType::Rain;
        particle.alive = true;
        particle.position = Vec3(randomReal(-halfArea, halfArea),
                                 settings_.spawnHeight,
                                 randomReal(-halfArea, halfArea));
        particle.velocity = Vec3(randomReal(-settings_.randomness, settings_.randomness),
                                 -settings_.dropSpeed,
                                 randomReal(-settings_.randomness, settings_.randomness));
        particle.acceleration = Vec3(0.0, -settings_.gravity, 0.0);
        particle.lifetime = safePositive(settings_.dropLifetime, 0.85, 0.10, 4.0);
        particle.age = 0.0;
        particle.size = settings_.particleSize;
        particle.color = Vec3(0.55, 0.72, 1.0);
        particle.alpha = 0.42;
        system.addParticle(particle);
    }
}

void SplashEmitter::setEnabled(bool enabled)
{
    enabled_ = enabled;
}

bool SplashEmitter::enabled() const
{
    return enabled_;
}

void SplashEmitter::setIntensity(double intensity)
{
    intensity_ = safePositive(intensity, 1.0, 0.0, 5.0);
}

void SplashEmitter::setParticleSize(double size)
{
    particleSize_ = safePositive(size, 0.025, 0.003, 0.25);
}

void SplashEmitter::setGravity(double gravity)
{
    gravity_ = safePositive(gravity, 18.0, 0.0, 120.0);
}

void SplashEmitter::emitSplash(ParticleSystem& system, const Vec3& impactPoint)
{
    if (!enabled_ || intensity_ <= 0.0) {
        return;
    }

    const int count = std::clamp(static_cast<int>(std::round(2.0 + intensity_ * 3.0)), 2, 14);
    for (int index = 0; index < count; ++index) {
        const double angle = randomReal(0.0, 6.28318530717958647692);
        const double radiusSpeed = randomReal(0.8, 2.8) * intensity_;
        const double upwardSpeed = randomReal(1.2, 3.8) * std::sqrt(std::max(intensity_, 0.1));

        Particle particle;
        particle.type = ParticleType::Splash;
        particle.alive = true;
        particle.position = impactPoint + Vec3(0.0, 0.025, 0.0);
        particle.velocity = Vec3(std::cos(angle) * radiusSpeed,
                                 upwardSpeed,
                                 std::sin(angle) * radiusSpeed);
        particle.acceleration = Vec3(0.0, -gravity_, 0.0);
        particle.lifetime = randomReal(0.12, 0.28);
        particle.age = 0.0;
        particle.size = particleSize_ * randomReal(0.55, 1.10);
        particle.color = Vec3(0.82, 0.92, 1.0);
        particle.alpha = 0.62;
        system.addParticle(particle);
    }
}

double SplashEmitter::randomReal(double minValue, double maxValue)
{
    std::uniform_real_distribution<double> distribution(minValue, maxValue);
    return distribution(random_);
}

} // namespace tinyray
