#pragma once

#include <algorithm>
#include <cmath>

#include <QColor>
#include <QImage>
#include <QString>

#include "core/Vec3.h"

namespace tinyray {

enum class EnvironmentMode
{
    Gradient,
    SolidColor,
    Image,
    HDRImage
};

class Environment
{
public:
    EnvironmentMode mode = EnvironmentMode::Gradient;
    Vec3 topColor = Vec3(0.30, 0.62, 1.00);
    Vec3 bottomColor = Vec3(0.82, 0.93, 1.00);
    Vec3 solidColor = Vec3(0.48, 0.74, 1.00);
    QString imagePath;
    double exposure = 1.0;
    double rotationY = 0.0;
    double intensity = 1.0;

    bool loadImage(const QString& path)
    {
        QImage image(path);
        if (image.isNull()) {
            cachedImage_ = QImage();
            loadedImagePath_.clear();
            return false;
        }

        imagePath = path;
        cachedImage_ = image.convertToFormat(QImage::Format_RGB32);
        loadedImagePath_ = path;
        mode = EnvironmentMode::Image;
        return true;
    }

    Vec3 sample(const Vec3& direction) const
    {
        Vec3 dir = direction.normalized();
        if (dir.nearZero()
            || !std::isfinite(dir.x)
            || !std::isfinite(dir.y)
            || !std::isfinite(dir.z)) {
            dir = Vec3(0.0, 1.0, 0.0);
        }

        Vec3 color;
        switch (mode) {
        case EnvironmentMode::SolidColor:
            color = solidColor;
            break;
        case EnvironmentMode::Image:
        case EnvironmentMode::HDRImage:
            color = sampleImage(dir);
            break;
        case EnvironmentMode::Gradient:
        default:
            color = sampleGradient(dir);
            break;
        }

        const double safeIntensity = std::isfinite(intensity) ? std::max(intensity, 0.0) : 1.0;
        const double safeExposure = std::isfinite(exposure) ? std::max(exposure, 0.0) : 1.0;
        return color * safeIntensity * safeExposure;
    }

    bool hasUsableImage() const
    {
        ensureImageLoaded();
        return !cachedImage_.isNull();
    }

private:
    static constexpr double twoPi = 6.28318530717958647692;
    static constexpr double pi = 3.14159265358979323846;

    mutable QImage cachedImage_;
    mutable QString loadedImagePath_;

    Vec3 sampleGradient(const Vec3& direction) const
    {
        const double t = std::clamp(0.5 * (direction.y + 1.0), 0.0, 1.0);
        return bottomColor * (1.0 - t) + topColor * t;
    }

    Vec3 sampleImage(const Vec3& direction) const
    {
        ensureImageLoaded();
        if (cachedImage_.isNull()) {
            return sampleGradient(direction);
        }

        const double yaw = std::atan2(direction.z, direction.x) + rotationY;
        double u = 0.5 + yaw / twoPi;
        u = u - std::floor(u);

        const double v = std::clamp(std::acos(std::clamp(direction.y, -1.0, 1.0)) / pi, 0.0, 1.0);
        const int x = std::clamp(static_cast<int>(u * static_cast<double>(cachedImage_.width())), 0, cachedImage_.width() - 1);
        const int y = std::clamp(static_cast<int>(v * static_cast<double>(cachedImage_.height())), 0, cachedImage_.height() - 1);
        const QColor pixel(cachedImage_.pixel(x, y));
        return Vec3(pixel.redF(), pixel.greenF(), pixel.blueF());
    }

    void ensureImageLoaded() const
    {
        if (imagePath.isEmpty() || (!cachedImage_.isNull() && loadedImagePath_ == imagePath)) {
            return;
        }

        QImage image(imagePath);
        if (image.isNull()) {
            cachedImage_ = QImage();
            loadedImagePath_.clear();
            return;
        }

        cachedImage_ = image.convertToFormat(QImage::Format_RGB32);
        loadedImagePath_ = imagePath;
    }
};

} // namespace tinyray
