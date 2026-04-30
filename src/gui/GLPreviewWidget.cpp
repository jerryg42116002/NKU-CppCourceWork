#include "gui/GLPreviewWidget.h"

#include <algorithm>
#include <cmath>

#include <QMouseEvent>
#include <QWheelEvent>
#include <QtGui/qopengl.h>

#include "core/Light.h"
#include "core/HitRecord.h"
#include "core/MathUtils.h"
#include "core/Plane.h"
#include "core/Sphere.h"
#include "interaction/Picking.h"

GLPreviewWidget::GLPreviewWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setMinimumSize(420, 240);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

void GLPreviewWidget::setScene(const tinyray::Scene& scene)
{
    scene_ = scene;
    selectedObjectId_ = scene_.selectedObjectId;
    update();
}

void GLPreviewWidget::setSelectedObjectId(int objectId)
{
    selectedObjectId_ = objectId;
    scene_.selectedObjectId = objectId;
    update();
}

int GLPreviewWidget::selectedObjectId() const
{
    return selectedObjectId_;
}

const tinyray::OrbitCamera& GLPreviewWidget::orbitCamera() const
{
    return camera_;
}

void GLPreviewWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.045f, 0.050f, 0.060f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
}

void GLPreviewWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, std::max(width, 1), std::max(height, 1));
}

void GLPreviewWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setupCameraMatrices();
    setupLights();
    drawScene();
}

void GLPreviewWidget::mousePressEvent(QMouseEvent* event)
{
    pressMousePosition_ = event->position().toPoint();
    lastMousePosition_ = pressMousePosition_;
    if (event->button() == Qt::LeftButton) {
        if (beginObjectDrag(pressMousePosition_)) {
            dragMode_ = DragMode::None;
            return;
        }
        dragMode_ = DragMode::Orbit;
    } else if (event->button() == Qt::RightButton) {
        dragMode_ = DragMode::Pan;
    } else {
        dragMode_ = DragMode::None;
    }
}

void GLPreviewWidget::mouseMoveEvent(QMouseEvent* event)
{
    const QPoint currentPosition = event->position().toPoint();
    if (isDraggingObject_) {
        updateObjectDrag(currentPosition);
        lastMousePosition_ = currentPosition;
        return;
    }

    const QPoint delta = currentPosition - lastMousePosition_;
    lastMousePosition_ = currentPosition;

    if (dragMode_ == DragMode::Orbit) {
        camera_.orbit(delta.x() * 0.35, -delta.y() * 0.35);
        update();
    } else if (dragMode_ == DragMode::Pan) {
        camera_.pan(-delta.x(), delta.y());
        update();
    }
}

void GLPreviewWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && isDraggingObject_) {
        finishObjectDrag();
    } else if (event->button() == Qt::LeftButton
        && dragMode_ == DragMode::Orbit
        && isSelectionClick(event->position().toPoint())) {
        updateSelectionFromClick(event->position().toPoint());
    }

    dragMode_ = DragMode::None;
}

void GLPreviewWidget::wheelEvent(QWheelEvent* event)
{
    const int wheelDelta = event->angleDelta().y();
    if (wheelDelta == 0) {
        return;
    }

    camera_.zoom(wheelDelta > 0 ? 0.88 : 1.14);
    update();
}

void GLPreviewWidget::setupCameraMatrices()
{
    const double aspectRatio = height() > 0 ? static_cast<double>(width()) / static_cast<double>(height()) : 1.0;
    const QMatrix4x4 projection = camera_.projectionMatrix(aspectRatio);
    const QMatrix4x4 view = camera_.viewMatrix();

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(projection.constData());

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(view.constData());
}

void GLPreviewWidget::setupLights()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    tinyray::Vec3 position(3.0, 4.0, 3.0);
    tinyray::Vec3 color(1.0, 0.95, 0.85);
    if (!scene_.lights.empty()) {
        position = scene_.lights.front().position;
        color = scene_.lights.front().color;
    }

    const GLfloat lightPosition[] = {
        static_cast<GLfloat>(position.x),
        static_cast<GLfloat>(position.y),
        static_cast<GLfloat>(position.z),
        1.0f
    };
    const GLfloat diffuse[] = {
        static_cast<GLfloat>(std::clamp(color.x, 0.0, 1.0)),
        static_cast<GLfloat>(std::clamp(color.y, 0.0, 1.0)),
        static_cast<GLfloat>(std::clamp(color.z, 0.0, 1.0)),
        1.0f
    };
    const GLfloat ambient[] = {0.18f, 0.18f, 0.20f, 1.0f};

    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, diffuse);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
}

