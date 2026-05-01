#include <QApplication>
#include <QPoint>
#include <QSize>
#include <QString>

#include <cmath>
#include <iostream>
#include <memory>

#include "core/HitRecord.h"
#include "core/Light.h"
#include "core/OrbitCamera.h"
#include "core/Box.h"
#include "core/Cylinder.h"
#include "core/Plane.h"
#include "core/Ray.h"
#include "core/RayTracer.h"
#include "core/Scene.h"
#include "core/Sphere.h"
#include "interaction/Picking.h"
#include "gui/MainWindow.h"

namespace {

bool finite(const tinyray::Vec3& value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

int runSelfTest()
{
    tinyray::OrbitCamera camera;
    if (!camera.isValid() || !finite(camera.position())) {
        std::cerr << "OrbitCamera default state is invalid.\n";
        return 1;
    }

    camera.orbit(45.0, 30.0);
    if (!camera.isValid() || !finite(camera.position())) {
        std::cerr << "OrbitCamera became invalid after orbit.\n";
        return 1;
    }

    camera.zoom(0.0001);
    if (!camera.isValid() || camera.distance <= 0.0 || !finite(camera.position())) {
        std::cerr << "OrbitCamera became invalid after zoom.\n";
        return 1;
    }

    camera.pan(120.0, -80.0);
    if (!camera.isValid() || !finite(camera.target) || !finite(camera.position())) {
        std::cerr << "OrbitCamera became invalid after pan.\n";
        return 1;
    }

    const tinyray::Ray pickingRay = tinyray::makePickingRay(QPoint(400, 225),
                                                            QSize(800, 450),
                                                            camera);
    if (!finite(pickingRay.origin)
        || !finite(pickingRay.direction)
        || pickingRay.direction.nearZero()) {
        std::cerr << "Picking ray is invalid.\n";
        return 1;
    }

    tinyray::Scene pickScene;
    auto hitSphere = std::make_unique<tinyray::Sphere>();
    hitSphere->center = tinyray::Vec3(0.0, 0.0, -3.0);
    hitSphere->radius = 1.0;
    const int hitSphereId = hitSphere->id();

    auto sideSphere = std::make_unique<tinyray::Sphere>();
    sideSphere->center = tinyray::Vec3(3.0, 0.0, -3.0);
    sideSphere->radius = 1.0;
    const int sideSphereId = sideSphere->id();

    if (hitSphereId <= 0 || sideSphereId <= 0 || hitSphereId == sideSphereId) {
        std::cerr << "Object ids are not stable unique positive values.\n";
        return 1;
    }

    pickScene.addObject(std::move(hitSphere));
    pickScene.addObject(std::move(sideSphere));

    tinyray::HitRecord hitRecord;
    const tinyray::Ray hitRay(tinyray::Vec3(0.0, 0.0, 0.0),
                              tinyray::Vec3(0.0, 0.0, -1.0));
    if (!pickScene.intersect(hitRay, 0.001, 1000.0, hitRecord)
        || hitRecord.hitObjectId != hitSphereId) {
        std::cerr << "Sphere hit did not report the expected object id.\n";
        return 1;
    }

    tinyray::HitRecord missRecord;
    const tinyray::Ray missRay(tinyray::Vec3(0.0, 0.0, 0.0),
                               tinyray::Vec3(0.0, 1.0, 0.0));
    if (pickScene.intersect(missRay, 0.001, 1000.0, missRecord)
        || missRecord.hitObjectId != -1) {
        std::cerr << "Miss ray incorrectly selected an object.\n";
        return 1;
    }

    tinyray::Material testMaterial;
    tinyray::Box testBox(tinyray::Vec3(0.0, 0.0, -3.0),
                         tinyray::Vec3(1.0, 1.0, 1.0),
                         testMaterial);
    tinyray::HitRecord boxRecord;
    if (!testBox.intersect(hitRay, 0.001, 1000.0, boxRecord)
        || boxRecord.hitObjectId != testBox.id()) {
        std::cerr << "Box intersection did not report the expected object id.\n";
        return 1;
    }

    tinyray::Cylinder testCylinder(tinyray::Vec3(0.0, 0.0, -3.0),
                                   0.5,
                                   1.0,
                                   testMaterial);
    tinyray::HitRecord cylinderRecord;
    if (!testCylinder.intersect(hitRay, 0.001, 1000.0, cylinderRecord)
        || cylinderRecord.hitObjectId != testCylinder.id()) {
        std::cerr << "Cylinder intersection did not report the expected object id.\n";
        return 1;
    }

    tinyray::Plane groundPlane;
    groundPlane.point = tinyray::Vec3(0.0, 0.0, 0.0);
    groundPlane.normal = tinyray::Vec3(0.0, 1.0, 0.0);
    tinyray::HitRecord planeRecord;
    const tinyray::Ray planeRay(tinyray::Vec3(0.0, 2.0, 0.0),
                                tinyray::Vec3(0.0, -1.0, 0.0));
    if (!groundPlane.intersect(planeRay, 0.001, 1000.0, planeRecord)
        || std::abs(planeRecord.t - 2.0) > 1.0e-6
        || std::abs(planeRecord.point.y) > 1.0e-6) {
        std::cerr << "Ray-plane intersection failed.\n";
        return 1;
    }

    tinyray::Scene movingScene;
    auto movingSphere = std::make_unique<tinyray::Sphere>();
    movingSphere->center = tinyray::Vec3(0.0, 0.0, -4.0);
    movingSphere->radius = 1.0;
    const int movingSphereId = movingSphere->id();
    movingScene.addObject(std::move(movingSphere));

    const tinyray::Ray oldCenterRay(tinyray::Vec3(0.0, 0.0, 0.0),
                                    tinyray::Vec3(0.0, 0.0, -1.0));
    tinyray::HitRecord beforeMoveRecord;
    if (!movingScene.intersect(oldCenterRay, 0.001, 1000.0, beforeMoveRecord)
        || beforeMoveRecord.hitObjectId != movingSphereId) {
        std::cerr << "Moving sphere was not hittable before center update.\n";
        return 1;
    }

    tinyray::Scene sceneSnapshot = movingScene;
    auto* editedSphere = dynamic_cast<tinyray::Sphere*>(movingScene.objectById(movingSphereId));
    if (editedSphere == nullptr) {
        std::cerr << "Could not find sphere by object id.\n";
        return 1;
    }
    editedSphere->center = tinyray::Vec3(4.0, 0.0, -4.0);

    tinyray::HitRecord afterMoveOldRayRecord;
    if (movingScene.intersect(oldCenterRay, 0.001, 1000.0, afterMoveOldRayRecord)) {
        std::cerr << "Sphere center update did not change scene intersection result.\n";
        return 1;
    }

    tinyray::HitRecord snapshotRecord;
    if (!sceneSnapshot.intersect(oldCenterRay, 0.001, 1000.0, snapshotRecord)
        || snapshotRecord.hitObjectId != movingSphereId) {
        std::cerr << "Scene snapshot changed after GUI scene mutation.\n";
        return 1;
    }

    tinyray::Material weakEmissive;
    weakEmissive.type = tinyray::MaterialType::Emissive;
    weakEmissive.emissionColor = tinyray::Vec3(1.0, 0.5, 0.1);
    weakEmissive.emissionStrength = 1.5;

    tinyray::Material zeroEmissive = weakEmissive;
    zeroEmissive.emissionStrength = 0.0;

    tinyray::Material strongEmissive = weakEmissive;
    strongEmissive.emissionStrength = 6.0;

    const tinyray::Vec3 weakEmission = weakEmissive.emissionColor * weakEmissive.safeEmissionStrength();
    const tinyray::Vec3 strongEmission = strongEmissive.emissionColor * strongEmissive.safeEmissionStrength();
    if (!finite(weakEmission)
        || !finite(strongEmission)
        || weakEmission.nearZero()
        || strongEmission.lengthSquared() <= weakEmission.lengthSquared()) {
        std::cerr << "Emissive material parameters did not produce valid brightness.\n";
        return 1;
    }

    tinyray::Scene emissiveScene;
    emissiveScene.addSphere(tinyray::Sphere(tinyray::Vec3(0.0, 0.0, -3.0), 1.0, strongEmissive));
    tinyray::RayTracer emissiveTracer;
    const tinyray::Vec3 emissiveColor =
        emissiveTracer.trace(tinyray::Ray(tinyray::Vec3(0.0, 0.0, 0.0),
                                          tinyray::Vec3(0.0, 0.0, -1.0)),
                             emissiveScene,
                             4);
    if (!finite(emissiveColor) || emissiveColor.nearZero()) {
        std::cerr << "Ray hit on emissive sphere did not return visible emission.\n";
        return 1;
    }

    tinyray::Scene zeroEmissiveScene;
    zeroEmissiveScene.addSphere(tinyray::Sphere(tinyray::Vec3(0.0, 0.0, -3.0), 1.0, zeroEmissive));
    const tinyray::Vec3 zeroTraceColor =
        emissiveTracer.trace(tinyray::Ray(tinyray::Vec3(0.0, 0.0, 0.0),
                                          tinyray::Vec3(0.0, 0.0, -1.0)),
                             zeroEmissiveScene,
                             4);
    if (!finite(zeroTraceColor) || !zeroTraceColor.nearZero()) {
        std::cerr << "Zero-strength emissive material did not return black emission.\n";
        return 1;
    }

    tinyray::Scene weakEmissiveScene;
    weakEmissiveScene.addSphere(tinyray::Sphere(tinyray::Vec3(0.0, 0.0, -3.0), 1.0, weakEmissive));
    const tinyray::Vec3 weakTraceColor =
        emissiveTracer.trace(tinyray::Ray(tinyray::Vec3(0.0, 0.0, 0.0),
                                          tinyray::Vec3(0.0, 0.0, -1.0)),
                             weakEmissiveScene,
                             4);
    if (!finite(weakTraceColor) || emissiveColor.lengthSquared() <= weakTraceColor.lengthSquared()) {
        std::cerr << "Higher emissive strength did not produce a brighter traced color.\n";
        return 1;
    }

    tinyray::Scene emissiveSnapshot = emissiveScene;
    const auto* snapshotSphere = dynamic_cast<const tinyray::Sphere*>(
        emissiveSnapshot.objects.empty() ? nullptr : emissiveSnapshot.objects.front().get());
    if (!snapshotSphere
        || snapshotSphere->material.type != tinyray::MaterialType::Emissive
        || std::abs(snapshotSphere->material.emissionStrength - strongEmissive.emissionStrength) > 1.0e-9
        || !finite(snapshotSphere->material.emissionColor)) {
        std::cerr << "Scene snapshot did not preserve emissive material parameters.\n";
        return 1;
    }

    tinyray::BloomSettings bloomSettings;
    if (!std::isfinite(bloomSettings.safeThreshold())
        || !std::isfinite(bloomSettings.safeStrength())
        || bloomSettings.safeBlurPassCount() <= 0) {
        std::cerr << "Bloom settings are invalid.\n";
        return 1;
    }

    tinyray::Light areaLight = tinyray::Light::area(tinyray::Vec3(0.0, 3.0, -2.0),
                                                    tinyray::Vec3(0.0, -1.0, 0.0),
                                                    2.0,
                                                    1.0,
                                                    tinyray::Vec3(1.0, 0.8, 0.6),
                                                    12.0);
    const tinyray::Vec3 samplePoint = areaLight.sampleArea(0.25, 0.75);
    const tinyray::Vec3 localOffset = samplePoint - areaLight.position;
    if (areaLight.type != tinyray::LightType::Area
        || areaLight.width <= 0.0
        || areaLight.height <= 0.0
        || !finite(samplePoint)
        || std::abs(dot(localOffset, areaLight.tangent())) > areaLight.width * 0.5 + 1.0e-6
        || std::abs(dot(localOffset, areaLight.bitangent())) > areaLight.height * 0.5 + 1.0e-6) {
        std::cerr << "Area light sampling produced an invalid point.\n";
        return 1;
    }

    const tinyray::Vec3 sampleDirection =
        (samplePoint - tinyray::Vec3(0.0, 0.0, -2.0)).normalized();
    if (!finite(sampleDirection) || sampleDirection.nearZero()) {
        std::cerr << "Area light sample direction is invalid.\n";
        return 1;
    }

    tinyray::Scene areaLightScene;
    areaLightScene.softShadowsEnabled = true;
    areaLightScene.areaLightSamples = 8;
    tinyray::Material areaDiffuseMaterial;
    areaLightScene.addPlane(tinyray::Plane(tinyray::Vec3(0.0, -1.0, 0.0),
                                           tinyray::Vec3(0.0, 1.0, 0.0),
                                           areaDiffuseMaterial));
    areaLightScene.addLight(areaLight);
    tinyray::RayTracer areaLightTracer;
    const tinyray::Vec3 areaLightColor =
        areaLightTracer.trace(tinyray::Ray(tinyray::Vec3(0.0, 0.0, 0.0),
                                           tinyray::Vec3(0.0, -1.0, -2.0).normalized()),
                              areaLightScene,
                              4);
    if (!finite(areaLightColor)
        || areaLightColor.x < 0.0
        || areaLightColor.y < 0.0
        || areaLightColor.z < 0.0) {
        std::cerr << "Area light shading produced an invalid color.\n";
        return 1;
    }

    tinyray::Scene areaLightSnapshot = areaLightScene;
    if (areaLightSnapshot.lights.empty()
        || areaLightSnapshot.lights.front().type != tinyray::LightType::Area
        || std::abs(areaLightSnapshot.lights.front().width - areaLight.width) > 1.0e-9
        || std::abs(areaLightSnapshot.lights.front().height - areaLight.height) > 1.0e-9
        || areaLightSnapshot.areaLightSamples != areaLightScene.areaLightSamples) {
        std::cerr << "Scene snapshot did not preserve area light parameters.\n";
        return 1;
    }

    tinyray::Material diffuseMaterial;
    tinyray::Material metalMaterial;
    metalMaterial.type = tinyray::MaterialType::Metal;
    tinyray::Material glassMaterial;
    glassMaterial.type = tinyray::MaterialType::Glass;
    if (diffuseMaterial.type != tinyray::MaterialType::Diffuse
        || metalMaterial.type != tinyray::MaterialType::Metal
        || glassMaterial.type != tinyray::MaterialType::Glass) {
        std::cerr << "Existing material types were changed unexpectedly.\n";
        return 1;
    }

    std::cout << "Self-test passed.\n";
    return 0;
}

} // namespace

int main(int argc, char* argv[])
{
    for (int index = 1; index < argc; ++index) {
        if (QString::fromLocal8Bit(argv[index]) == QStringLiteral("--self-test")) {
            return runSelfTest();
        }
    }

    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}
