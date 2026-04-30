#include "core/Renderer.h"

#include <algorithm>

#include <QColor>

#include "core/RayTracer.h"

namespace tinyray {

namespace {

int toByte(double value)
{
    const double clamped = std::clamp(value, 0.0, 1.0);
    return static_cast<int>(clamped * 255.0 + 0.5);
}

} // namespace

QImage Renderer::render(const Scene& scene, const RenderSettings& settings,
                        const ProgressCallback& progressCallback)
{
    const int width = std::max(settings.width, 1);
    const int height = std::max(settings.height, 1);
    Camera camera = scene.camera;
    camera.aspectRatio = static_cast<double>(width) / static_cast<double>(height);
    const RayTracer rayTracer;

    QImage image(width, height, QImage::Format_RGB32);
    image.fill(qRgb(8, 10, 14));

    int lastProgress = -1;
    auto reportProgress = [&](int value) {
        const int progress = std::clamp(value, 0, 100);
        if (progressCallback && progress != lastProgress) {
            lastProgress = progress;
            progressCallback(progress);
        }
    };

    reportProgress(0);

    for (int y = 0; y < height; ++y) {
        if (stopRequested()) {
            break;
        }

        QRgb* row = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < width; ++x) {
            if (stopRequested()) {
                break;
            }

            const double u = (static_cast<double>(x) + 0.5) / static_cast<double>(width);
            const double v = 1.0 - (static_cast<double>(y) + 0.5) / static_cast<double>(height);

            const Ray ray = camera.generateRay(u, v);
            const Vec3 color = rayTracer.trace(ray, scene);

            row[x] = qRgb(toByte(color.x), toByte(color.y), toByte(color.z));
        }

        const int progress = static_cast<int>((static_cast<double>(y + 1) / height) * 100.0);
        reportProgress(progress);
    }

    return image;
}

void Renderer::requestStop()
{
    stopRequested_.store(true);
}

void Renderer::resetStopFlag()
{
    stopRequested_.store(false);
}

bool Renderer::stopRequested() const
{
    return stopRequested_.load();
}

} // namespace tinyray
