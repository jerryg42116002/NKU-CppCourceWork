#pragma once

#include <algorithm>

#include "core/Vec3.h"

namespace tinyray {

enum class LightType
{
    Point,
    Area
};

class Light
{
public:
    LightType type = LightType::Point;
    Vec3 position = Vec3(0.0, 3.0, 0.0);
    Vec3 normal = Vec3(0.0, -1.0, 0.0);
    double width = 2.0;
    double height = 2.0;
    Vec3 color = Vec3(1.0, 1.0, 1.0);
    double intensity = 10.0;

    Light() = default;

    Light(const Vec3& position, const Vec3& color, double intensity)
        : type(LightType::Point)
        , position(position)
        , color(color)
        , intensity(intensity)
    {
    }

    static Light area(const Vec3& center,
                      const Vec3& normal,
                      double width,
                      double height,
                      const Vec3& color,
                      double intensity)
    {
        Light light;
        light.type = LightType::Area;
        light.position = center;
        light.normal = normal.nearZero() ? Vec3(0.0, -1.0, 0.0) : normal.normalized();
        light.width = std::max(width, 0.01);
        light.height = std::max(height, 0.01);
        light.color = color;
        light.intensity = std::max(intensity, 0.0);
        return light;
    }

    Vec3 safeNormal() const
    {
        return normal.nearZero() ? Vec3(0.0, -1.0, 0.0) : normal.normalized();
    }

    Vec3 tangent() const
    {
        const Vec3 n = safeNormal();
        Vec3 t = cross(n, Vec3(0.0, 1.0, 0.0)).normalized();
        if (t.nearZero()) {
            t = cross(n, Vec3(1.0, 0.0, 0.0)).normalized();
        }
        return t.nearZero() ? Vec3(1.0, 0.0, 0.0) : t;
    }

    Vec3 bitangent() const
    {
        Vec3 b = cross(safeNormal(), tangent()).normalized();
        return b.nearZero() ? Vec3(0.0, 0.0, 1.0) : b;
    }

    Vec3 sampleArea(double u, double v) const
    {
        const double safeWidth = std::max(width, 0.01);
        const double safeHeight = std::max(height, 0.01);
        const double offsetU = std::clamp(u, 0.0, 1.0) - 0.5;
        const double offsetV = std::clamp(v, 0.0, 1.0) - 0.5;
        return position + tangent() * (offsetU * safeWidth) + bitangent() * (offsetV * safeHeight);
    }
};

} // namespace tinyray
