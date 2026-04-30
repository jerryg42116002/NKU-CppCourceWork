#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

namespace tinyray {

constexpr double pi = 3.1415926535897932385;
constexpr double infinity = std::numeric_limits<double>::infinity();

inline double degreesToRadians(double degrees)
{
    return degrees * pi / 180.0;
}

inline double radiansToDegrees(double radians)
{
    return radians * 180.0 / pi;
}

inline double clamp(double value, double minValue, double maxValue)
{
    return std::clamp(value, minValue, maxValue);
}

inline bool nearlyEqual(double left, double right, double epsilon = 1.0e-8)
{
    return std::abs(left - right) <= epsilon;
}

} // namespace tinyray
