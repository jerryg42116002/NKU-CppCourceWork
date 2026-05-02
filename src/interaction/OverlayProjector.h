#pragma once

#include <cmath>

#include <QPointF>
#include <QSize>
#include <QVector4D>

#include "core/OrbitCamera.h"
#include "core/Vec3.h"

namespace tinyray {

struct ScreenProjection
{
    QPointF position;
    bool visible = false;
};

inline ScreenProjection worldToScreen(const Vec3& worldPosition,
                                      const QSize& viewportSize,
                                      const OrbitCamera& camera)
{
    if (viewportSize.width() <= 0 || viewportSize.height() <= 0) {
        return {};
    }

    if (!std::isfinite(worldPosition.x)
        || !std::isfinite(worldPosition.y)
        || !std::isfinite(worldPosition.z)) {
        return {};
    }

    const double aspect = static_cast<double>(viewportSize.width())
        / static_cast<double>(viewportSize.height());
    const QMatrix4x4 view = camera.viewMatrix();
    const QMatrix4x4 projection = camera.projectionMatrix(aspect);
    const QVector4D clip = projection * view * QVector4D(static_cast<float>(worldPosition.x),
                                                         static_cast<float>(worldPosition.y),
                                                         static_cast<float>(worldPosition.z),
                                                         1.0f);

    if (!std::isfinite(clip.x())
        || !std::isfinite(clip.y())
        || !std::isfinite(clip.z())
        || !std::isfinite(clip.w())
        || clip.w() <= 1.0e-6f) {
        return {};
    }

    const double ndcX = static_cast<double>(clip.x() / clip.w());
    const double ndcY = static_cast<double>(clip.y() / clip.w());
    const double ndcZ = static_cast<double>(clip.z() / clip.w());
    if (!std::isfinite(ndcX)
        || !std::isfinite(ndcY)
        || !std::isfinite(ndcZ)
        || ndcX < -1.0
        || ndcX > 1.0
        || ndcY < -1.0
        || ndcY > 1.0
        || ndcZ < -1.0
        || ndcZ > 1.0) {
        return {};
    }

    ScreenProjection result;
    result.position = QPointF((ndcX * 0.5 + 0.5) * static_cast<double>(viewportSize.width()),
                              (1.0 - (ndcY * 0.5 + 0.5)) * static_cast<double>(viewportSize.height()));
    result.visible = true;
    return result;
}

} // namespace tinyray
