#pragma once

#include "core/Material.h"
#include "core/Object.h"
#include "core/Vec3.h"

namespace tinyray {

class Plane : public Object
{
public:
    Plane() = default;
    Plane(const Vec3& pointValue, const Vec3& normalValue, const Material& materialValue)
        : point(pointValue)
        , normal(normalValue.normalized())
        , material(materialValue)
    {
    }

    bool intersect(const Ray& ray, double tMin, double tMax, HitRecord& record) const override;

    Vec3 point = Vec3(0.0, 0.0, 0.0);
    Vec3 normal = Vec3(0.0, 1.0, 0.0);
    Material material;
};

} // namespace tinyray
