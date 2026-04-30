#pragma once

#include <algorithm>

#include "core/Vec3.h"

namespace tinyray {

enum class MaterialType
{
    Diffuse,
    Metal,
    Glass
};

class Material
{
public:
    MaterialType type = MaterialType::Diffuse;
    Vec3 albedo = Vec3(0.8, 0.8, 0.8);
    double roughness = 0.2;
    double refractiveIndex = 1.5;

    double clampedRoughness() const
    {
        return std::clamp(roughness, 0.0, 1.0);
    }

    double safeRefractiveIndex() const
    {
        return std::max(refractiveIndex, 1.0);
    }
};

} // namespace tinyray
