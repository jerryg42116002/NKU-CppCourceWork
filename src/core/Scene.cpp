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
    bloomSettings = other.bloomSettings;
    environment = other.environment;
    softShadowsEnabled = other.softShadowsEnabled;
    areaLightSamples = other.areaLightSamples;
    overlayLabelsEnabled = other.overlayLabelsEnabled;
    overlayShowPosition = other.overlayShowPosition;
    overlayShowMaterialInfo = other.overlayShowMaterialInfo;
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
    if (dynamic_cast<const Box*>(object)) {
        return QStringLiteral("Box #%1").arg(objectId);
    }
    if (dynamic_cast<const Cylinder*>(object)) {
        return QStringLiteral("Cylinder #%1").arg(objectId);
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

    Material purpleMaterial;
    purpleMaterial.albedo = Vec3(0.58, 0.32, 0.92);

    Material metalMaterial;
    metalMaterial.type = MaterialType::Metal;
    metalMaterial.albedo = Vec3(0.86, 0.82, 0.72);
    metalMaterial.roughness = 0.12;

    Material glassMaterial;
    glassMaterial.type = MaterialType::Glass;
    glassMaterial.albedo = Vec3(0.96, 0.98, 1.0);
    glassMaterial.refractiveIndex = 1.5;

    Material glassBoxMaterial;
    glassBoxMaterial.type = MaterialType::Glass;
    glassBoxMaterial.albedo = Vec3(0.78, 0.92, 1.0);
    glassBoxMaterial.refractiveIndex = 1.5;

    scene.addPlane(Plane(Vec3(0.0, -0.5, 0.0), Vec3(0.0, 1.0, 0.0), groundMaterial));
    scene.addSphere(Sphere(Vec3(-1.35, 0.0, 0.05), 0.5, redMaterial));
    scene.addSphere(Sphere(Vec3(-0.25, 0.0, -0.35), 0.5, metalMaterial));
    scene.addSphere(Sphere(Vec3(0.85, 0.0, -0.15), 0.5, glassMaterial));
    scene.addSphere(Sphere(Vec3(0.0, 0.75, -1.35), 0.35, blueMaterial));
    scene.addSphere(Sphere(Vec3(1.65, -0.08, 0.2), 0.42, greenMaterial));
    scene.addBox(Box(Vec3(-1.95, -0.12, -1.25), Vec3(0.65, 0.75, 0.65), purpleMaterial));
    scene.addCylinder(Cylinder(Vec3(1.85, -0.05, -1.1), 0.32, 0.9, metalMaterial));
    scene.addBox(Box(Vec3(1.95, -0.15, -1.85), Vec3(0.55, 0.68, 0.55), glassBoxMaterial));
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

    Material glassBoxMaterial;
    glassBoxMaterial.type = MaterialType::Glass;
    glassBoxMaterial.albedo = Vec3(0.78, 0.92, 1.0);
    glassBoxMaterial.refractiveIndex = 1.5;

    Material blueDiffuse;
    blueDiffuse.albedo = Vec3(0.15, 0.35, 0.90);

    Material warmDiffuse;
    warmDiffuse.albedo = Vec3(0.95, 0.55, 0.22);

    Material tealDiffuse;
    tealDiffuse.albedo = Vec3(0.18, 0.78, 0.72);

    Material metalMaterial;
    metalMaterial.type = MaterialType::Metal;
    metalMaterial.albedo = Vec3(0.88, 0.84, 0.72);
    metalMaterial.roughness = 0.10;

    scene.addPlane(Plane(Vec3(0.0, -0.5, 0.0), Vec3(0.0, 1.0, 0.0), groundMaterial));
    scene.addSphere(Sphere(Vec3(-0.65, 0.05, -0.35), 0.55, glassMaterial));
    scene.addSphere(Sphere(Vec3(0.85, -0.02, -0.65), 0.48, blueDiffuse));
    scene.addSphere(Sphere(Vec3(-1.55, -0.12, 0.45), 0.38, warmDiffuse));
    scene.addBox(Box(Vec3(1.65, -0.22, 0.38), Vec3(0.58, 0.55, 0.58), tealDiffuse));
    scene.addCylinder(Cylinder(Vec3(-1.95, -0.14, -0.85), 0.28, 0.72, warmDiffuse));
    scene.addBox(Box(Vec3(1.55, -0.16, -1.45), Vec3(0.58, 0.68, 0.58), metalMaterial));
    scene.addCylinder(Cylinder(Vec3(-1.25, -0.08, -1.55), 0.30, 0.84, glassMaterial));
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
    scene.bloomSettings.enabled = true;
    scene.bloomSettings.exposure = 1.05;
    scene.bloomSettings.threshold = 0.85;
    scene.bloomSettings.strength = 0.90;
    scene.bloomSettings.blurPassCount = 7;
    scene.softShadowsEnabled = true;
    scene.areaLightSamples = 16;
    scene.overlayLabelsEnabled = true;
    scene.overlayShowPosition = true;
    scene.overlayShowMaterialInfo = true;
    scene.environment.mode = EnvironmentMode::Gradient;
    scene.environment.topColor = Vec3(0.05, 0.08, 0.16);
    scene.environment.bottomColor = Vec3(0.42, 0.48, 0.58);
    scene.environment.solidColor = Vec3(0.04, 0.045, 0.055);
    scene.environment.exposure = 1.0;
    scene.environment.intensity = 1.0;
    scene.environment.rotationY = 0.0;

    Material groundMaterial;
    groundMaterial.albedo = Vec3(0.18, 0.19, 0.22);
    groundMaterial.useTexture = true;
    groundMaterial.textureType = TextureType::Checkerboard;
    groundMaterial.textureScale = 5.5;
    groundMaterial.textureStrength = 1.0;
    groundMaterial.fallbackColor = groundMaterial.albedo;
    groundMaterial.checkerColorA = Vec3(0.72, 0.72, 0.68);
    groundMaterial.checkerColorB = Vec3(0.12, 0.13, 0.15);

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

    Material violetMaterial;
    violetMaterial.albedo = Vec3(0.62, 0.28, 0.88);

    Material cyanGlassMaterial;
    cyanGlassMaterial.type = MaterialType::Glass;
    cyanGlassMaterial.albedo = Vec3(0.78, 0.95, 1.0);
    cyanGlassMaterial.refractiveIndex = 1.5;

    Material orangeEmissiveMaterial;
    orangeEmissiveMaterial.type = MaterialType::Emissive;
    orangeEmissiveMaterial.albedo = Vec3(1.0, 0.45, 0.12);
    orangeEmissiveMaterial.emissionColor = Vec3(1.0, 0.45, 0.12);
    orangeEmissiveMaterial.emissionStrength = 6.0;

    Material violetEmissiveMaterial;
    violetEmissiveMaterial.type = MaterialType::Emissive;
    violetEmissiveMaterial.albedo = Vec3(0.45, 0.25, 1.0);
    violetEmissiveMaterial.emissionColor = Vec3(0.45, 0.25, 1.0);
    violetEmissiveMaterial.emissionStrength = 4.5;

    scene.addPlane(Plane(Vec3(0.0, -0.5, 0.0), Vec3(0.0, 1.0, 0.0), groundMaterial));
    scene.addSphere(Sphere(Vec3(-1.35, 0.0, -0.15), 0.5, redMaterial));
    scene.addSphere(Sphere(Vec3(-0.15, 0.0, -0.55), 0.5, metalMaterial));
    scene.addSphere(Sphere(Vec3(1.05, 0.0, -0.2), 0.5, glassMaterial));
    scene.addSphere(Sphere(Vec3(0.15, 0.85, -1.45), 0.35, greenMaterial));
    scene.addSphere(Sphere(Vec3(0.0, 1.55, -0.95), 0.22, orangeEmissiveMaterial));
    scene.addBox(Box(Vec3(-2.05, -0.13, -0.95), Vec3(0.62, 0.74, 0.62), violetMaterial));
    scene.addCylinder(Cylinder(Vec3(2.0, -0.09, -1.1), 0.30, 0.82, redMaterial));
    scene.addBox(Box(Vec3(-0.95, -0.15, -1.85), Vec3(0.55, 0.70, 0.55), metalMaterial));
    scene.addCylinder(Cylinder(Vec3(1.35, -0.08, -1.75), 0.28, 0.84, cyanGlassMaterial));
    scene.addBox(Box(Vec3(2.25, 0.10, -1.85), Vec3(0.18, 1.20, 0.18), violetEmissiveMaterial));
    scene.addLight(Light(Vec3(-2.6, 2.8, 1.5), Vec3(1.0, 0.18, 0.15), 18.0));
    scene.addLight(Light(Vec3(2.6, 2.8, 1.5), Vec3(0.15, 0.35, 1.0), 18.0));
    scene.addLight(Light(Vec3(0.0, 3.6, 1.0), Vec3(0.30, 1.0, 0.45), 16.0));
    scene.addLight(Light::area(Vec3(0.0, 3.2, -0.6),
                               Vec3(0.0, -1.0, 0.15),
                               3.4,
                               1.5,
                               Vec3(1.0, 0.82, 0.55),
                               18.0));

    return scene;
}

} // namespace tinyray