void GLPreviewWidget::drawScene()
{
    for (const auto& object : scene_.objects) {
        if (const auto* sphere = dynamic_cast<const tinyray::Sphere*>(object.get())) {
            drawSphere(*sphere);
        } else if (const auto* plane = dynamic_cast<const tinyray::Plane*>(object.get())) {
            drawPlane(*plane);
        }
    }

    for (const tinyray::Light& light : scene_.lights) {
        drawLightMarker(light);
    }
}

void GLPreviewWidget::drawSphere(const tinyray::Sphere& sphere)
{
    applyMaterial(sphere.material, sphere.id() == selectedObjectId_);

    constexpr int stacks = 14;
    constexpr int slices = 24;
    const double radius = std::max(sphere.radius, 0.01);

    for (int stack = 0; stack < stacks; ++stack) {
        const double phi0 = tinyray::pi * static_cast<double>(stack) / stacks;
        const double phi1 = tinyray::pi * static_cast<double>(stack + 1) / stacks;

        glBegin(GL_QUAD_STRIP);
        for (int slice = 0; slice <= slices; ++slice) {
            const double theta = 2.0 * tinyray::pi * static_cast<double>(slice) / slices;
            const double phis[2] = {phi0, phi1};
            for (double phi : phis) {
                const double sinPhi = std::sin(phi);
                const tinyray::Vec3 normal(
                    std::cos(theta) * sinPhi,
                    std::cos(phi),
                    std::sin(theta) * sinPhi);
                const tinyray::Vec3 point = sphere.center + normal * radius;
                glNormal3d(normal.x, normal.y, normal.z);
                glVertex3d(point.x, point.y, point.z);
            }
        }
        glEnd();
    }
}

void GLPreviewWidget::drawPlane(const tinyray::Plane& plane)
{
    applyMaterial(plane.material, plane.id() == selectedObjectId_);

    tinyray::Vec3 normal = plane.normal.normalized();
    if (normal.nearZero()) {
        normal = tinyray::Vec3(0.0, 1.0, 0.0);
    }

    tinyray::Vec3 tangent = cross(normal, tinyray::Vec3(0.0, 0.0, 1.0)).normalized();
    if (tangent.nearZero()) {
        tangent = cross(normal, tinyray::Vec3(1.0, 0.0, 0.0)).normalized();
    }
    const tinyray::Vec3 bitangent = cross(normal, tangent).normalized();
    constexpr double halfSize = 8.0;

    const tinyray::Vec3 p0 = plane.point + tangent * halfSize + bitangent * halfSize;
    const tinyray::Vec3 p1 = plane.point - tangent * halfSize + bitangent * halfSize;
    const tinyray::Vec3 p2 = plane.point - tangent * halfSize - bitangent * halfSize;
    const tinyray::Vec3 p3 = plane.point + tangent * halfSize - bitangent * halfSize;

    glBegin(GL_QUADS);
    glNormal3d(normal.x, normal.y, normal.z);
    glVertex3d(p0.x, p0.y, p0.z);
    glVertex3d(p1.x, p1.y, p1.z);
    glVertex3d(p2.x, p2.y, p2.z);
    glVertex3d(p3.x, p3.y, p3.z);
    glEnd();
}

void GLPreviewWidget::drawLightMarker(const tinyray::Light& light)
{
    glDisable(GL_LIGHTING);
    glLineWidth(2.0f);
    glColor3d(std::clamp(light.color.x, 0.0, 1.0),
              std::clamp(light.color.y, 0.0, 1.0),
              std::clamp(light.color.z, 0.0, 1.0));

    const tinyray::Vec3 p = light.position;
    constexpr double size = 0.18;

    glBegin(GL_LINES);
    glVertex3d(p.x - size, p.y, p.z);
    glVertex3d(p.x + size, p.y, p.z);
    glVertex3d(p.x, p.y - size, p.z);
    glVertex3d(p.x, p.y + size, p.z);
    glVertex3d(p.x, p.y, p.z - size);
    glVertex3d(p.x, p.y, p.z + size);
    glEnd();

    glEnable(GL_LIGHTING);
}

