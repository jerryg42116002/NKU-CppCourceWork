#include "core/Sphere.h"

#include <cmath>

namespace tinyray {

bool Sphere::intersect(const Ray& ray, double tMin, double tMax, HitRecord& record) const
{
    const Vec3 oc = ray.origin - center;

    // Quadratic equation for |ray.origin + t * ray.direction - center|^2 = radius^2.
    const double a = ray.direction.lengthSquared();
    const double halfB = dot(oc, ray.direction);
    const double c = oc.lengthSquared() - radius * radius;
    const double discriminant = halfB * halfB - a * c;

    if (discriminant < 0.0) {
        return false;
    }

    const double sqrtDiscriminant = std::sqrt(discriminant);

    double root = (-halfB - sqrtDiscriminant) / a;
    if (root < tMin || root > tMax) {
        root = (-halfB + sqrtDiscriminant) / a;
        if (root < tMin || root > tMax) {
            return false;
        }
    }

    record.t = root;
    record.point = ray.at(record.t);
    const Vec3 outwardNormal = (record.point - center) / radius;
    record.setFaceNormal(ray, outwardNormal);
    record.material = &material;

    return true;
}

} // namespace tinyray
