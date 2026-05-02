#pragma once

#include <algorithm>
#include <cmath>
#include <QString>

#include "core/Texture.h"
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
    bool useTexture = false;
    TextureType textureType = TextureType::Checkerboard;
    QString texturePath;
    double textureScale = 1.0;
    double textureOffsetU = 0.0;
    double textureOffsetV = 0.0;
    double textureRotation = 0.0;
    double textureStrength = 1.0;
    Vec3 fallbackColor = Vec3(0.8, 0.8, 0.8);
    Vec3 checkerColorA = Vec3(0.78, 0.78, 0.74);
    Vec3 checkerColorB = Vec3(0.16, 0.17, 0.18);
    mutable Texture textureCache;

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

    Vec3 baseColorAt(double u, double v) const
    {
        if (!useTexture) {
            return albedo;
        }

        const double safeScale = std::isfinite(textureScale) ? std::max(textureScale, 0.001) : 1.0;
        const double safeStrength = std::isfinite(textureStrength) ? std::clamp(textureStrength, 0.0, 1.0) : 1.0;
        const double centeredU = u - 0.5;
        const double centeredV = v - 0.5;
        const double c = std::cos(textureRotation);
        const double s = std::sin(textureRotation);
        const double rotatedU = centeredU * c - centeredV * s + 0.5;
        const double rotatedV = centeredU * s + centeredV * c + 0.5;

        textureCache.type = textureType;
        textureCache.imagePath = texturePath;
        textureCache.colorA = checkerColorA;
        textureCache.colorB = checkerColorB;
        textureCache.scale = safeScale;
        const double sampleU = textureType == TextureType::Image
            ? rotatedU * safeScale + textureOffsetU
            : rotatedU + textureOffsetU;
        const double sampleV = textureType == TextureType::Image
            ? rotatedV * safeScale + textureOffsetV
            : rotatedV + textureOffsetV;
        const Vec3 sampled = textureCache.sample(sampleU, sampleV, fallbackColor);
        return albedo * (1.0 - safeStrength) + sampled * safeStrength;
    }
};

} // namespace tinyray
