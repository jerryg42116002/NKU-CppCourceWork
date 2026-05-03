#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include <QColor>
#include <QImage>

#include "core/BloomSettings.h"
#include "core/Vec3.h"

namespace tinyray {

class ImagePostProcess
{
public:
    static QImage applyBloom(const QImage& source, const BloomSettings& settings)
    {
        if ((!settings.enabled && !settings.cinematicGlowEnabled) || source.isNull()) {
            return source;
        }

        const QImage input = source.convertToFormat(QImage::Format_RGB32);
        const int width = input.width();
        const int height = input.height();
        if (width <= 0 || height <= 0) {
            return input;
        }

        const double threshold = settings.cinematicGlowEnabled
            ? std::min(settings.safeThreshold(), 0.78)
            : settings.safeThreshold();
        std::vector<Vec3> scene(static_cast<std::size_t>(width * height));
        std::vector<Vec3> bright(scene.size());

        for (int y = 0; y < height; ++y) {
            const QRgb* row = reinterpret_cast<const QRgb*>(input.constScanLine(y));
            for (int x = 0; x < width; ++x) {
                const Vec3 color = pixelToVec3(row[x]);
                const std::size_t index = static_cast<std::size_t>(y * width + x);
                scene[index] = color;
                const double luma = luminance(color);
                const double brightFactor = std::clamp((luma - threshold) / 0.35, 0.0, 1.0);
                bright[index] = color * brightFactor;
            }
        }

        std::vector<Vec3> blurred = bright;
        const int blurPasses = settings.safeBlurPassCount();
        for (int pass = 0; pass < blurPasses; ++pass) {
            blurred = blurHorizontal(blurred, width, height);
            blurred = blurVertical(blurred, width, height);
        }

        std::vector<Vec3> wideGlow = blurred;
        if (settings.cinematicGlowEnabled) {
            const int widePasses = std::min(blurPasses + 4, 16);
            for (int pass = 0; pass < widePasses; ++pass) {
                wideGlow = blurHorizontal(wideGlow, width, height);
                wideGlow = blurVertical(wideGlow, width, height);
            }
        }

        QImage output(width, height, QImage::Format_RGB32);
        const double strength = settings.safeStrength();
        const double glowStrength = settings.safeCinematicGlowStrength();
        for (int y = 0; y < height; ++y) {
            QRgb* row = reinterpret_cast<QRgb*>(output.scanLine(y));
            for (int x = 0; x < width; ++x) {
                const std::size_t index = static_cast<std::size_t>(y * width + x);
                Vec3 color = scene[index];
                if (settings.enabled) {
                    color += blurred[index] * strength;
                }
                if (settings.cinematicGlowEnabled) {
                    const Vec3 centerGlow = wideGlow[index] * Vec3(0.92, 0.96, 1.0);
                    const Vec3 warmFringe =
                        sampleClamped(wideGlow, width, height, x + 3, y) * Vec3(1.0, 0.46, 0.18);
                    const Vec3 coolFringe =
                        sampleClamped(wideGlow, width, height, x - 3, y) * Vec3(0.18, 0.50, 1.0);
                    const Vec3 verticalAura =
                        sampleClamped(wideGlow, width, height, x, y - 4) * Vec3(0.60, 0.30, 1.0);
                    color += (centerGlow * 0.78
                              + warmFringe * 0.18
                              + coolFringe * 0.22
                              + verticalAura * 0.16) * glowStrength;
                }
                row[x] = vec3ToPixel(color);
            }
        }

        return output;
    }

private:
    static double luminance(const Vec3& color)
    {
        return color.x * 0.2126 + color.y * 0.7152 + color.z * 0.0722;
    }

    static Vec3 pixelToVec3(QRgb pixel)
    {
        const QColor color(pixel);
        return Vec3(color.redF(), color.greenF(), color.blueF());
    }

    static QRgb vec3ToPixel(const Vec3& color)
    {
        const auto toByte = [](double value) {
            return static_cast<int>(std::clamp(value, 0.0, 1.0) * 255.0 + 0.5);
        };
        return qRgb(toByte(color.x), toByte(color.y), toByte(color.z));
    }

    static Vec3 sampleClamped(const std::vector<Vec3>& pixels, int width, int height, int x, int y)
    {
        x = std::clamp(x, 0, width - 1);
        y = std::clamp(y, 0, height - 1);
        return pixels[static_cast<std::size_t>(y * width + x)];
    }

    static std::vector<Vec3> blurHorizontal(const std::vector<Vec3>& source, int width, int height)
    {
        std::vector<Vec3> result(source.size());
        constexpr double weights[5] = {0.070270, 0.316216, 0.227027, 0.316216, 0.070270};
        constexpr int offsets[5] = {-3, -1, 0, 1, 3};

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                Vec3 color(0.0, 0.0, 0.0);
                for (int index = 0; index < 5; ++index) {
                    color += sampleClamped(source, width, height, x + offsets[index], y) * weights[index];
                }
                result[static_cast<std::size_t>(y * width + x)] = color;
            }
        }

        return result;
    }

    static std::vector<Vec3> blurVertical(const std::vector<Vec3>& source, int width, int height)
    {
        std::vector<Vec3> result(source.size());
        constexpr double weights[5] = {0.070270, 0.316216, 0.227027, 0.316216, 0.070270};
        constexpr int offsets[5] = {-3, -1, 0, 1, 3};

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                Vec3 color(0.0, 0.0, 0.0);
                for (int index = 0; index < 5; ++index) {
                    color += sampleClamped(source, width, height, x, y + offsets[index]) * weights[index];
                }
                result[static_cast<std::size_t>(y * width + x)] = color;
            }
        }

        return result;
    }
};

} // namespace tinyray
