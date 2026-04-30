#include "core/Plane.h"

#include <cmath>

namespace tinyray {

bool Plane::intersect(const Ray& ray, double tMin, double tMax, HitRecord& record) const
{
    const double denominator = dot(normal, ray.direction);
    if (std::abs(denominator) < 1.0e-8) {
        return false;
    }

    const double root = dot(point - ray.origin, normal) / denominator;
    if (root < tMin || root > tMax) {
        return false;
    }

    record.t = root;
    record.point = ray.at(record.t);
    record.setFaceNormal(ray, normal);
    record.material = &material;

    return true;
}

} // namespace tinyray
