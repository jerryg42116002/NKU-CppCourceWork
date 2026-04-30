#pragma once

#include "core/HitRecord.h"
#include "core/Random.h"
#include "core/Ray.h"
#include "core/Scene.h"
#include "core/Vec3.h"

namespace tinyray {

class RayTracer
{
public:
    Vec3 trace(const Ray& ray, const Scene& scene, int maxDepth);

    void setAmbientLight(const Vec3& ambientLight);
    void setShadowEpsilon(double shadowEpsilon);

private:
    Vec3 rayColor(const Ray& ray, const Scene& scene, int depth);
    Vec3 shadeDiffuse(const HitRecord& record, const Scene& scene) const;
    Vec3 shadeMetal(const Ray& ray, const HitRecord& record, const Scene& scene, int depth);
    Vec3 shadeGlass(const Ray& ray, const HitRecord& record, const Scene& scene, int depth);
    Vec3 backgroundColor(const Ray& ray) const;
    bool isInShadow(const Vec3& point, const Vec3& normal, const Light& light, const Scene& scene) const;

    Vec3 ambientLight_ = Vec3(0.08, 0.08, 0.09);
    double shadowEpsilon_ = 1.0e-4;
    Random random_;
};

} // namespace tinyray
