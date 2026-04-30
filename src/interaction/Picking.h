#pragma once

#include <QPoint>
#include <QSize>

#include "core/OrbitCamera.h"
#include "core/Ray.h"
#include "core/Scene.h"

namespace tinyray {

Ray makePickingRay(const QPoint& screenPosition,
                   const QSize& viewportSize,
                   const OrbitCamera& camera);

int pickObjectId(const QPoint& screenPosition,
                 const QSize& viewportSize,
                 const OrbitCamera& camera,
                 const Scene& scene);

} // namespace tinyray