void GLPreviewWidget::applyMaterial(const tinyray::Material& material, bool selected)
{
    tinyray::Vec3 color = material.albedo;
    GLfloat shininess = 16.0f;
    GLfloat specular[] = {0.12f, 0.12f, 0.12f, 1.0f};
    GLfloat emission[] = {0.0f, 0.0f, 0.0f, 1.0f};

    if (material.type == tinyray::MaterialType::Metal) {
        color = color * 0.72 + tinyray::Vec3(0.22, 0.22, 0.22);
        shininess = 72.0f;
        specular[0] = specular[1] = specular[2] = 0.75f;
    } else if (material.type == tinyray::MaterialType::Glass) {
        color = tinyray::Vec3(0.65, 0.86, 1.0);
        shininess = 96.0f;
        specular[0] = specular[1] = specular[2] = 0.95f;
    }

    if (selected) {
        color = color * 0.35 + tinyray::Vec3(1.0, 0.78, 0.12) * 0.65;
        shininess = std::max(shininess, 88.0f);
        specular[0] = specular[1] = 1.0f;
        specular[2] = 0.35f;
        emission[0] = 0.20f;
        emission[1] = 0.13f;
        emission[2] = 0.01f;
    }

    const GLfloat diffuse[] = {
        static_cast<GLfloat>(std::clamp(color.x, 0.0, 1.0)),
        static_cast<GLfloat>(std::clamp(color.y, 0.0, 1.0)),
        static_cast<GLfloat>(std::clamp(color.z, 0.0, 1.0)),
        1.0f
    };
    const GLfloat ambient[] = {
        diffuse[0] * 0.32f,
        diffuse[1] * 0.32f,
        diffuse[2] * 0.32f,
        1.0f
    };

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
}

bool GLPreviewWidget::isSelectionClick(const QPoint& releasePosition) const
{
    constexpr int clickThresholdPixels = 4;
    return (releasePosition - pressMousePosition_).manhattanLength() <= clickThresholdPixels;
}

void GLPreviewWidget::updateSelectionFromClick(const QPoint& screenPosition)
{
    const int objectId = tinyray::pickObjectId(screenPosition, size(), camera_, scene_);
    if (selectedObjectId_ == objectId) {
        return;
    }

    selectedObjectId_ = objectId;
    scene_.selectedObjectId = objectId;
    emit objectSelected(objectId);
    update();
}

bool GLPreviewWidget::beginObjectDrag(const QPoint& screenPosition)
{
    const tinyray::Ray ray = tinyray::makePickingRay(screenPosition, size(), camera_);
    tinyray::HitRecord record;
    if (!scene_.intersect(ray, 0.001, tinyray::infinity, record)) {
        return false;
    }

    auto* sphere = dynamic_cast<tinyray::Sphere*>(scene_.objectById(record.hitObjectId));
    if (sphere == nullptr) {
        return false;
    }

    tinyray::Vec3 groundPoint;
    if (!rayIntersectsGroundPlane(ray, groundPoint)) {
        return false;
    }

    selectedObjectId_ = sphere->id();
    scene_.selectedObjectId = selectedObjectId_;
    draggedObjectId_ = selectedObjectId_;
    isDraggingObject_ = true;
    dragStartObjectCenter_ = sphere->center;
    dragStartGroundPoint_ = groundPoint;
    dragOffset_ = tinyray::Vec3(dragStartObjectCenter_.x - dragStartGroundPoint_.x,
                                0.0,
                                dragStartObjectCenter_.z - dragStartGroundPoint_.z);

    emit objectSelected(selectedObjectId_);
    update();
    return true;
}

void GLPreviewWidget::updateObjectDrag(const QPoint& screenPosition)
{
    if (!isDraggingObject_) {
        return;
    }

    auto* sphere = dynamic_cast<tinyray::Sphere*>(scene_.objectById(draggedObjectId_));
    if (sphere == nullptr) {
        finishObjectDrag();
        return;
    }

    tinyray::Vec3 groundPoint;
    const tinyray::Ray ray = tinyray::makePickingRay(screenPosition, size(), camera_);
    if (!rayIntersectsGroundPlane(ray, groundPoint)) {
        return;
    }

    sphere->center = tinyray::Vec3(groundPoint.x + dragOffset_.x,
                                   dragStartObjectCenter_.y,
                                   groundPoint.z + dragOffset_.z);
    scene_.selectedObjectId = sphere->id();

    emit sphereMoved(sphere->id(), sphere->center.x, sphere->center.y, sphere->center.z);
    update();
}

void GLPreviewWidget::finishObjectDrag()
{
    isDraggingObject_ = false;
    draggedObjectId_ = -1;
}

bool GLPreviewWidget::rayIntersectsGroundPlane(const tinyray::Ray& ray, tinyray::Vec3& hitPoint) const
{
    constexpr double groundY = 0.0;
    constexpr double epsilon = 1.0e-8;

    if (std::abs(ray.direction.y) < epsilon) {
        return false;
    }

    const double t = (groundY - ray.origin.y) / ray.direction.y;
    if (!std::isfinite(t) || t <= epsilon) {
        return false;
    }

    hitPoint = ray.at(t);
    return std::isfinite(hitPoint.x) && std::isfinite(hitPoint.y) && std::isfinite(hitPoint.z);
}
