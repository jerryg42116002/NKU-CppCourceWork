#include "core/Cylinder.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace tinyray {
namespace {

bool updateHit(double candidateT,
               double tMin,
               double tMax,
               double& bestT,
               Vec3& bestNormal,
               const Vec3& normal)
{
    if (candidateT < tMin || candidateT > tMax || candidateT >= bestT) {
        return false;
    }

    bestT = candidateT;
    bestNormal = normal;
    return true;
}

} // namespace

bool Cylinder::intersect(const Ray& ray, double tMin, double tMax, HitRecord& record) const
{
    const double safeRadius = std::max(std::abs(radius), 1.0e-4);
    const double halfHeight = std::max(std::abs(height), 1.0e-4) * 0.5;
    const double minY = center.y - halfHeight;
    const double maxY = center.y + halfHeight;

    double bestT = std::numeric_limits<double>::infinity();
    Vec3 bestNormal(0.0, 1.0, 0.0);

    const Vec3 localOrigin = ray.origin - center;
    const double a = ray.direction.x * ray.direction.x + ray.direction.z * ray.direction.z;
    const double b = 2.0 * (localOrigin.x * ray.direction.x + localOrigin.z * ray.direction.z);
    const double c = localOrigin.x * localOrigin.x + localOrigin.z * localOrigin.z - safeRadius * safeRadius;

    if (std::abs(a) > 1.0e-10) {
        const double discriminant = b * b - 4.0 * a * c;
        if (discriminant >= 0.0) {
            const double root = std::sqrt(discriminant);
            const double candidates[2] = {
                (-b - root) / (2.0 * a),
                (-b + root) / (2.0 * a)
            };

            for (double t : candidates) {
                const Vec3 point = ray.at(t);
                if (point.y < minY || point.y > maxY) {
                    continue;
                }

                Vec3 normal(point.x - center.x, 0.0, point.z - center.z);
                if (normal.nearZero()) {
                    normal = Vec3(1.0, 0.0, 0.0);
                }
                updateHit(t, tMin, tMax, bestT, bestNormal, normal.normalized());
            }
        }
    }

    if (std::abs(ray.direction.y) > 1.0e-10) {
        const double capYs[2] = {minY, maxY};
        const Vec3 normals[2] = {Vec3(0.0, -1.0, 0.0), Vec3(0.0, 1.0, 0.0)};
        for (int index = 0; index < 2; ++index) {
            const double t = (capYs[index] - ray.origin.y) / ray.direction.y;
            const Vec3 point = ray.at(t);
            const double dx = point.x - center.x;
            const double dz = point.z - center.z;
            if (dx * dx + dz * dz <= safeRadius * safeRadius) {
                updateHit(t, tMin, tMax, bestT, bestNormal, normals[index]);
            }
        }
    }

    if (!std::isfinite(bestT)) {
        return false;
    }

    record.t = bestT;
    record.point = ray.at(bestT);
    record.material = &material;
    record.hitObjectId = id();
    record.setFaceNormal(ray, bestNormal);
    return true;
}

std::shared_ptr<Object> Cylinder::clone() const
{
    return std::make_shared<Cylinder>(*this);
}

} // namespace tinyray
