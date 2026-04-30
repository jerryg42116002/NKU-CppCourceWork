#include <QApplication>
#include <QPoint>
#include <QSize>
#include <QString>

#include <cmath>
#include <iostream>
#include <memory>

#include "core/HitRecord.h"
#include "core/OrbitCamera.h"
#include "core/Box.h"
#include "core/Cylinder.h"
#include "core/Plane.h"
#include "core/Ray.h"
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
