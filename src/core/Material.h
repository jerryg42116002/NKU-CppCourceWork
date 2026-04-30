#pragma once

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
    double indexOfRefraction = 1.5;
    double emissionStrength = 0.0;
};

} // namespace tinyray
