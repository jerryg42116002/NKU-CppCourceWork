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
#include "core/Texture.h"
#include "interaction/Picking.h"
#include "interaction/OverlayProjector.h"
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

    tinyray::OrbitCamera turntableCamera;
    const double originalPitch = turntableCamera.pitch;
    const double originalDistance = turntableCamera.distance;
    const double originalFov = turntableCamera.fov;
    turntableCamera.turntableEnabled = true;
    turntableCamera.turntableSpeed = 30.0;
    turntableCamera.turntableDirection = tinyray::TurntableDirection::Counterclockwise;
    const double startingYaw = turntableCamera.yaw;
    if (!turntableCamera.updateTurntable(0.5)
        || turntableCamera.yaw <= startingYaw
        || std::abs(turntableCamera.pitch - originalPitch) > 1.0e-9
        || std::abs(turntableCamera.distance - originalDistance) > 1.0e-9
        || std::abs(turntableCamera.fov - originalFov) > 1.0e-9
        || !turntableCamera.isValid()
        || !finite(turntableCamera.position())) {
        std::cerr << "Turntable update did not advance yaw safely.\n";
        return 1;
    }

    const double yawAfterCounterclockwise = turntableCamera.yaw;
    turntableCamera.turntableSpeed = 0.0;
    if (turntableCamera.updateTurntable(1.0)
        || std::abs(turntableCamera.yaw - yawAfterCounterclockwise) > 1.0e-9) {
        std::cerr << "Turntable speed zero changed yaw.\n";
        return 1;
    }

    turntableCamera.yaw = 0.0;
    turntableCamera.turntableSpeed = 20.0;
    turntableCamera.turntableDirection = tinyray::TurntableDirection::Clockwise;
    if (!turntableCamera.updateTurntable(0.5) || turntableCamera.yaw >= 0.0) {
        std::cerr << "Clockwise turntable direction did not decrease yaw.\n";
        return 1;
    }

    turntableCamera.yaw = 179.0;
    turntableCamera.turntableDirection = tinyray::TurntableDirection::Counterclockwise;
    if (!turntableCamera.updateTurntable(1.0)
        || turntableCamera.yaw < -180.0
        || turntableCamera.yaw >= 180.0
        || !turntableCamera.isValid()
        || !finite(turntableCamera.position())) {
        std::cerr << "Turntable yaw wrap produced an invalid camera.\n";
        return 1;
    }

    const tinyray::Vec3 sceneCenter(1.0, 0.5, -2.0);
    const tinyray::Vec3 selectedCenter(3.0, 1.0, -4.0);
    turntableCamera.turntableTargetMode = tinyray::TurntableTargetMode::SelectedObject;
    const tinyray::Vec3 fallbackTarget = turntableCamera.resolvedTurntableTarget(sceneCenter, nullptr);
    const tinyray::Vec3 selectedTarget = turntableCamera.resolvedTurntableTarget(sceneCenter, &selectedCenter);
    if (!finite(fallbackTarget)
        || std::abs(fallbackTarget.x - sceneCenter.x) > 1.0e-9
        || std::abs(fallbackTarget.y - sceneCenter.y) > 1.0e-9
        || std::abs(fallbackTarget.z - sceneCenter.z) > 1.0e-9
        || !finite(selectedTarget)
        || std::abs(selectedTarget.x - selectedCenter.x) > 1.0e-9
        || std::abs(selectedTarget.y - selectedCenter.y) > 1.0e-9
        || std::abs(selectedTarget.z - selectedCenter.z) > 1.0e-9) {
        std::cerr << "Turntable target mode did not safely resolve selected-object fallback.\n";
        return 1;
    }

    tinyray::OrbitCamera overlayCamera;
    const tinyray::ScreenProjection overlayProjection =
        tinyray::worldToScreen(overlayCamera.target, QSize(800, 450), overlayCamera);
    if (!overlayProjection.visible
        || !std::isfinite(overlayProjection.position.x())
        || !std::isfinite(overlayProjection.position.y())) {
        std::cerr << "Overlay world-to-screen projection failed for a visible point.\n";
        return 1;
    }

    const tinyray::Vec3 cameraPosition = overlayCamera.position();
    const tinyray::Vec3 behindPoint = cameraPosition - (overlayCamera.target - cameraPosition);
    if (tinyray::worldToScreen(behindPoint, QSize(800, 450), overlayCamera).visible) {
        std::cerr << "Overlay projector reported a point behind the camera as visible.\n";
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

    if (pickScene.objectLabel(hitSphereId).isEmpty()
        || pickScene.objectLabel(-12345).isEmpty()) {
        std::cerr << "Overlay object labels are empty or unsafe for missing selection.\n";
        return 1;
    }

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
    if (std::abs(dynamic_cast<tinyray::Sphere*>(movingScene.objectById(movingSphereId))->center.x - 4.0) > 1.0e-9) {
        std::cerr << "Overlay data source did not reflect object position changes.\n";
        return 1;
    }

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

    tinyray::Environment environment;
    if (!finite(environment.sample(tinyray::Vec3(0.0, 1.0, 0.0)))) {
        std::cerr << "Default environment returned an invalid color.\n";
        return 1;
    }

    environment.mode = tinyray::EnvironmentMode::SolidColor;
    environment.solidColor = tinyray::Vec3(0.2, 0.3, 0.4);
    const tinyray::Vec3 solidEnvironmentColor = environment.sample(tinyray::Vec3(1.0, 0.0, 0.0));
    if (!finite(solidEnvironmentColor)
        || std::abs(solidEnvironmentColor.x - 0.2) > 1.0e-9
        || std::abs(solidEnvironmentColor.y - 0.3) > 1.0e-9
        || std::abs(solidEnvironmentColor.z - 0.4) > 1.0e-9) {
        std::cerr << "Solid environment did not return the configured color.\n";
        return 1;
    }

    environment.mode = tinyray::EnvironmentMode::Image;
    environment.imagePath = QStringLiteral("__missing_environment_image__.png");
    const tinyray::Vec3 fallbackEnvironmentColor = environment.sample(tinyray::Vec3(0.0, 1.0, 0.0));
    if (!finite(fallbackEnvironmentColor)) {
        std::cerr << "Missing environment image did not safely fall back.\n";
        return 1;
    }

    tinyray::Scene environmentScene;
    environmentScene.environment.mode = tinyray::EnvironmentMode::SolidColor;
    environmentScene.environment.solidColor = tinyray::Vec3(0.12, 0.23, 0.34);
    tinyray::Scene environmentSnapshot = environmentScene;
    if (environmentSnapshot.environment.mode != tinyray::EnvironmentMode::SolidColor
        || std::abs(environmentSnapshot.environment.solidColor.y - 0.23) > 1.0e-9) {
        std::cerr << "Scene snapshot did not preserve environment settings.\n";
        return 1;
    }

    tinyray::RayTracer environmentTracer;
    const tinyray::Vec3 missEnvironmentColor =
        environmentTracer.trace(tinyray::Ray(tinyray::Vec3(0.0, 0.0, 0.0),
                                             tinyray::Vec3(0.0, 1.0, 0.0)),
                                environmentScene,
                                4);
    if (!finite(missEnvironmentColor)
        || std::abs(missEnvironmentColor.x - 0.12) > 1.0e-9
        || std::abs(missEnvironmentColor.y - 0.23) > 1.0e-9
        || std::abs(missEnvironmentColor.z - 0.34) > 1.0e-9) {
        std::cerr << "CPU ray tracer did not return environment color for miss rays.\n";
        return 1;
    }

    tinyray::Texture checkerTexture;
    checkerTexture.type = tinyray::TextureType::Checkerboard;
    checkerTexture.scale = 2.0;
    checkerTexture.colorA = tinyray::Vec3(1.0, 1.0, 1.0);
    checkerTexture.colorB = tinyray::Vec3(0.0, 0.0, 0.0);
    const tinyray::Vec3 checkerA = checkerTexture.sample(0.10, 0.10, tinyray::Vec3(0.5, 0.5, 0.5));
    const tinyray::Vec3 checkerB = checkerTexture.sample(0.60, 0.10, tinyray::Vec3(0.5, 0.5, 0.5));
    if (!finite(checkerA)
        || !finite(checkerB)
        || (checkerA - checkerB).lengthSquared() <= 1.0e-9) {
        std::cerr << "Checkerboard texture did not produce alternating colors.\n";
        return 1;
    }

    tinyray::Texture missingTexture;
    missingTexture.type = tinyray::TextureType::Image;
    missingTexture.imagePath = QStringLiteral("__missing_texture_image__.png");
    const tinyray::Vec3 missingTextureColor =
        missingTexture.sample(0.25, 0.75, tinyray::Vec3(0.25, 0.50, 0.75));
    if (!finite(missingTextureColor)
        || std::abs(missingTextureColor.y - 0.50) > 1.0e-9) {
        std::cerr << "Missing image texture did not safely return fallback color.\n";
        return 1;
    }

    tinyray::Sphere uvSphere(tinyray::Vec3(0.0, 0.0, -3.0), 1.0, tinyray::Material());
    tinyray::HitRecord sphereUvRecord;
    if (!uvSphere.intersect(tinyray::Ray(tinyray::Vec3(0.0, 0.0, 0.0),
                                         tinyray::Vec3(0.0, 0.0, -1.0)),
                            0.001,
                            1000.0,
                            sphereUvRecord)
        || sphereUvRecord.u < 0.0 || sphereUvRecord.u > 1.0
        || sphereUvRecord.v < 0.0 || sphereUvRecord.v > 1.0) {
        std::cerr << "Sphere intersection did not produce legal UV coordinates.\n";
        return 1;
    }

    tinyray::Plane uvPlane(tinyray::Vec3(0.0, -1.0, 0.0),
                           tinyray::Vec3(0.0, 1.0, 0.0),
                           tinyray::Material());
    tinyray::HitRecord planeUvRecord;
    if (!uvPlane.intersect(tinyray::Ray(tinyray::Vec3(0.25, 0.0, -0.75),
                                        tinyray::Vec3(0.0, -1.0, 0.0)),
                           0.001,
                           1000.0,
                           planeUvRecord)
        || !std::isfinite(planeUvRecord.u)
        || !std::isfinite(planeUvRecord.v)) {
        std::cerr << "Plane intersection did not produce finite UV coordinates.\n";
        return 1;
    }

    tinyray::Material texturedMaterial;
    texturedMaterial.useTexture = true;
    texturedMaterial.textureType = tinyray::TextureType::Checkerboard;
    texturedMaterial.textureScale = 2.0;
    texturedMaterial.textureStrength = 1.0;
    texturedMaterial.checkerColorA = tinyray::Vec3(1.0, 1.0, 1.0);
    texturedMaterial.checkerColorB = tinyray::Vec3(0.0, 0.0, 0.0);
    if ((texturedMaterial.baseColorAt(0.10, 0.10) - texturedMaterial.baseColorAt(0.60, 0.10)).lengthSquared() <= 1.0e-9) {
        std::cerr << "Textured material did not use checkerboard sample color.\n";
        return 1;
    }

    tinyray::Scene texturedScene;
    texturedScene.addPlane(tinyray::Plane(tinyray::Vec3(0.0, -1.0, 0.0),
                                          tinyray::Vec3(0.0, 1.0, 0.0),
                                          texturedMaterial));
    tinyray::Scene texturedSnapshot = texturedScene;
    const auto* texturedPlane = dynamic_cast<const tinyray::Plane*>(
        texturedSnapshot.objects.empty() ? nullptr : texturedSnapshot.objects.front().get());
    if (!texturedPlane
        || !texturedPlane->material.useTexture
        || texturedPlane->material.textureType != tinyray::TextureType::Checkerboard
        || std::abs(texturedPlane->material.textureScale - texturedMaterial.textureScale) > 1.0e-9) {
        std::cerr << "Scene snapshot did not preserve texture parameters.\n";
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
