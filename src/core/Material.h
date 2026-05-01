#pragma once

#include <algorithm>
#include <cmath>

#include "core/Vec3.h"

namespace tinyray {

enum class MaterialType
{
    Diffuse,
    Metal,
    Glass,
    Emissive
};

class Material
{
public:
    MaterialType type = MaterialType::Diffuse;
    Vec3 albedo = Vec3(0.8, 0.8, 0.8);
    double roughness = 0.2;
    double refractiveIndex = 1.5;
    Vec3 emissionColor = Vec3(1.0, 0.6, 0.1);
    double emissionStrength = 3.0;

    double clampedRoughness() const
    {
        return std::clamp(roughness, 0.0, 1.0);
    }

    double safeRefractiveIndex() const
    {
        return std::max(refractiveIndex, 1.0);
    }

    double safeEmissionStrength() const
    {
        if (!std::isfinite(emissionStrength)) {
            return 0.0;
        }
        return std::max(emissionStrength, 0.0);
    }

    Vec3 emittedRadiance() const
    {
        if (type != MaterialType::Emissive
            || !std::isfinite(emissionColor.x)
            || !std::isfinite(emissionColor.y)
            || !std::isfinite(emissionColor.z)) {
            return Vec3(0.0, 0.0, 0.0);
        }
        return emissionColor * safeEmissionStrength();
    }
};

} // namespace tinyray
