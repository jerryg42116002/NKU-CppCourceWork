#pragma once

#include <random>

#include "core/Vec3.h"

namespace tinyray {

class Random
{
public:
    Random()
        : generator_(std::random_device{}())
    {
    }

    explicit Random(unsigned int seed)
        : generator_(seed)
    {
    }

    double real()
    {
        return real(0.0, 1.0);
    }

    double real(double minValue, double maxValue)
    {
        std::uniform_real_distribution<double> distribution(minValue, maxValue);
        return distribution(generator_);
    }

    int integer(int minValue, int maxValue)
    {
        std::uniform_int_distribution<int> distribution(minValue, maxValue);
        return distribution(generator_);
    }

    Vec3 vector()
    {
        return Vec3(real(), real(), real());
    }

    Vec3 vector(double minValue, double maxValue)
    {
        return Vec3(real(minValue, maxValue), real(minValue, maxValue), real(minValue, maxValue));
    }

    Vec3 inUnitSphere()
    {
        while (true) {
            const Vec3 candidate = vector(-1.0, 1.0);
            if (candidate.lengthSquared() < 1.0) {
                return candidate;
            }
        }
    }

    Vec3 unitVector()
    {
        return inUnitSphere().normalized();
    }

    Vec3 inHemisphere(const Vec3& normal)
    {
        const Vec3 candidate = inUnitSphere();
        return dot(candidate, normal) > 0.0 ? candidate : -candidate;
    }

private:
    std::mt19937 generator_;
};

} // namespace tinyray
