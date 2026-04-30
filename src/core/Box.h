#pragma once

#include "core/Material.h"
#include "core/Object.h"
#include "core/Vec3.h"

namespace tinyray {

class Box : public Object
{
public:
    Box() = default;
    Box(const Vec3& centerValue, const Vec3& sizeValue, const Material& materialValue)
        : center(centerValue)
        , size(sizeValue)
        , material(materialValue)
    {
    }

    bool intersect(const Ray& ray, double tMin, double tMax, HitRecord& record) const override;
    std::shared_ptr<Object> clone() const override;

    Vec3 minCorner() const;
    Vec3 maxCorner() const;

    Vec3 center = Vec3(0.0, 0.0, 0.0);
    Vec3 size = Vec3(1.0, 1.0, 1.0);
    Material material;
};

} // namespace tinyray
