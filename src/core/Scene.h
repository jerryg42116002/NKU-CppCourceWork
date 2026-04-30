#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "core/Camera.h"
#include "core/HitRecord.h"
#include "core/Light.h"
#include "core/Object.h"
#include "core/Plane.h"
#include "core/Ray.h"
#include "core/Sphere.h"

namespace tinyray {

class Scene
{
public:
    void clear()
    {
        objects.clear();
        lights.clear();
    }

    void addObject(std::shared_ptr<Object> object)
    {
        objects.push_back(std::move(object));
    }

    void addSphere(const Sphere& sphere)
    {
        addObject(std::make_shared<Sphere>(sphere));
    }

    void addPlane(const Plane& plane)
    {
        addObject(std::make_shared<Plane>(plane));
    }

    void addLight(const Light& light)
    {
        lights.push_back(light);
    }

    bool intersect(const Ray& ray, double tMin, double tMax, HitRecord& record) const;

    static Scene createDefaultScene();

    Camera camera;
    std::vector<std::shared_ptr<Object>> objects;
    std::vector<Light> lights;
};

} // namespace tinyray
