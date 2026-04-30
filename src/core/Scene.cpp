#include "core/Scene.h"

namespace tinyray {

bool Scene::intersect(const Ray& ray, double tMin, double tMax, HitRecord& record) const
{
    HitRecord tempRecord;
    bool hitAnything = false;
    double closestSoFar = tMax;

    for (const auto& object : objects) {
        if (object && object->intersect(ray, tMin, closestSoFar, tempRecord)) {
            hitAnything = true;
            closestSoFar = tempRecord.t;
            record = tempRecord;
        }
    }

    return hitAnything;
}

Scene Scene::createDefaultScene()
{
    Scene scene;
    scene.camera.position = Vec3(0.0, 1.2, 4.2);
    scene.camera.lookAt = Vec3(0.0, 0.15, 0.0);
    scene.camera.up = Vec3(0.0, 1.0, 0.0);
    scene.camera.fieldOfViewYDegrees = 42.0;
    scene.camera.aspectRatio = 16.0 / 9.0;

    Material groundMaterial;
    groundMaterial.albedo = Vec3(0.55, 0.58, 0.60);

    Material redMaterial;
    redMaterial.albedo = Vec3(0.90, 0.25, 0.20);

    Material blueMaterial;
    blueMaterial.albedo = Vec3(0.20, 0.42, 0.95);

    Material greenMaterial;
    greenMaterial.albedo = Vec3(0.25, 0.75, 0.35);

    scene.addPlane(Plane(Vec3(0.0, -0.5, 0.0), Vec3(0.0, 1.0, 0.0), groundMaterial));
    scene.addSphere(Sphere(Vec3(-1.05, 0.0, 0.0), 0.5, redMaterial));
    scene.addSphere(Sphere(Vec3(0.0, 0.0, -0.25), 0.5, blueMaterial));
    scene.addSphere(Sphere(Vec3(1.05, 0.0, 0.0), 0.5, greenMaterial));
    scene.addLight(Light(Vec3(2.5, 4.0, 2.5), Vec3(1.0, 0.96, 0.88), 24.0));

    return scene;
}

} // namespace tinyray
