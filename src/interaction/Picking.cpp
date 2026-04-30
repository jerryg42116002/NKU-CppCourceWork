#include "interaction/Picking.h"

#include <algorithm>
#include <cmath>

#include <QMatrix4x4>
#include <QVector3D>
#include <QVector4D>

#include "core/HitRecord.h"
#include "core/MathUtils.h"

namespace tinyray {
namespace {

bool isFinite(const Vec3& value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

Vec3 toVec3(const QVector4D& value)
{
    return Vec3(value.x(), value.y(), value.z());
}

bool homogenize(QVector4D& value)
{
    if (std::abs(value.w()) < 1.0e-8f || !std::isfinite(value.w())) {
        return false;
    }

    value /= value.w();
    return std::isfinite(value.x()) && std::isfinite(value.y()) && std::isfinite(value.z());
}

Ray fallbackRay(const OrbitCamera& camera)
{
    const Vec3 origin = camera.position();
    bool invertible = false;
    const QMatrix4x4 inverseView = camera.viewMatrix().inverted(&invertible);
    QVector3D forwardVector(0.0f, 0.0f, -1.0f);
    if (invertible) {
        forwardVector = inverseView.mapVector(forwardVector);
    }

    Vec3 direction(forwardVector.x(), forwardVector.y(), forwardVector.z());
    if (direction.nearZero() || !isFinite(direction)) {
        direction = Vec3(0.0, 0.0, -1.0);
    }

    return Ray(origin, direction.normalized());
}

} // namespace

Ray makePickingRay(const QPoint& screenPosition,
                   const QSize& viewportSize,
                   const OrbitCamera& camera)
{
    const int viewportWidth = std::max(viewportSize.width(), 1);
    const int viewportHeight = std::max(viewportSize.height(), 1);

    const double ndcX = (2.0 * (static_cast<double>(screenPosition.x()) + 0.5)
                         / static_cast<double>(viewportWidth)) - 1.0;
    const double ndcY = 1.0 - (2.0 * (static_cast<double>(screenPosition.y()) + 0.5)
                               / static_cast<double>(viewportHeight));

    const double aspectRatio = static_cast<double>(viewportWidth) / static_cast<double>(viewportHeight);
    const QMatrix4x4 view = camera.viewMatrix();
    const QMatrix4x4 projection = camera.projectionMatrix(aspectRatio);

    bool invertible = false;
    const QMatrix4x4 inverseViewProjection = (projection * view).inverted(&invertible);
    if (!invertible) {
        return fallbackRay(camera);
    }

    QVector4D nearPoint(static_cast<float>(ndcX), static_cast<float>(ndcY), -1.0f, 1.0f);
    QVector4D farPoint(static_cast<float>(ndcX), static_cast<float>(ndcY), 1.0f, 1.0f);
    nearPoint = inverseViewProjection * nearPoint;
    farPoint = inverseViewProjection * farPoint;

    if (!homogenize(nearPoint) || !homogenize(farPoint)) {
        return fallbackRay(camera);
    }

    const Vec3 origin = camera.position();
    Vec3 direction = toVec3(farPoint) - origin;
    if (direction.nearZero() || !isFinite(origin) || !isFinite(direction)) {
        direction = toVec3(farPoint) - toVec3(nearPoint);
    }
    if (direction.nearZero() || !isFinite(direction)) {
        return fallbackRay(camera);
    }

    return Ray(origin, direction.normalized());
}

int pickObjectId(const QPoint& screenPosition,
                 const QSize& viewportSize,
                 const OrbitCamera& camera,
                 const Scene& scene)
{
    const Ray ray = makePickingRay(screenPosition, viewportSize, camera);
    HitRecord record;
    if (scene.intersect(ray, 0.001, infinity, record)) {
        return record.hitObjectId;
    }

    return -1;
}

} // namespace tinyray
