#pragma once

#include "core/Vec3.h"

namespace tinyray {

enum class LightType
{
    Point
};

class Light
{
public:
    Light() = default;
    Light(const Vec3& positionValue, const Vec3& colorValue, double intensityValue)
        : position(positionValue)
        , color(colorValue)
        , intensity(intensityValue)
    {
    }

    LightType type = LightType::Point;
    Vec3 position = Vec3(0.0, 4.0, 2.0);
    Vec3 color = Vec3(1.0, 1.0, 1.0);
    double intensity = 1.0;
};

} // namespace tinyray
