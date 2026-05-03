#pragma once

#include <algorithm>
#include <cmath>

namespace tinyray {

class BloomSettings
{
public:
    bool enabled = true;
    bool cinematicGlowEnabled = false;
    double exposure = 1.0;
    double threshold = 1.0;
    double strength = 0.75;
    double cinematicGlowStrength = 0.55;
    int blurPassCount = 6;

    double safeExposure() const
    {
        return std::isfinite(exposure) ? std::clamp(exposure, 0.0, 5.0) : 1.0;
    }

    double safeThreshold() const
    {
        return std::isfinite(threshold) ? std::clamp(threshold, 0.0, 5.0) : 1.0;
    }

    double safeStrength() const
    {
        return std::isfinite(strength) ? std::clamp(strength, 0.0, 5.0) : 0.75;
    }

    double safeCinematicGlowStrength() const
    {
        return std::isfinite(cinematicGlowStrength)
            ? std::clamp(cinematicGlowStrength, 0.0, 3.0)
            : 0.55;
    }

    int safeBlurPassCount() const
    {
        return std::clamp(blurPassCount, 1, 16);
    }
};

} // namespace tinyray
