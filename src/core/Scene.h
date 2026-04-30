#pragma once

#include <memory>
#include <utility>
#include <vector>

#include <QString>

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
    Scene() = default;
    Scene(const Scene& other);
    Scene& operator=(const Scene& other);

    void clear()
    {
        objects.clear();
        lights.clear();
    }

    void addObject(std::shared_ptr<Object> object)
    {
        if (!object) {
            return;
        }
        if (object->id() <= 0 || containsObjectId(object->id())) {
            object->setId(Object::createId());
        }
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
    bool containsObjectId(int objectId) const;
    Object* objectById(int objectId);
    const Object* objectById(int objectId) const;
    QString objectLabel(int objectId) const;

    static Scene createDefaultScene();
    static Scene createReflectionDemo();
    static Scene createGlassDemo();
    static Scene createColoredLightsDemo();

    Camera camera;
    int selectedObjectId = -1;
    std::vector<std::shared_ptr<Object>> objects;
    std::vector<Light> lights;
};

} // namespace tinyray
