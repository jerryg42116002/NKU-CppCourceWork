#pragma once

#include <algorithm>
#include <cmath>

namespace tinyray {

class Vec3
{
public:
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    constexpr Vec3() = default;
    constexpr Vec3(double xValue, double yValue, double zValue)
        : x(xValue)
        , y(yValue)
        , z(zValue)
    {
    }

    constexpr double operator[](int index) const
    {
        return index == 0 ? x : (index == 1 ? y : z);
    }

    constexpr double& operator[](int index)
    {
        return index == 0 ? x : (index == 1 ? y : z);
    }

    constexpr Vec3 operator-() const
    {
        return Vec3(-x, -y, -z);
    }

    constexpr Vec3& operator+=(const Vec3& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    constexpr Vec3& operator-=(const Vec3& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    constexpr Vec3& operator*=(const Vec3& other)
    {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        return *this;
    }

    constexpr Vec3& operator*=(double scalar)
    {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    constexpr Vec3& operator/=(double scalar)
    {
        return *this *= (1.0 / scalar);
    }

    constexpr double lengthSquared() const
    {
        return x * x + y * y + z * z;
    }

    double length() const
    {
        return std::sqrt(lengthSquared());
    }

    // Returns a unit-length vector. Zero vectors stay zero to avoid NaN values.
    Vec3 normalized() const
    {
        const double len = length();
        if (len == 0.0) {
            return Vec3();
        }
        return Vec3(x / len, y / len, z / len);
    }

    bool nearZero() const
    {
        constexpr double epsilon = 1.0e-8;
        return std::abs(x) < epsilon && std::abs(y) < epsilon && std::abs(z) < epsilon;
    }
};

constexpr Vec3 operator+(Vec3 left, const Vec3& right)
{
    left += right;
    return left;
}

constexpr Vec3 operator-(Vec3 left, const Vec3& right)
{
    left -= right;
    return left;
}

constexpr Vec3 operator*(Vec3 left, const Vec3& right)
{
    left *= right;
    return left;
}

constexpr Vec3 operator*(Vec3 vector, double scalar)
{
    vector *= scalar;
    return vector;
}

constexpr Vec3 operator*(double scalar, Vec3 vector)
{
    vector *= scalar;
    return vector;
}

constexpr Vec3 operator/(Vec3 vector, double scalar)
{
    vector /= scalar;
    return vector;
}

constexpr Vec3 operator/(Vec3 left, const Vec3& right)
{
    return Vec3(left.x / right.x, left.y / right.y, left.z / right.z);
}

constexpr double dot(const Vec3& left, const Vec3& right)
{
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

constexpr Vec3 cross(const Vec3& left, const Vec3& right)
{
    return Vec3(
        left.y * right.z - left.z * right.y,
        left.z * right.x - left.x * right.z,
        left.x * right.y - left.y * right.x);
}

inline Vec3 normalized(const Vec3& vector)
{
    return vector.normalized();
}

inline Vec3 reflect(const Vec3& incident, const Vec3& normal)
{
    // Reflection mirrors the incident direction around the surface normal.
    return incident - 2.0 * dot(incident, normal) * normal;
}

inline Vec3 refract(const Vec3& unitDirection, const Vec3& normal, double etaRatio)
{
    // Snell's law split into components perpendicular and parallel to the normal.
    const double cosTheta = std::min(dot(-unitDirection, normal), 1.0);
    const Vec3 perpendicular = etaRatio * (unitDirection + cosTheta * normal);
    const Vec3 parallel = -std::sqrt(std::abs(1.0 - perpendicular.lengthSquared())) * normal;
    return perpendicular + parallel;
}

inline Vec3 clamp(const Vec3& value, double minValue, double maxValue)
{
    return Vec3(
        std::clamp(value.x, minValue, maxValue),
        std::clamp(value.y, minValue, maxValue),
        std::clamp(value.z, minValue, maxValue));
}

} // namespace tinyray
