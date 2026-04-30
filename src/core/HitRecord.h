#pragma once

#include "core/Material.h"
#include "core/Ray.h"
#include "core/Vec3.h"

namespace tinyray {

class HitRecord
{
public:
    Vec3 point;
    Vec3 normal;
    double t = 0.0;
    const Material* material = nullptr;
    bool frontFace = true;

    void setFaceNormal(const Ray& ray, const Vec3& outwardNormal)
    {
        // Normals always point against the incoming ray. frontFace keeps the side information.
        frontFace = dot(ray.direction, outwardNormal) < 0.0;
        normal = frontFace ? outwardNormal : -outwardNormal;
    }
};

} // namespace tinyray
