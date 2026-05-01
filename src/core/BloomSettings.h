#pragma once

#include <algorithm>
#include <cmath>

namespace tinyray {

struct BloomSettings
{
    bool enabled = true;
    double exposure = 1.0;
    double threshold = 1.15;
    double strength = 0.65;
    int blurPassCount = 6;

    double safeThreshold() const
    {
        return std::isfinite(threshold) ? std::clamp(threshold, 0.0, 20.0) : 1.15;
    }

    double safeExposure() const
    {
        return std::isfinite(exposure) ? std::clamp(exposure, 0.05, 8.0) : 1.0;
    }

    double safeStrength() const
    {
        return std::isfinite(strength) ? std::clamp(strength, 0.0, 5.0) : 0.65;
    }

    int safeBlurPassCount() const
    {
        return std::clamp(blurPassCount, 1, 16);
    }
};

} // namespace tinyray
