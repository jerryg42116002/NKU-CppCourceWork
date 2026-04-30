#pragma once

#include "core/Vec3.h"

namespace tinyray {

class Ray
{
public:
    Ray() = default;
    Ray(const Vec3& originValue, const Vec3& directionValue)
        : origin(originValue)
        , direction(directionValue)
    {
    }

    Vec3 at(double t) const
    {
        return origin + t * direction;
    }

    Vec3 origin;
    Vec3 direction;
};

} // namespace tinyray
