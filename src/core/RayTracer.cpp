#include "core/RayTracer.h"

#include <algorithm>
#include <cmath>

#include "core/MathUtils.h"

namespace tinyray {

namespace {

Vec3 lerp(const Vec3& start, const Vec3& end, double t)
{
    return (1.0 - t) * start + t * end;
}

} // namespace

Vec3 RayTracer::trace(const Ray& ray, const Scene& scene) const
{
    HitRecord record;
    if (!scene.intersect(ray, shadowEpsilon_, infinity, record)) {
        return backgroundColor(ray);
    }

    return shadeHit(ray, record, scene);
}

void RayTracer::setAmbientLight(const Vec3& ambientLight)
{
    ambientLight_ = ambientLight;
}

void RayTracer::setShadowEpsilon(double shadowEpsilon)
{
    shadowEpsilon_ = std::max(shadowEpsilon, 0.0);
}

Vec3 RayTracer::shadeHit(const Ray& ray, const HitRecord& record, const Scene& scene) const
{
    (void)ray;

    const Material* material = record.material;
    const Vec3 albedo = material ? material->albedo : Vec3(0.8, 0.8, 0.8);

    Vec3 color = ambientLight_ * albedo;

    if (!material || material->type != MaterialType::Diffuse) {
        return color;
    }

    for (const Light& light : scene.lights) {
        if (light.type != LightType::Point) {
            continue;
        }

        if (isInShadow(record.point, record.normal, light, scene)) {
            continue;
        }

        const Vec3 toLight = light.position - record.point;
        const double distanceSquared = std::max(toLight.lengthSquared(), shadowEpsilon_);
        const Vec3 lightDirection = toLight.normalized();
        const double lambert = std::max(0.0, dot(record.normal, lightDirection));

        // Simple inverse-square attenuation keeps point lights physically plausible.
        const double attenuation = light.intensity / distanceSquared;
        color += albedo * light.color * (lambert * attenuation);
    }

    return clamp(color, 0.0, 1.0);
}

Vec3 RayTracer::backgroundColor(const Ray& ray) const
{
    const Vec3 unitDirection = ray.direction.normalized();
    const double t = 0.5 * (unitDirection.y + 1.0);
    return lerp(Vec3(0.92, 0.95, 1.0), Vec3(0.05, 0.07, 0.12), t);
}

bool RayTracer::isInShadow(const Vec3& point, const Vec3& normal, const Light& light, const Scene& scene) const
{
    const Vec3 toLight = light.position - point;
    const double lightDistance = toLight.length();
    if (lightDistance <= shadowEpsilon_) {
        return false;
    }

    const Vec3 lightDirection = toLight / lightDistance;
    const Vec3 shadowOrigin = point + normal * shadowEpsilon_;
    const Ray shadowRay(shadowOrigin, lightDirection);

    HitRecord shadowRecord;
    return scene.intersect(shadowRay, shadowEpsilon_, lightDistance - shadowEpsilon_, shadowRecord);
}

} // namespace tinyray
