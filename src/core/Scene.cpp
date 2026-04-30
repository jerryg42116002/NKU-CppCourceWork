#include "core/Scene.h"

namespace tinyray {

Scene::Scene(const Scene& other)
{
    *this = other;
}

Scene& Scene::operator=(const Scene& other)
{
    if (this == &other) {
        return *this;
    }

    camera = other.camera;
    selectedObjectId = other.selectedObjectId;
    lights = other.lights;
    objects.clear();
    objects.reserve(other.objects.size());
    for (const auto& object : other.objects) {
        if (object) {
            objects.push_back(object->clone());
        }
    }

    return *this;
}

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

bool Scene::containsObjectId(int objectId) const
{
    return objectById(objectId) != nullptr;
}

Object* Scene::objectById(int objectId)
{
    for (const auto& object : objects) {
        if (object && object->id() == objectId) {
            return object.get();
        }
    }
    return nullptr;
}

const Object* Scene::objectById(int objectId) const
{
    for (const auto& object : objects) {
        if (object && object->id() == objectId) {
            return object.get();
        }
    }
    return nullptr;
}

QString Scene::objectLabel(int objectId) const
{
    const Object* object = objectById(objectId);
    if (!object) {
        return QStringLiteral("None");
    }

    if (dynamic_cast<const Sphere*>(object)) {
        return QStringLiteral("Sphere #%1").arg(objectId);
    }
    if (dynamic_cast<const Plane*>(object)) {
        return QStringLiteral("Plane #%1").arg(objectId);
    }
    return QStringLiteral("Object #%1").arg(objectId);
}

Scene Scene::createDefaultScene()
{
    return createColoredLightsDemo();
}

Scene Scene::createReflectionDemo()
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

    Material metalMaterial;
    metalMaterial.type = MaterialType::Metal;
    metalMaterial.albedo = Vec3(0.86, 0.82, 0.72);
    metalMaterial.roughness = 0.12;

    Material glassMaterial;
    glassMaterial.type = MaterialType::Glass;
    glassMaterial.albedo = Vec3(0.96, 0.98, 1.0);
    glassMaterial.refractiveIndex = 1.5;

    scene.addPlane(Plane(Vec3(0.0, -0.5, 0.0), Vec3(0.0, 1.0, 0.0), groundMaterial));
    scene.addSphere(Sphere(Vec3(-1.35, 0.0, 0.05), 0.5, redMaterial));
    scene.addSphere(Sphere(Vec3(-0.25, 0.0, -0.35), 0.5, metalMaterial));
    scene.addSphere(Sphere(Vec3(0.85, 0.0, -0.15), 0.5, glassMaterial));
    scene.addSphere(Sphere(Vec3(0.0, 0.75, -1.35), 0.35, blueMaterial));
    scene.addSphere(Sphere(Vec3(1.65, -0.08, 0.2), 0.42, greenMaterial));
    scene.addLight(Light(Vec3(2.5, 4.0, 2.5), Vec3(1.0, 0.96, 0.88), 24.0));

    return scene;
}

Scene Scene::createGlassDemo()
{
    Scene scene;
    scene.camera.position = Vec3(0.0, 1.4, 4.8);
    scene.camera.lookAt = Vec3(0.0, 0.15, -0.25);
    scene.camera.up = Vec3(0.0, 1.0, 0.0);
    scene.camera.fieldOfViewYDegrees = 40.0;

    Material groundMaterial;
    groundMaterial.albedo = Vec3(0.48, 0.52, 0.58);

    Material glassMaterial;
    glassMaterial.type = MaterialType::Glass;
    glassMaterial.albedo = Vec3(0.96, 0.98, 1.0);
    glassMaterial.refractiveIndex = 1.5;

    Material blueDiffuse;
    blueDiffuse.albedo = Vec3(0.15, 0.35, 0.90);

    Material warmDiffuse;
    warmDiffuse.albedo = Vec3(0.95, 0.55, 0.22);

    scene.addPlane(Plane(Vec3(0.0, -0.5, 0.0), Vec3(0.0, 1.0, 0.0), groundMaterial));
    scene.addSphere(Sphere(Vec3(-0.65, 0.05, -0.35), 0.55, glassMaterial));
    scene.addSphere(Sphere(Vec3(0.85, -0.02, -0.65), 0.48, blueDiffuse));
    scene.addSphere(Sphere(Vec3(-1.55, -0.12, 0.45), 0.38, warmDiffuse));
    scene.addLight(Light(Vec3(-2.8, 3.4, 1.8), Vec3(0.75, 0.86, 1.0), 20.0));
    scene.addLight(Light(Vec3(2.6, 3.0, 2.4), Vec3(1.0, 0.78, 0.52), 18.0));

    return scene;
}

Scene Scene::createColoredLightsDemo()
{
    Scene scene;
    scene.camera.position = Vec3(0.0, 1.35, 5.2);
    scene.camera.lookAt = Vec3(0.0, 0.1, -0.35);
    scene.camera.up = Vec3(0.0, 1.0, 0.0);
    scene.camera.fieldOfViewYDegrees = 42.0;

    Material groundMaterial;
    groundMaterial.albedo = Vec3(0.35, 0.36, 0.40);

    Material redMaterial;
    redMaterial.albedo = Vec3(0.95, 0.20, 0.18);

    Material greenMaterial;
    greenMaterial.albedo = Vec3(0.20, 0.85, 0.34);

    Material metalMaterial;
    metalMaterial.type = MaterialType::Metal;
    metalMaterial.albedo = Vec3(0.88, 0.84, 0.74);
    metalMaterial.roughness = 0.08;

    Material glassMaterial;
    glassMaterial.type = MaterialType::Glass;
    glassMaterial.albedo = Vec3(0.92, 0.98, 1.0);
    glassMaterial.refractiveIndex = 1.5;

    scene.addPlane(Plane(Vec3(0.0, -0.5, 0.0), Vec3(0.0, 1.0, 0.0), groundMaterial));
    scene.addSphere(Sphere(Vec3(-1.35, 0.0, -0.15), 0.5, redMaterial));
    scene.addSphere(Sphere(Vec3(-0.15, 0.0, -0.55), 0.5, metalMaterial));
    scene.addSphere(Sphere(Vec3(1.05, 0.0, -0.2), 0.5, glassMaterial));
    scene.addSphere(Sphere(Vec3(0.15, 0.85, -1.45), 0.35, greenMaterial));
    scene.addLight(Light(Vec3(-2.6, 2.8, 1.5), Vec3(1.0, 0.18, 0.15), 18.0));
    scene.addLight(Light(Vec3(2.6, 2.8, 1.5), Vec3(0.15, 0.35, 1.0), 18.0));
    scene.addLight(Light(Vec3(0.0, 3.6, 1.0), Vec3(0.30, 1.0, 0.45), 16.0));

    return scene;
}

} // namespace tinyray
