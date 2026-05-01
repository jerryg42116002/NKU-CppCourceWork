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

double schlickReflectance(double cosine, double refractiveIndex)
{
    // Schlick approximation estimates how likely light is to reflect instead of refract.
    // At grazing angles the probability rises, which makes glass edges look brighter.
    double r0 = (1.0 - refractiveIndex) / (1.0 + refractiveIndex);
    r0 = r0 * r0;
    return r0 + (1.0 - r0) * std::pow(1.0 - cosine, 5.0);
}

double pseudoRandom01(int sampleIndex, double salt, const Vec3& point)
{
    const double seed = point.x * 12.9898 + point.y * 78.233 + point.z * 37.719
        + static_cast<double>(sampleIndex) * 19.19 + salt;
    const double value = std::sin(seed) * 43758.5453123;
    return value - std::floor(value);
}

} // namespace

Vec3 RayTracer::trace(const Ray& ray, const Scene& scene, int maxDepth)
{
    return rayColor(ray, scene, std::max(maxDepth, 0));
}

Vec3 RayTracer::rayColor(const Ray& ray, const Scene& scene, int depth)
{
    if (depth <= 0) {
        return Vec3(0.0, 0.0, 0.0);
    }

    HitRecord record;
    if (!scene.intersect(ray, shadowEpsilon_, infinity, record)) {
        return backgroundColor(ray);
    }

    const Material* material = record.material;
    if (!material) {
        return shadeDiffuse(record, scene);
    }

    switch (material->type) {
    case MaterialType::Emissive:
        return material->emittedRadiance();
    case MaterialType::Glass:
        return shadeGlass(ray, record, scene, depth);
    case MaterialType::Metal:
        return shadeMetal(ray, record, scene, depth);
    case MaterialType::Diffuse:
    default:
        return shadeDiffuse(record, scene);
    }
}

void RayTracer::setAmbientLight(const Vec3& ambientLight)
{
    ambientLight_ = ambientLight;
}

void RayTracer::setShadowEpsilon(double shadowEpsilon)
{
    shadowEpsilon_ = std::max(shadowEpsilon, 0.0);
}

Vec3 RayTracer::shadeDiffuse(const HitRecord& record, const Scene& scene) const
{
    const Material* material = record.material;
    const Vec3 albedo = material ? material->albedo : Vec3(0.8, 0.8, 0.8);

    Vec3 color = ambientLight_ * albedo;

    for (const Light& light : scene.lights) {
        if (light.type == LightType::Point) {
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
            continue;
        }

        if (light.type == LightType::Area) {
            const int requestedSamples = scene.softShadowsEnabled ? scene.areaLightSamples : 1;
            const int sampleCount = std::clamp(requestedSamples, 1, 128);
            Vec3 contribution(0.0, 0.0, 0.0);

            for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
                const double u = scene.softShadowsEnabled
                    ? pseudoRandom01(sampleIndex, 3.17, record.point)
                    : 0.5;
                const double v = scene.softShadowsEnabled
                    ? pseudoRandom01(sampleIndex, 9.73, record.point)
                    : 0.5;
                const Vec3 lightPoint = light.sampleArea(u, v);
                const Vec3 toLight = lightPoint - record.point;
                const double lightDistance = toLight.length();
                if (lightDistance <= shadowEpsilon_) {
                    continue;
                }

                const Vec3 lightDirection = toLight / lightDistance;
                const double lambert = std::max(0.0, dot(record.normal, lightDirection));
                if (lambert <= 0.0) {
                    continue;
                }

                const double lightFacing = std::max(0.0, dot(light.safeNormal(), -lightDirection));
                if (lightFacing <= 0.0) {
                    continue;
                }

                const Vec3 shadowOrigin = record.point + record.normal * shadowEpsilon_;
                const Ray shadowRay(shadowOrigin, lightDirection);
                HitRecord shadowRecord;
                const bool occluded = scene.intersect(
                    shadowRay, shadowEpsilon_, lightDistance - shadowEpsilon_, shadowRecord);
                if (occluded) {
                    continue;
                }

                const double distanceSquared = std::max(lightDistance * lightDistance, shadowEpsilon_);
                const double attenuation = light.intensity * lightFacing / distanceSquared;
                contribution += albedo * light.color * (lambert * attenuation);
            }

            color += contribution / static_cast<double>(sampleCount);
        }
    }

    return clamp(color, 0.0, 1.0);
}

Vec3 RayTracer::shadeMetal(const Ray& ray, const HitRecord& record, const Scene& scene, int depth)
{
    const Material* material = record.material;
    const Vec3 albedo = material ? material->albedo : Vec3(0.8, 0.8, 0.8);
    const double roughness = material ? material->clampedRoughness() : 0.0;

    const Vec3 reflected = reflect(ray.direction.normalized(), record.normal);
    const Vec3 scatteredDirection = reflected + roughness * random_.inUnitSphere();
    if (scatteredDirection.nearZero() || dot(scatteredDirection, record.normal) <= 0.0) {
        return Vec3(0.0, 0.0, 0.0);
    }

    const Vec3 scatteredOrigin = record.point + record.normal * shadowEpsilon_;
    const Ray scatteredRay(scatteredOrigin, scatteredDirection.normalized());
    return albedo * rayColor(scatteredRay, scene, depth - 1);
}

Vec3 RayTracer::shadeGlass(const Ray& ray, const HitRecord& record, const Scene& scene, int depth)
{
    const Material* material = record.material;
    const Vec3 albedo = material ? material->albedo : Vec3(1.0, 1.0, 1.0);
    const double refractiveIndex = material ? material->safeRefractiveIndex() : 1.5;

    const Vec3 unitDirection = ray.direction.normalized();
    if (unitDirection.nearZero()) {
        return Vec3(0.0, 0.0, 0.0);
    }

    // Snell's law uses eta_i / eta_t. frontFace means the ray enters glass from air.
    const double etaRatio = record.frontFace ? (1.0 / refractiveIndex) : refractiveIndex;
    const double cosTheta = std::min(dot(-unitDirection, record.normal), 1.0);
    const double sinTheta = std::sqrt(std::max(0.0, 1.0 - cosTheta * cosTheta));

    // Total internal reflection happens when Snell's law has no real refracted solution.
    const bool cannotRefract = etaRatio * sinTheta > 1.0;

    Vec3 scatteredDirection;
    if (cannotRefract || schlickReflectance(cosTheta, refractiveIndex) > random_.real()) {
        scatteredDirection = reflect(unitDirection, record.normal);
    } else {
        scatteredDirection = refract(unitDirection, record.normal, etaRatio);
    }

    if (scatteredDirection.nearZero()) {
        return Vec3(0.0, 0.0, 0.0);
    }

    const Vec3 safeDirection = scatteredDirection.normalized();
    const Vec3 scatteredOrigin = record.point + safeDirection * shadowEpsilon_;
    const Ray scatteredRay(scatteredOrigin, safeDirection);

    return albedo * rayColor(scatteredRay, scene, depth - 1);
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
