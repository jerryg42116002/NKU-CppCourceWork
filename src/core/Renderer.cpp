#include "core/Renderer.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

#include <QColor>

#include "core/Random.h"
#include "core/RayTracer.h"

namespace tinyray {

namespace {

int toByte(double value)
{
    if (!std::isfinite(value)) {
        return 0;
    }

    const double clamped = std::clamp(value, 0.0, 1.0);
    return static_cast<int>(clamped * 255.0 + 0.5);
}

double gammaCorrect(double linearValue)
{
    if (!std::isfinite(linearValue)) {
        return 0.0;
    }

    const double clamped = std::clamp(linearValue, 0.0, 1.0);
    constexpr double inverseGamma = 1.0 / 2.2;
    return std::pow(clamped, inverseGamma);
}

Vec3 toDisplayColor(const Vec3& linearColor)
{
    return Vec3(
        gammaCorrect(linearColor.x),
        gammaCorrect(linearColor.y),
        gammaCorrect(linearColor.z));
}

} // namespace

QImage Renderer::render(const Scene& scene, const RenderSettings& settings,
                        const ProgressCallback& progressCallback,
                        const PreviewCallback& previewCallback)
{
    const int width = std::max(settings.width, 1);
    const int height = std::max(settings.height, 1);
    const int samplesPerPixel = std::max(settings.samplesPerPixel, 1);
    const int maxDepth = std::max(settings.maxDepth, 1);
    const int requestedThreads = settings.threadCount > 0 ? settings.threadCount : defaultThreadCount();
    const int workerCount = std::clamp(requestedThreads, 1, height);
    const int totalRowSamples = height * samplesPerPixel;

    Camera camera = scene.camera;
    camera.aspectRatio = static_cast<double>(width) / static_cast<double>(height);

    QImage image(width, height, QImage::Format_RGB32);
    image.fill(qRgb(8, 10, 14));

    unsigned char* imageBits = image.bits();
    const int bytesPerLine = image.bytesPerLine();
    std::vector<Vec3> accumulatedColors(static_cast<std::size_t>(width * height));

    std::mutex imageMutex;
    std::mutex previewMutex;
    std::atomic_int completedRowSamples = 0;
    std::atomic_int lastProgress = -1;

    const auto startTime = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point lastPreviewTime = startTime;

    auto elapsedSeconds = [&]() {
        const auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(now - startTime).count();
    };

    auto reportProgress = [&](int value) {
        const int progress = std::clamp(value, 0, 100);
        int previous = lastProgress.load();
        while (progressCallback && progress > previous) {
            if (lastProgress.compare_exchange_weak(previous, progress)) {
                progressCallback(progress);
                return;
            }
        }
    };

    auto publishPreview = [&](int currentSample, int progress, bool force) {
        if (!previewCallback) {
            return;
        }

        QImage imageCopy;
        {
            std::lock_guard<std::mutex> previewLock(previewMutex);
            const auto now = std::chrono::steady_clock::now();
            const auto elapsedSinceLastPreview =
                std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPreviewTime).count();
            if (!force && elapsedSinceLastPreview < 100) {
                return;
            }
            lastPreviewTime = now;

            std::lock_guard<std::mutex> imageLock(imageMutex);
            imageCopy = image.copy();
        }

        previewCallback(imageCopy,
                        std::clamp(currentSample, 0, samplesPerPixel),
                        samplesPerPixel,
                        std::clamp(progress, 0, 100),
                        elapsedSeconds());
    };

    auto renderRowSample = [&](int y, int sampleIndex, RayTracer& rayTracer, Random& random) {
        std::vector<QRgb> rowPixels(static_cast<std::size_t>(width));

        for (int x = 0; x < width; ++x) {
            if (stopRequested()) {
                break;
            }

            const double u = (static_cast<double>(x) + random.real()) / static_cast<double>(width);
            const double v = 1.0 - (static_cast<double>(y) + random.real()) / static_cast<double>(height);
            const Ray ray = camera.generateRay(u, v);

            const int pixelIndex = y * width + x;
            accumulatedColors[static_cast<std::size_t>(pixelIndex)] += rayTracer.trace(ray, scene, maxDepth);

            const Vec3 averageColor =
                accumulatedColors[static_cast<std::size_t>(pixelIndex)] / static_cast<double>(sampleIndex + 1);
            const Vec3 displayColor = toDisplayColor(averageColor);
            rowPixels[static_cast<std::size_t>(x)] =
                qRgb(toByte(displayColor.x), toByte(displayColor.y), toByte(displayColor.z));
        }

        {
            std::lock_guard<std::mutex> imageLock(imageMutex);
            QRgb* row = reinterpret_cast<QRgb*>(imageBits + y * bytesPerLine);
            std::memcpy(row, rowPixels.data(), static_cast<std::size_t>(width) * sizeof(QRgb));
        }
    };

    auto renderSamplePass = [&](int sampleIndex) {
        std::atomic_int nextRow = 0;

        auto worker = [&](int workerIndex) {
            const unsigned int seed = static_cast<unsigned int>((sampleIndex + 1) * 1009 + workerIndex + 1);
            RayTracer rayTracer;
            Random random(seed);

            while (!stopRequested()) {
                const int y = nextRow.fetch_add(1);
                if (y >= height) {
                    break;
                }

                renderRowSample(y, sampleIndex, rayTracer, random);

                const int done = completedRowSamples.fetch_add(1) + 1;
                const int progress = static_cast<int>((static_cast<double>(done) / totalRowSamples) * 100.0);
                reportProgress(progress);
                publishPreview(sampleIndex + 1, progress, false);
            }
        };

        std::vector<std::thread> workers;
        workers.reserve(static_cast<std::size_t>(workerCount));
        for (int workerIndex = 0; workerIndex < workerCount; ++workerIndex) {
            workers.emplace_back(worker, workerIndex);
        }

        for (std::thread& thread : workers) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    };

    reportProgress(0);
    publishPreview(0, 0, true);

    for (int sampleIndex = 0; sampleIndex < samplesPerPixel; ++sampleIndex) {
        if (stopRequested()) {
            break;
        }

        renderSamplePass(sampleIndex);

        const int progress = static_cast<int>(
            (static_cast<double>(completedRowSamples.load()) / totalRowSamples) * 100.0);
        publishPreview(sampleIndex + 1, progress, true);
    }

    if (!stopRequested()) {
        reportProgress(100);
        publishPreview(samplesPerPixel, 100, true);
    } else {
        const int completed = completedRowSamples.load();
        const int progress = static_cast<int>((static_cast<double>(completed) / totalRowSamples) * 100.0);
        const int currentSample = std::min(samplesPerPixel, (completed / height) + 1);
        publishPreview(currentSample, progress, true);
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
