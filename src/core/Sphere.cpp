#include "core/Sphere.h"

#include <algorithm>

#include <cmath>

#include "core/MathUtils.h"

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
    const double theta = std::acos(std::clamp(-outwardNormal.y, -1.0, 1.0));
    const double phi = std::atan2(-outwardNormal.z, outwardNormal.x) + pi;
    record.u = phi / (2.0 * pi);
    record.v = theta / pi;
    record.setFaceNormal(ray, outwardNormal);
    record.material = &material;
    record.hitObjectId = id();

    return true;
}

std::shared_ptr<Object> Sphere::clone() const
{
    return std::make_shared<Sphere>(*this);
}

} // namespace tinyray
