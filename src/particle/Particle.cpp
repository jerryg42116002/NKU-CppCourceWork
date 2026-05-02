#include "particle/Particle.h"

#include <algorithm>
#include <cmath>

namespace tinyray {

namespace {

bool finite(const Vec3& value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

} // namespace

bool Particle::isFinite() const
{
    return finite(position)
        && finite(velocity)
        && finite(acceleration)
        && finite(color)
        && std::isfinite(alpha)
        && std::isfinite(lifetime)
        && std::isfinite(age)
        && std::isfinite(size);
}

double Particle::normalizedAge() const
{
    if (!std::isfinite(lifetime) || lifetime <= 0.0) {
        return 1.0;
    }
    return std::clamp(age / lifetime, 0.0, 1.0);
}

} // namespace tinyray
