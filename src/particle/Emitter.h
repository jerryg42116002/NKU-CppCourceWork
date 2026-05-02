#pragma once

#include <random>

#include "core/Vec3.h"
#include "particle/ParticleSystem.h"

namespace tinyray {

struct RainSettings
{
    bool rainEnabled = false;
    bool splashEnabled = true;
    double rainRate = 260.0;
    double dropSpeed = 15.0;
    double gravity = 18.0;
    double spawnAreaSize = 7.5;
    double spawnHeight = 5.0;
    double randomness = 0.75;
    double splashIntensity = 1.0;
    double particleSize = 0.012;
    double dropLifetime = 0.85;
};

class Emitter
{
public:
    virtual ~Emitter() = default;
    virtual void emitParticles(ParticleSystem& system, double deltaTimeSeconds) = 0;

    void setEnabled(bool enabled);
    bool enabled() const;
    void setRate(double particlesPerSecond);
    double rate() const;

protected:
    double consumeEmissionCount(double deltaTimeSeconds);
    double randomReal(double minValue, double maxValue);

    bool enabled_ = true;
    double rate_ = 100.0;
    double accumulator_ = 0.0;
    std::mt19937 random_{1337u};
};

class RainEmitter : public Emitter
{
public:
    RainEmitter();

    void setSettings(const RainSettings& settings);
    const RainSettings& settings() const;
    void emitParticles(ParticleSystem& system, double deltaTimeSeconds) override;

private:
    RainSettings settings_;
};

class SplashEmitter
{
public:
    void setEnabled(bool enabled);
    bool enabled() const;
    void setIntensity(double intensity);
    void setParticleSize(double size);
    void setGravity(double gravity);
    void emitSplash(ParticleSystem& system, const Vec3& impactPoint);

private:
    double randomReal(double minValue, double maxValue);

    bool enabled_ = true;
    double intensity_ = 1.0;
    double particleSize_ = 0.025;
    double gravity_ = 18.0;
    std::mt19937 random_{4242u};
};

} // namespace tinyray
