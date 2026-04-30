#pragma once

#include <QMatrix4x4>

#include "core/Vec3.h"

namespace tinyray {

class OrbitCamera
{
public:
    Vec3 target = Vec3(0.0, 0.0, 0.0);
    double distance = 5.0;
    double yaw = 0.0;
    double pitch = 15.0;
    double fov = 45.0;
    Vec3 up = Vec3(0.0, 1.0, 0.0);

    Vec3 position() const;
    QMatrix4x4 viewMatrix() const;
    QMatrix4x4 projectionMatrix(double aspectRatio, double nearPlane = 0.1, double farPlane = 500.0) const;

    void orbit(double deltaYawDegrees, double deltaPitchDegrees);
    void zoom(double scaleFactor);
    void pan(double deltaRight, double deltaUp);
    bool isValid() const;

private:
    Vec3 forwardDirection() const;
    void sanitize();
};

} // namespace tinyray
