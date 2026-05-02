#pragma once

#include "core/Vec3.h"

namespace tinyray {

enum class ParticleType
{
    Rain,
    Splash
};

struct Particle
{
    Vec3 position = Vec3(0.0, 0.0, 0.0);
    Vec3 velocity = Vec3(0.0, 0.0, 0.0);
    Vec3 acceleration = Vec3(0.0, 0.0, 0.0);
    Vec3 color = Vec3(0.55, 0.75, 1.0);
    double alpha = 1.0;
    double lifetime = 1.0;
    double age = 0.0;
    double size = 0.035;
    bool alive = false;
    ParticleType type = ParticleType::Rain;

    bool isFinite() const;
    double normalizedAge() const;
};

} // namespace tinyray
