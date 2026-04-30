#pragma once

#include <cmath>

#include "core/MathUtils.h"
#include "core/Ray.h"
#include "core/Vec3.h"

namespace tinyray {

class Camera
{
public:
    Vec3 position = Vec3(0.0, 1.0, 5.0);
    Vec3 lookAt = Vec3(0.0, 0.5, 0.0);
    Vec3 up = Vec3(0.0, 1.0, 0.0);
    double fieldOfViewYDegrees = 45.0;
    double aspectRatio = 16.0 / 9.0;

    Ray generateRay(double u, double v) const
    {
        const double theta = degreesToRadians(fieldOfViewYDegrees);
        const double halfViewportHeight = std::tan(theta * 0.5);
        const double viewportHeight = 2.0 * halfViewportHeight;
        const double viewportWidth = aspectRatio * viewportHeight;

        // Camera basis: backward points from the target to the eye.
        Vec3 backward = (position - lookAt).normalized();
        if (backward.nearZero()) {
            backward = Vec3(0.0, 0.0, 1.0);
        }

        Vec3 right = cross(up, backward).normalized();
        if (right.nearZero()) {
            right = cross(Vec3(0.0, 1.0, 0.0), backward).normalized();
        }
        if (right.nearZero()) {
            right = cross(Vec3(1.0, 0.0, 0.0), backward).normalized();
        }
        const Vec3 cameraUp = cross(backward, right).normalized();

        const Vec3 horizontal = viewportWidth * right;
        const Vec3 vertical = viewportHeight * cameraUp;
        const Vec3 lowerLeftCorner = position - horizontal * 0.5 - vertical * 0.5 - backward;
        const Vec3 direction = lowerLeftCorner + u * horizontal + v * vertical - position;

        return Ray(position, direction.normalized());
    }
};

} // namespace tinyray
