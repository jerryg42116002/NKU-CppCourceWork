#pragma once

#include "core/Material.h"
#include "core/Object.h"
#include "core/Vec3.h"

namespace tinyray {

class Cylinder : public Object
{
public:
    Cylinder() = default;
    Cylinder(const Vec3& centerValue, double radiusValue, double heightValue, const Material& materialValue)
        : center(centerValue)
        , radius(radiusValue)
        , height(heightValue)
        , material(materialValue)
    {
    }

    bool intersect(const Ray& ray, double tMin, double tMax, HitRecord& record) const override;
    std::shared_ptr<Object> clone() const override;

    Vec3 center = Vec3(0.0, 0.0, 0.0);
    double radius = 0.5;
    double height = 1.0;
    Material material;
};

} // namespace tinyray
