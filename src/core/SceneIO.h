#pragma once

#include <QString>

#include "core/RenderSettings.h"
#include "core/Scene.h"

namespace tinyray {

class SceneIO
{
public:
    static bool saveToFile(const Scene& scene,
                           const RenderSettings& settings,
                           const QString& fileName,
                           QString* errorMessage = nullptr);

    static bool loadFromFile(const QString& fileName,
                             Scene& scene,
                             RenderSettings& settings,
                             QString* errorMessage = nullptr);
};

} // namespace tinyray
