#pragma once

#include <atomic>
#include <functional>

#include <QImage>

#include "core/RenderSettings.h"
#include "core/Scene.h"

namespace tinyray {

class Renderer
{
public:
    using ProgressCallback = std::function<void(int)>;

    QImage render(const Scene& scene, const RenderSettings& settings,
                  const ProgressCallback& progressCallback = ProgressCallback());

    void requestStop();
    void resetStopFlag();
    bool stopRequested() const;

private:
    std::atomic_bool stopRequested_ = false;
};

} // namespace tinyray
