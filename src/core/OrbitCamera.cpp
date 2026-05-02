#include "core/OrbitCamera.h"

#include <algorithm>
#include <cmath>

#include <QVector3D>

#include "core/MathUtils.h"

namespace tinyray {

namespace {

constexpr double minPitch = -89.0;
constexpr double maxPitch = 89.0;
constexpr double minDistance = 0.25;
constexpr double maxDistance = 500.0;
constexpr double minFov = 5.0;
constexpr double maxFov = 120.0;
constexpr double minFocusDistance = 0.05;
constexpr double maxFocusDistance = 500.0;

bool finite(double value)
{
    return std::isfinite(value);
}

bool finite(const Vec3& value)
{
    return finite(value.x) && finite(value.y) && finite(value.z);
}

double wrapDegrees(double degrees)
{
    if (!finite(degrees)) {
        return 0.0;
    }

    double wrapped = std::fmod(degrees, 360.0);
    if (wrapped < -180.0) {
        wrapped += 360.0;
    } else if (wrapped >= 180.0) {
        wrapped -= 360.0;
    }
    return wrapped;
}

QVector3D toQVector(const Vec3& value)
{
    return QVector3D(static_cast<float>(value.x),
                     static_cast<float>(value.y),
                     static_cast<float>(value.z));
}

} // namespace

Vec3 OrbitCamera::position() const
{
    const Vec3 forward = forwardDirection();
    return target - forward * distance;
}

QMatrix4x4 OrbitCamera::viewMatrix() const
{
    QMatrix4x4 matrix;
    const Vec3 eye = position();
    matrix.lookAt(toQVector(eye), toQVector(target), toQVector(up.normalized()));
    return matrix;
}

QMatrix4x4 OrbitCamera::projectionMatrix(double aspectRatio, double nearPlane, double farPlane) const
{
    const double safeAspect = finite(aspectRatio) && aspectRatio > 0.0 ? aspectRatio : 16.0 / 9.0;
    const double safeNear = finite(nearPlane) && nearPlane > 0.0 ? nearPlane : 0.1;
    const double safeFar = finite(farPlane) && farPlane > safeNear ? farPlane : 500.0;

    QMatrix4x4 matrix;
    matrix.perspective(static_cast<float>(std::clamp(fov, minFov, maxFov)),
                       static_cast<float>(safeAspect),
                       static_cast<float>(safeNear),
                       static_cast<float>(safeFar));
    return matrix;
}

void OrbitCamera::orbit(double deltaYawDegrees, double deltaPitchDegrees)
{
    yaw += deltaYawDegrees;
    pitch += deltaPitchDegrees;
    sanitize();
}

void OrbitCamera::zoom(double scaleFactor)
{
    if (!finite(scaleFactor) || scaleFactor <= 0.0) {
        return;
    }

    distance *= scaleFactor;
    sanitize();
}

void OrbitCamera::pan(double deltaRight, double deltaUp)
{
    if (!finite(deltaRight) || !finite(deltaUp)) {
        return;
    }

    const Vec3 forward = forwardDirection();
    Vec3 right = cross(forward, up).normalized();
    if (right.nearZero()) {
        right = Vec3(1.0, 0.0, 0.0);
    }

    Vec3 cameraUp = cross(right, forward).normalized();
    if (cameraUp.nearZero()) {
        cameraUp = Vec3(0.0, 1.0, 0.0);
    }

    const double panScale = std::max(distance, minDistance) * 0.0025;
    target += right * (deltaRight * panScale) + cameraUp * (deltaUp * panScale);
    sanitize();
}

bool OrbitCamera::updateTurntable(double deltaTimeSeconds)
{
    sanitize();
    if (!turntableEnabled
        || !finite(deltaTimeSeconds)
        || deltaTimeSeconds <= 0.0
        || turntableSpeed <= 0.0) {
        return false;
    }

    const double previousYaw = yaw;
    const double directionSign = static_cast<int>(turntableDirection) < 0 ? -1.0 : 1.0;
    yaw = wrapDegrees(yaw + directionSign * turntableSpeed * deltaTimeSeconds);
    sanitize();
    return std::abs(yaw - previousYaw) > 1.0e-9;
}

Vec3 OrbitCamera::resolvedTurntableTarget(const Vec3& sceneCenter, const Vec3* selectedObjectCenter) const
{
    const Vec3 safeSceneCenter = finite(sceneCenter) ? sceneCenter : Vec3(0.0, 0.0, 0.0);
    if (turntableTargetMode == TurntableTargetMode::SelectedObject) {
        return selectedObjectCenter != nullptr && finite(*selectedObjectCenter)
            ? *selectedObjectCenter
            : safeSceneCenter;
    }

    if (turntableTargetMode == TurntableTargetMode::CustomTarget) {
        return finite(turntableCustomTarget) ? turntableCustomTarget : safeSceneCenter;
    }

    return safeSceneCenter;
}

bool OrbitCamera::isValid() const
{
    return finite(target)
        && finite(up)
        && finite(distance)
        && finite(yaw)
        && finite(pitch)
        && finite(fov)
        && finite(aperture)
        && finite(focusDistance)
        && distance > minDistance * 0.5
        && aperture >= 0.0
        && focusDistance > 0.0
        && !forwardDirection().nearZero();
}

Vec3 OrbitCamera::forwardDirection() const
{
    const double yawRadians = degreesToRadians(yaw);
    const double pitchRadians = degreesToRadians(std::clamp(pitch, minPitch, maxPitch));

    const double cosPitch = std::cos(pitchRadians);
    Vec3 direction(
        std::sin(yawRadians) * cosPitch,
        std::sin(pitchRadians),
        -std::cos(yawRadians) * cosPitch);

    if (!finite(direction) || direction.nearZero()) {
        return Vec3(0.0, 0.0, -1.0);
    }

    return direction.normalized();
}

void OrbitCamera::sanitize()
{
    if (!finite(target)) {
        target = Vec3(0.0, 0.0, 0.0);
    }
    if (!finite(up) || up.nearZero()) {
        up = Vec3(0.0, 1.0, 0.0);
    }
    if (!finite(distance)) {
        distance = 5.0;
    }
    if (!finite(yaw)) {
        yaw = 0.0;
    }
    if (!finite(pitch)) {
        pitch = 15.0;
    }
    if (!finite(fov)) {
        fov = 45.0;
    }
    if (!finite(aperture)) {
        aperture = 0.35;
    }
    if (!finite(focusDistance)) {
        focusDistance = distance;
    }
    if (!finite(turntableSpeed)) {
        turntableSpeed = 24.0;
    }
    if (!finite(turntableCustomTarget)) {
        turntableCustomTarget = target;
    }

    distance = std::clamp(distance, minDistance, maxDistance);
    yaw = wrapDegrees(yaw);
    pitch = std::clamp(pitch, minPitch, maxPitch);
    fov = std::clamp(fov, minFov, maxFov);
    aperture = std::clamp(aperture, 0.0, 5.0);
    focusDistance = std::clamp(focusDistance, minFocusDistance, maxFocusDistance);
    turntableSpeed = std::clamp(turntableSpeed, 0.0, 360.0);
}

} // namespace tinyray
