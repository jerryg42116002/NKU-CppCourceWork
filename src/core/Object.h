#pragma once

#include "core/HitRecord.h"
#include "core/Ray.h"

namespace tinyray {

class Object
{
public:
    virtual ~Object() = default;

    virtual bool intersect(const Ray& ray, double tMin, double tMax, HitRecord& record) const = 0;
};

} // namespace tinyray
