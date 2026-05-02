#pragma once

#include <algorithm>
#include <cmath>

#include <QColor>
#include <QImage>
#include <QString>

#include "core/Vec3.h"

namespace tinyray {

enum class TextureType
{
    Image,
    Checkerboard
};

class Texture
{
public:
    TextureType type = TextureType::Checkerboard;
    QString imagePath;
    Vec3 colorA = Vec3(0.78, 0.78, 0.74);
    Vec3 colorB = Vec3(0.16, 0.17, 0.18);
    double scale = 6.0;

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
        type = TextureType::Image;
        return true;
    }

    Vec3 sample(double u, double v, const Vec3& fallbackColor) const
    {
        if (!std::isfinite(u) || !std::isfinite(v)) {
            return fallbackColor;
        }

        if (type == TextureType::Image) {
            return sampleImage(u, v, fallbackColor);
        }

        return sampleCheckerboard(u, v);
    }

private:
    mutable QImage cachedImage_;
    mutable QString loadedImagePath_;

    static double repeat(double value)
    {
        return value - std::floor(value);
    }

    Vec3 sampleCheckerboard(double u, double v) const
    {
        const double safeScale = std::isfinite(scale) ? std::max(scale, 0.001) : 1.0;
        const int cellU = static_cast<int>(std::floor(u * safeScale));
        const int cellV = static_cast<int>(std::floor(v * safeScale));
        return ((cellU + cellV) & 1) == 0 ? colorA : colorB;
    }

    Vec3 sampleImage(double u, double v, const Vec3& fallbackColor) const
    {
        ensureImageLoaded();
        if (cachedImage_.isNull()) {
            return fallbackColor;
        }

        const double wrappedU = repeat(u);
        const double wrappedV = repeat(v);
        const double x = wrappedU * static_cast<double>(cachedImage_.width() - 1);
        const double y = (1.0 - wrappedV) * static_cast<double>(cachedImage_.height() - 1);

        const int x0 = std::clamp(static_cast<int>(std::floor(x)), 0, cachedImage_.width() - 1);
        const int y0 = std::clamp(static_cast<int>(std::floor(y)), 0, cachedImage_.height() - 1);
        const int x1 = std::clamp(x0 + 1, 0, cachedImage_.width() - 1);
        const int y1 = std::clamp(y0 + 1, 0, cachedImage_.height() - 1);
        const double tx = x - static_cast<double>(x0);
        const double ty = y - static_cast<double>(y0);

        const Vec3 c00 = pixelToVec3(cachedImage_.pixel(x0, y0));
        const Vec3 c10 = pixelToVec3(cachedImage_.pixel(x1, y0));
        const Vec3 c01 = pixelToVec3(cachedImage_.pixel(x0, y1));
        const Vec3 c11 = pixelToVec3(cachedImage_.pixel(x1, y1));
        const Vec3 cx0 = c00 * (1.0 - tx) + c10 * tx;
        const Vec3 cx1 = c01 * (1.0 - tx) + c11 * tx;
        return cx0 * (1.0 - ty) + cx1 * ty;
    }

    static Vec3 pixelToVec3(QRgb pixel)
    {
        const QColor color(pixel);
        return Vec3(color.redF(), color.greenF(), color.blueF());
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
