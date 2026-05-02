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
    double aperture = 0.35;
    double focusDistance = 5.0;

    Ray generateRay(double u, double v) const
    {
        return generateRay(u, v, 0.0, 0.0);
    }

    Ray generateRay(double u, double v, double lensU, double lensV) const
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
        Vec3 direction = lowerLeftCorner + u * horizontal + v * vertical - position;

        const double safeAperture = std::isfinite(aperture) ? std::clamp(aperture, 0.0, 5.0) : 0.0;
        if (safeAperture <= 1.0e-8) {
            return Ray(position, direction.normalized());
        }

        Vec3 unitDirection = direction.normalized();
        if (unitDirection.nearZero()) {
            unitDirection = -backward;
        }

        const double safeFocusDistance = std::isfinite(focusDistance)
            ? std::clamp(focusDistance, 0.05, 500.0)
            : std::max((lookAt - position).length(), 0.05);
        const double forwardProjection = std::max(dot(unitDirection, -backward), 1.0e-4);
        const Vec3 focusPoint = position + unitDirection * (safeFocusDistance / forwardProjection);

        const double clampedLensU = std::isfinite(lensU) ? std::clamp(lensU, 0.0, 1.0) : 0.5;
        const double clampedLensV = std::isfinite(lensV) ? std::clamp(lensV, 0.0, 1.0) : 0.5;
        const double diskRadius = std::sqrt(clampedLensU);
        const double diskAngle = 2.0 * pi * clampedLensV;
        const Vec3 lensOffset = right * (std::cos(diskAngle) * diskRadius * safeAperture * 0.5)
            + cameraUp * (std::sin(diskAngle) * diskRadius * safeAperture * 0.5);
        const Vec3 origin = position + lensOffset;
        direction = focusPoint - origin;
        if (direction.nearZero()) {
            direction = unitDirection;
        }

        return Ray(origin, direction.normalized());
    }
};

} // namespace tinyray
