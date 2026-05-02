#include "gui/GLPreviewWidget.h"

#include <algorithm>
#include <cmath>

#include <QOpenGLFramebufferObjectFormat>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QSize>
#include <QStringList>
#include <QVector2D>
#include <QVector3D>
#include <QWheelEvent>
#include <QtGui/qopengl.h>

#include "core/Light.h"
#include "core/HitRecord.h"
#include "core/Box.h"
#include "core/Cylinder.h"
#include "core/MathUtils.h"
#include "core/Object.h"
#include "core/Plane.h"
#include "core/Sphere.h"
#include "interaction/Picking.h"
#include "interaction/OverlayProjector.h"
#include "preview/BloomPass.h"
#include "preview/SkyboxRenderer.h"

namespace {

QVector3D toQVector3D(const tinyray::Vec3& value)
{
    return QVector3D(static_cast<float>(value.x),
                     static_cast<float>(value.y),
                     static_cast<float>(value.z));
}

int materialTypeIndex(tinyray::MaterialType type)
{
    switch (type) {
    case tinyray::MaterialType::Metal:
        return 1;
    case tinyray::MaterialType::Glass:
        return 2;
    case tinyray::MaterialType::Emissive:
        return 3;
    case tinyray::MaterialType::Diffuse:
    default:
        return 0;
    }
}

int environmentModeIndex(tinyray::EnvironmentMode mode)
{
    switch (mode) {
    case tinyray::EnvironmentMode::SolidColor:
        return 1;
    case tinyray::EnvironmentMode::Image:
        return 2;
    case tinyray::EnvironmentMode::HDRImage:
        return 3;
    case tinyray::EnvironmentMode::Gradient:
    default:
        return 0;
    }
}

void setMaterialUniform(QOpenGLShaderProgram& program,
                        const QString& name,
                        const tinyray::Material& material)
{
    program.setUniformValue((name + QStringLiteral(".type")).toUtf8().constData(),
                            materialTypeIndex(material.type));
    program.setUniformValue((name + QStringLiteral(".albedo")).toUtf8().constData(),
                            toQVector3D(material.albedo));
    program.setUniformValue((name + QStringLiteral(".roughness")).toUtf8().constData(),
                            static_cast<float>(material.clampedRoughness()));
    program.setUniformValue((name + QStringLiteral(".refractiveIndex")).toUtf8().constData(),
                            static_cast<float>(material.safeRefractiveIndex()));
    program.setUniformValue((name + QStringLiteral(".emissionColor")).toUtf8().constData(),
                            toQVector3D(material.emissionColor));
    program.setUniformValue((name + QStringLiteral(".emissionStrength")).toUtf8().constData(),
                            static_cast<float>(material.safeEmissionStrength()));
    program.setUniformValue((name + QStringLiteral(".useTexture")).toUtf8().constData(),
                            material.useTexture ? 1 : 0);
    program.setUniformValue((name + QStringLiteral(".textureType")).toUtf8().constData(),
                            material.textureType == tinyray::TextureType::Checkerboard ? 1 : 0);
    program.setUniformValue((name + QStringLiteral(".textureScale")).toUtf8().constData(),
                            static_cast<float>(material.textureScale));
    program.setUniformValue((name + QStringLiteral(".textureOffset")).toUtf8().constData(),
                            QVector2D(static_cast<float>(material.textureOffsetU),
                                      static_cast<float>(material.textureOffsetV)));
    program.setUniformValue((name + QStringLiteral(".textureRotation")).toUtf8().constData(),
                            static_cast<float>(material.textureRotation));
    program.setUniformValue((name + QStringLiteral(".textureStrength")).toUtf8().constData(),
                            static_cast<float>(material.textureStrength));
    program.setUniformValue((name + QStringLiteral(".fallbackColor")).toUtf8().constData(),
                            toQVector3D(material.fallbackColor));
    program.setUniformValue((name + QStringLiteral(".checkerColorA")).toUtf8().constData(),
                            toQVector3D(material.checkerColorA));
    program.setUniformValue((name + QStringLiteral(".checkerColorB")).toUtf8().constData(),
                            toQVector3D(material.checkerColorB));
}

QString firstMaterialTexturePath(const tinyray::Scene& scene)
{
    for (const auto& object : scene.objects) {
        if (const auto* sphere = dynamic_cast<const tinyray::Sphere*>(object.get())) {
            if (sphere->material.useTexture && sphere->material.textureType == tinyray::TextureType::Image && !sphere->material.texturePath.isEmpty()) {
                return sphere->material.texturePath;
            }
        } else if (const auto* box = dynamic_cast<const tinyray::Box*>(object.get())) {
            if (box->material.useTexture && box->material.textureType == tinyray::TextureType::Image && !box->material.texturePath.isEmpty()) {
                return box->material.texturePath;
            }
        } else if (const auto* cylinder = dynamic_cast<const tinyray::Cylinder*>(object.get())) {
            if (cylinder->material.useTexture && cylinder->material.textureType == tinyray::TextureType::Image && !cylinder->material.texturePath.isEmpty()) {
                return cylinder->material.texturePath;
            }
        } else if (const auto* plane = dynamic_cast<const tinyray::Plane*>(object.get())) {
            if (plane->material.useTexture && plane->material.textureType == tinyray::TextureType::Image && !plane->material.texturePath.isEmpty()) {
                return plane->material.texturePath;
            }
        }
    }
    return QString();
}

QString materialTypeName(tinyray::MaterialType type)
{
    switch (type) {
    case tinyray::MaterialType::Metal:
        return QStringLiteral("Metal");
    case tinyray::MaterialType::Glass:
        return QStringLiteral("Glass");
    case tinyray::MaterialType::Emissive:
        return QStringLiteral("Emissive");
    case tinyray::MaterialType::Diffuse:
    default:
        return QStringLiteral("Diffuse");
    }
}

QString vec3Text(const tinyray::Vec3& value)
{
    return QStringLiteral("(%1, %2, %3)")
        .arg(value.x, 0, 'f', 2)
        .arg(value.y, 0, 'f', 2)
        .arg(value.z, 0, 'f', 2);
}

const tinyray::Material* objectMaterial(const tinyray::Object* object)
{
    if (const auto* sphere = dynamic_cast<const tinyray::Sphere*>(object)) {
        return &sphere->material;
    }
    if (const auto* box = dynamic_cast<const tinyray::Box*>(object)) {
        return &box->material;
    }
    if (const auto* cylinder = dynamic_cast<const tinyray::Cylinder*>(object)) {
        return &cylinder->material;
    }
    if (const auto* plane = dynamic_cast<const tinyray::Plane*>(object)) {
        return &plane->material;
    }
    return nullptr;
}

tinyray::Vec3 objectOverlayPosition(const tinyray::Object* object)
{
    if (const auto* sphere = dynamic_cast<const tinyray::Sphere*>(object)) {
        return sphere->center + tinyray::Vec3(0.0, std::max(sphere->radius, 0.0) + 0.18, 0.0);
    }
    if (const auto* box = dynamic_cast<const tinyray::Box*>(object)) {
        return box->center + tinyray::Vec3(0.0, std::max(box->size.y, 0.0) * 0.5 + 0.18, 0.0);
    }
    if (const auto* cylinder = dynamic_cast<const tinyray::Cylinder*>(object)) {
        return cylinder->center + tinyray::Vec3(0.0, std::max(cylinder->height, 0.0) * 0.5 + 0.18, 0.0);
    }
    if (const auto* plane = dynamic_cast<const tinyray::Plane*>(object)) {
        tinyray::Vec3 normal = plane->normal.normalized();
        if (normal.nearZero()) {
            normal = tinyray::Vec3(0.0, 1.0, 0.0);
        }
        return plane->point + normal * 0.22;
    }
    return tinyray::Vec3(0.0, 0.0, 0.0);
}

QString objectTypeName(const tinyray::Object* object)
{
    if (dynamic_cast<const tinyray::Sphere*>(object)) {
        return QStringLiteral("Sphere");
    }
    if (dynamic_cast<const tinyray::Box*>(object)) {
        return QStringLiteral("Box");
    }
    if (dynamic_cast<const tinyray::Cylinder*>(object)) {
        return QStringLiteral("Cylinder");
    }
    if (dynamic_cast<const tinyray::Plane*>(object)) {
        return QStringLiteral("Plane");
    }
    return QStringLiteral("Object");
}

tinyray::Vec3 objectPosition(const tinyray::Object* object)
{
    if (const auto* sphere = dynamic_cast<const tinyray::Sphere*>(object)) {
        return sphere->center;
    }
    if (const auto* box = dynamic_cast<const tinyray::Box*>(object)) {
        return box->center;
    }
    if (const auto* cylinder = dynamic_cast<const tinyray::Cylinder*>(object)) {
        return cylinder->center;
    }
    if (const auto* plane = dynamic_cast<const tinyray::Plane*>(object)) {
        return plane->point;
    }
    return tinyray::Vec3(0.0, 0.0, 0.0);
}

} // namespace

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
    camera_.aperture = std::isfinite(scene_.camera.aperture)
        ? std::clamp(scene_.camera.aperture, 0.0, 5.0)
        : 0.35;
    camera_.focusDistance = std::isfinite(scene_.camera.focusDistance)
        ? std::clamp(scene_.camera.focusDistance, 0.05, 500.0)
        : std::max(camera_.distance, 0.05);
    if (selectedLightIndex_ >= static_cast<int>(scene_.lights.size())) {
        selectedLightIndex_ = -1;
    }
    updateTurntableTarget();
    resetPathTraceAccumulation();
    update();
}

void GLPreviewWidget::setSelectedObjectId(int objectId)
{
    selectedObjectId_ = objectId;
    selectedLightIndex_ = -1;
    scene_.selectedObjectId = objectId;
    updateTurntableTarget();
    resetPathTraceAccumulation();
    update();
}

void GLPreviewWidget::setObjectDragMode(tinyray::ObjectDragMode mode)
{
    objectDragMode_ = mode;
}

void GLPreviewWidget::setTurntableEnabled(bool enabled)
{
    camera_.turntableEnabled = enabled;
    turntablePausedByUserInputPending_ = false;
    if (enabled) {
        updateTurntableTarget();
    }
    resetPathTraceAccumulation();
    update();
}

bool GLPreviewWidget::turntableEnabled() const
{
    return camera_.turntableEnabled;
}

void GLPreviewWidget::setTurntableSpeed(double degreesPerSecond)
{
    camera_.turntableSpeed = std::clamp(degreesPerSecond, 0.0, 360.0);
}

double GLPreviewWidget::turntableSpeed() const
{
    return camera_.turntableSpeed;
}

void GLPreviewWidget::setTurntableDirection(tinyray::TurntableDirection direction)
{
    camera_.turntableDirection = direction;
}

void GLPreviewWidget::setTurntableTargetMode(tinyray::TurntableTargetMode mode)
{
    camera_.turntableTargetMode = mode;
    if (mode == tinyray::TurntableTargetMode::CustomTarget) {
        camera_.turntableCustomTarget = camera_.target;
    }
    updateTurntableTarget();
    resetPathTraceAccumulation();
    update();
}

tinyray::TurntableTargetMode GLPreviewWidget::turntableTargetMode() const
{
    return camera_.turntableTargetMode;
}

void GLPreviewWidget::setTurntableCustomTarget(const tinyray::Vec3& target)
{
    if (!std::isfinite(target.x) || !std::isfinite(target.y) || !std::isfinite(target.z)) {
        return;
    }
    camera_.turntableCustomTarget = target;
    if (camera_.turntableTargetMode == tinyray::TurntableTargetMode::CustomTarget) {
        updateTurntableTarget();
        resetPathTraceAccumulation();
        update();
    }
}

void GLPreviewWidget::setDepthOfField(double aperture, double focusDistance)
{
    camera_.aperture = std::isfinite(aperture) ? std::clamp(aperture, 0.0, 5.0) : 0.0;
    camera_.focusDistance = std::isfinite(focusDistance)
        ? std::clamp(focusDistance, 0.05, 500.0)
        : std::max(camera_.distance, 0.05);
    resetPathTraceAccumulation();
    update();
}

void GLPreviewWidget::advanceTurntable(double deltaTimeSeconds)
{
    if (!camera_.turntableEnabled) {
        return;
    }

    updateTurntableTarget();
    if (camera_.updateTurntable(deltaTimeSeconds)) {
        resetPathTraceAccumulation();
        update();
    }
}

void GLPreviewWidget::resetView()
{
    const double aperture = camera_.aperture;
    const double focusDistance = camera_.focusDistance;
    camera_ = tinyray::OrbitCamera();
    camera_.aperture = aperture;
    camera_.focusDistance = focusDistance;
    camera_.target = sceneCenter();
    camera_.turntableCustomTarget = camera_.target;
    resetPathTraceAccumulation();
    update();
}

bool GLPreviewWidget::focusSelectedObject()
{
    tinyray::Vec3 center;
    if (!selectedObjectCenter(center)) {
        return false;
    }

    camera_.target = center;
    camera_.turntableCustomTarget = center;
    resetPathTraceAccumulation();
    update();
    return true;
}

tinyray::Camera GLPreviewWidget::currentCameraSnapshot() const
{
    tinyray::Camera camera = scene_.camera;
    const QSize bufferSize = renderBufferSize();
    camera.position = camera_.position();
    camera.lookAt = camera_.target;
    camera.up = camera_.up;
    camera.fieldOfViewYDegrees = camera_.fov;
    camera.aperture = camera_.aperture;
    camera.focusDistance = camera_.focusDistance;
    camera.aspectRatio = bufferSize.height() > 0
        ? static_cast<double>(bufferSize.width()) / static_cast<double>(bufferSize.height())
        : camera.aspectRatio;
    return camera;
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
    initializePathTracer();
    recreateAccumulationBuffers();
}

void GLPreviewWidget::resizeGL(int width, int height)
{
    Q_UNUSED(width);
    Q_UNUSED(height);
    const QSize bufferSize = renderBufferSize();
    glViewport(0, 0, bufferSize.width(), bufferSize.height());
    recreateAccumulationBuffers();
    resetPathTraceAccumulation();
}

void GLPreviewWidget::paintGL()
{
    if (pathTracerReady_) {
        renderPathTracedScene();
        drawOverlayLabels();
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setupCameraMatrices();
    setupLights();
    drawScene();
    drawRealtimeOverlay();
    drawTransformGizmo();
    drawOverlayLabels();
}

void GLPreviewWidget::drawRealtimeOverlay()
{
}

const tinyray::Scene& GLPreviewWidget::previewScene() const
{
    return scene_;
}

const tinyray::OrbitCamera& GLPreviewWidget::previewCamera() const
{
    return camera_;
}

void GLPreviewWidget::invalidateRealtimeAccumulation()
{
    resetPathTraceAccumulation();
}

void GLPreviewWidget::mousePressEvent(QMouseEvent* event)
{
    pressMousePosition_ = event->position().toPoint();
    lastMousePosition_ = pressMousePosition_;
    if (event->button() == Qt::LeftButton) {
        if (beginGizmoDrag(pressMousePosition_)) {
            dragMode_ = DragMode::None;
            return;
        }
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
    if (isDraggingObject_ || isDraggingLight_) {
        updateObjectDrag(currentPosition);
        pauseTurntableForUserInput();
        lastMousePosition_ = currentPosition;
        return;
    }

    const QPoint delta = currentPosition - lastMousePosition_;
    lastMousePosition_ = currentPosition;

    if (dragMode_ == DragMode::Orbit) {
        camera_.orbit(delta.x() * 0.35, -delta.y() * 0.35);
        emit interactionStatusChanged(QStringLiteral("Camera orbiting"));
        resetPathTraceAccumulation();
        update();
        pauseTurntableForUserInput();
    } else if (dragMode_ == DragMode::Pan) {
        camera_.pan(-delta.x(), delta.y());
        emit interactionStatusChanged(QStringLiteral("Camera panning"));
        resetPathTraceAccumulation();
        update();
        pauseTurntableForUserInput();
    }
}

void GLPreviewWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && (isDraggingObject_ || isDraggingLight_)) {
        finishObjectDrag();
    } else if (event->button() == Qt::LeftButton
        && dragMode_ == DragMode::Orbit
        && isSelectionClick(event->position().toPoint())) {
        updateSelectionFromClick(event->position().toPoint());
    }

    dragMode_ = DragMode::None;
    if (turntablePausedByUserInputPending_) {
        emit interactionStatusChanged(QStringLiteral("Turntable paused by user input"));
        turntablePausedByUserInputPending_ = false;
    } else {
        emit interactionStatusChanged(QStringLiteral("Real-time rendering"));
    }
}

void GLPreviewWidget::wheelEvent(QWheelEvent* event)
{
    const int wheelDelta = event->angleDelta().y();
    if (wheelDelta == 0) {
        return;
    }

    camera_.zoom(wheelDelta > 0 ? 0.88 : 1.14);
    emit interactionStatusChanged(QStringLiteral("Real-time rendering"));
    resetPathTraceAccumulation();
    update();
    pauseTurntableForUserInput();
    turntablePausedByUserInputPending_ = false;
}

void GLPreviewWidget::setupCameraMatrices()
{
    const QSize bufferSize = renderBufferSize();
    const double aspectRatio = bufferSize.height() > 0
        ? static_cast<double>(bufferSize.width()) / static_cast<double>(bufferSize.height())
        : 1.0;
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

void GLPreviewWidget::initializePathTracer()
{
    static const char* vertexShaderSource = R"(
        #version 120
        attribute vec2 aPosition;
        varying vec2 vUv;

        void main()
        {
            vUv = aPosition * 0.5 + vec2(0.5);
            gl_Position = vec4(aPosition, 0.0, 1.0);
        }
    )";

    static const char* pathTraceFragmentShaderSource = R"(
        #version 120

        const int MAX_SPHERES = 16;
        const int MAX_BOXES = 16;
        const int MAX_CYLINDERS = 16;
        const int MAX_PLANES = 8;
        const int MAX_LIGHTS = 8;
        struct MaterialData {
            int type;
            vec3 albedo;
            float roughness;
            float refractiveIndex;
            vec3 emissionColor;
            float emissionStrength;
            int useTexture;
            int textureType;
            float textureScale;
            vec2 textureOffset;
            float textureRotation;
            float textureStrength;
            vec3 fallbackColor;
            vec3 checkerColorA;
            vec3 checkerColorB;
        };

        uniform sampler2D uPreviousFrame;
        uniform vec2 uResolution;
        uniform int uFrameIndex;
        uniform int uSelectedId;
        uniform int uEnvironmentMode;
        uniform int uEnvironmentHasImage;
        uniform sampler2D uEnvironmentMap;
        uniform int uMaterialImageEnabled;
        uniform sampler2D uMaterialImageMap;
        uniform vec3 uEnvironmentTopColor;
        uniform vec3 uEnvironmentBottomColor;
        uniform vec3 uEnvironmentSolidColor;
        uniform float uEnvironmentExposure;
        uniform float uEnvironmentIntensity;
        uniform float uEnvironmentRotationY;
        uniform vec3 uCameraPosition;
        uniform vec3 uCameraForward;
        uniform vec3 uCameraRight;
        uniform vec3 uCameraUp;
        uniform float uCameraFov;

        uniform int uSphereCount;
        uniform vec3 uSphereCenter[MAX_SPHERES];
        uniform float uSphereRadius[MAX_SPHERES];
        uniform int uSphereId[MAX_SPHERES];
        uniform MaterialData uSphereMaterial[MAX_SPHERES];

        uniform int uBoxCount;
        uniform vec3 uBoxMin[MAX_BOXES];
        uniform vec3 uBoxMax[MAX_BOXES];
        uniform int uBoxId[MAX_BOXES];
        uniform MaterialData uBoxMaterial[MAX_BOXES];

        uniform int uCylinderCount;
        uniform vec3 uCylinderCenter[MAX_CYLINDERS];
        uniform float uCylinderRadius[MAX_CYLINDERS];
        uniform float uCylinderHeight[MAX_CYLINDERS];
        uniform int uCylinderId[MAX_CYLINDERS];
        uniform MaterialData uCylinderMaterial[MAX_CYLINDERS];

        uniform int uPlaneCount;
        uniform vec3 uPlanePoint[MAX_PLANES];
        uniform vec3 uPlaneNormal[MAX_PLANES];
        uniform int uPlaneId[MAX_PLANES];
        uniform MaterialData uPlaneMaterial[MAX_PLANES];

        uniform int uLightCount;
        uniform vec3 uLightPosition[MAX_LIGHTS];
        uniform vec3 uLightColor[MAX_LIGHTS];
        uniform float uLightIntensity[MAX_LIGHTS];

        varying vec2 vUv;
    )"
    R"(

        struct Hit {
            float t;
            vec3 point;
            vec3 normal;
            vec2 uv;
            int objectId;
            MaterialData material;
        };

        float hash(vec3 p)
        {
            p = fract(p * 0.3183099 + vec3(0.1, 0.2, 0.3));
            p *= 17.0;
            return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
        }

        vec3 randomUnitVector(vec3 seed)
        {
            float z = hash(seed) * 2.0 - 1.0;
            float a = hash(seed + 19.17) * 6.2831853;
            float r = sqrt(max(0.0, 1.0 - z * z));
            return vec3(r * cos(a), z, r * sin(a));
        }

        vec3 rotateEnvironment(vec3 direction)
        {
            float c = cos(uEnvironmentRotationY);
            float s = sin(uEnvironmentRotationY);
            return vec3(c * direction.x - s * direction.z,
                        direction.y,
                        s * direction.x + c * direction.z);
        }

        vec3 sampleEnvironment(vec3 direction)
        {
            vec3 dir = normalize(direction);
            vec3 color;
            if (uEnvironmentMode == 1) {
                color = uEnvironmentSolidColor;
            } else if ((uEnvironmentMode == 2 || uEnvironmentMode == 3) && uEnvironmentHasImage == 1) {
                vec3 envDir = rotateEnvironment(dir);
                float u = fract(0.5 + atan(envDir.z, envDir.x) / 6.2831853);
                float v = clamp(acos(clamp(envDir.y, -1.0, 1.0)) / 3.1415926, 0.0, 1.0);
                color = texture2D(uEnvironmentMap, vec2(u, v)).rgb;
            } else {
                float t = 0.5 * (dir.y + 1.0);
                color = mix(uEnvironmentBottomColor, uEnvironmentTopColor, clamp(t, 0.0, 1.0));
            }
            return color * max(uEnvironmentIntensity, 0.0) * max(uEnvironmentExposure, 0.0);
        }

        MaterialData fallbackMaterial()
        {
            MaterialData material;
            material.type = 0;
            material.albedo = vec3(0.8);
            material.roughness = 0.2;
            material.refractiveIndex = 1.5;
            material.emissionColor = vec3(1.0, 0.6, 0.1);
            material.emissionStrength = 0.0;
            material.useTexture = 0;
            material.textureType = 0;
            material.textureScale = 1.0;
            material.textureOffset = vec2(0.0);
            material.textureRotation = 0.0;
            material.textureStrength = 0.0;
            material.fallbackColor = vec3(0.8);
            material.checkerColorA = vec3(0.78, 0.78, 0.74);
            material.checkerColorB = vec3(0.16, 0.17, 0.18);
            return material;
        }

        bool intersectSphere(vec3 ro, vec3 rd, int index, float tMin, float tMax, inout Hit hit)
        {
            vec3 oc = ro - uSphereCenter[index];
            float a = dot(rd, rd);
            float halfB = dot(oc, rd);
            float c = dot(oc, oc) - uSphereRadius[index] * uSphereRadius[index];
            float discriminant = halfB * halfB - a * c;
            if (discriminant < 0.0) {
                return false;
            }

            float root = sqrt(discriminant);
            float t = (-halfB - root) / a;
            if (t < tMin || t > tMax) {
                t = (-halfB + root) / a;
                if (t < tMin || t > tMax) {
                    return false;
                }
            }

            hit.t = t;
            hit.point = ro + rd * t;
            hit.normal = normalize(hit.point - uSphereCenter[index]);
            float theta = acos(clamp(-hit.normal.y, -1.0, 1.0));
            float phi = atan(-hit.normal.z, hit.normal.x) + 3.1415926;
            hit.uv = vec2(phi / 6.2831853, theta / 3.1415926);
            hit.objectId = uSphereId[index];
            hit.material = uSphereMaterial[index];
            return true;
        }

        bool intersectPlane(vec3 ro, vec3 rd, int index, float tMin, float tMax, inout Hit hit)
        {
            vec3 normal = normalize(uPlaneNormal[index]);
            float denom = dot(normal, rd);
            if (abs(denom) < 0.00001) {
                return false;
            }

            float t = dot(uPlanePoint[index] - ro, normal) / denom;
            if (t < tMin || t > tMax) {
                return false;
            }

            hit.t = t;
            hit.point = ro + rd * t;
            hit.normal = denom < 0.0 ? normal : -normal;
            hit.uv = hit.point.xz;
            hit.objectId = uPlaneId[index];
            hit.material = uPlaneMaterial[index];
            return true;
        }

        vec3 boxNormal(vec3 point, vec3 mn, vec3 mx)
        {
            vec3 d0 = abs(point - mn);
            vec3 d1 = abs(point - mx);
            float m = d0.x;
            vec3 n = vec3(-1.0, 0.0, 0.0);
            if (d1.x < m) { m = d1.x; n = vec3(1.0, 0.0, 0.0); }
            if (d0.y < m) { m = d0.y; n = vec3(0.0, -1.0, 0.0); }
            if (d1.y < m) { m = d1.y; n = vec3(0.0, 1.0, 0.0); }
            if (d0.z < m) { m = d0.z; n = vec3(0.0, 0.0, -1.0); }
            if (d1.z < m) { n = vec3(0.0, 0.0, 1.0); }
            return n;
        }

        bool intersectBox(vec3 ro, vec3 rd, int index, float tMin, float tMax, inout Hit hit)
        {
            vec3 invRd = 1.0 / max(abs(rd), vec3(0.000001)) * sign(rd);
            vec3 t0 = (uBoxMin[index] - ro) * invRd;
            vec3 t1 = (uBoxMax[index] - ro) * invRd;
            vec3 tsmaller = min(t0, t1);
            vec3 tbigger = max(t0, t1);
            float tNear = max(max(tsmaller.x, tsmaller.y), max(tsmaller.z, tMin));
            float tFar = min(min(tbigger.x, tbigger.y), min(tbigger.z, tMax));
            if (tNear > tFar) {
                return false;
            }

            hit.t = tNear;
            hit.point = ro + rd * tNear;
            hit.normal = boxNormal(hit.point, uBoxMin[index], uBoxMax[index]);
            hit.uv = hit.point.xz;
            if (dot(hit.normal, rd) > 0.0) {
                hit.normal = -hit.normal;
            }
            hit.objectId = uBoxId[index];
            hit.material = uBoxMaterial[index];
            return true;
        }

        bool intersectCylinder(vec3 ro, vec3 rd, int index, float tMin, float tMax, inout Hit hit)
        {
            vec3 localOrigin = ro - uCylinderCenter[index];
            float radius = max(uCylinderRadius[index], 0.001);
            float halfHeight = max(uCylinderHeight[index], 0.001) * 0.5;
            float bestT = tMax;
            vec3 bestNormal = vec3(0.0, 1.0, 0.0);
            bool found = false;

            float a = rd.x * rd.x + rd.z * rd.z;
            float b = 2.0 * (localOrigin.x * rd.x + localOrigin.z * rd.z);
            float c = localOrigin.x * localOrigin.x + localOrigin.z * localOrigin.z - radius * radius;
            float disc = b * b - 4.0 * a * c;
            if (a > 0.000001 && disc >= 0.0) {
                float root = sqrt(disc);
                float ts[2];
                ts[0] = (-b - root) / (2.0 * a);
                ts[1] = (-b + root) / (2.0 * a);
                for (int i = 0; i < 2; ++i) {
                    float t = ts[i];
                    float y = localOrigin.y + rd.y * t;
                    if (t >= tMin && t <= bestT && abs(y) <= halfHeight) {
                        bestT = t;
                        vec3 p = localOrigin + rd * t;
                        bestNormal = normalize(vec3(p.x, 0.0, p.z));
                        found = true;
                    }
                }
            }

            if (abs(rd.y) > 0.000001) {
                float ys[2];
                ys[0] = -halfHeight;
                ys[1] = halfHeight;
                for (int i = 0; i < 2; ++i) {
                    float t = (ys[i] - localOrigin.y) / rd.y;
                    vec3 p = localOrigin + rd * t;
                    if (t >= tMin && t <= bestT && p.x * p.x + p.z * p.z <= radius * radius) {
                        bestT = t;
                        bestNormal = i == 0 ? vec3(0.0, -1.0, 0.0) : vec3(0.0, 1.0, 0.0);
                        found = true;
                    }
                }
            }

            if (!found) {
                return false;
            }

            hit.t = bestT;
            hit.point = ro + rd * bestT;
            hit.normal = dot(bestNormal, rd) > 0.0 ? -bestNormal : bestNormal;
            hit.uv = vec2(atan(hit.point.z - uCylinderCenter[index].z, hit.point.x - uCylinderCenter[index].x) / 6.2831853 + 0.5,
                          (hit.point.y - (uCylinderCenter[index].y - uCylinderHeight[index] * 0.5)) / max(uCylinderHeight[index], 0.001));
            hit.objectId = uCylinderId[index];
            hit.material = uCylinderMaterial[index];
            return true;
        }

        bool intersectScene(vec3 ro, vec3 rd, float tMin, float tMax, inout Hit closestHit)
        {
            bool found = false;
            closestHit.t = tMax;
            closestHit.material = fallbackMaterial();

            for (int i = 0; i < MAX_SPHERES; ++i) {
                if (i >= uSphereCount) break;
                Hit h;
                if (intersectSphere(ro, rd, i, tMin, closestHit.t, h)) {
                    closestHit = h;
                    found = true;
                }
            }
            for (int i = 0; i < MAX_BOXES; ++i) {
                if (i >= uBoxCount) break;
                Hit h;
                if (intersectBox(ro, rd, i, tMin, closestHit.t, h)) {
                    closestHit = h;
                    found = true;
                }
            }
            for (int i = 0; i < MAX_CYLINDERS; ++i) {
                if (i >= uCylinderCount) break;
                Hit h;
                if (intersectCylinder(ro, rd, i, tMin, closestHit.t, h)) {
                    closestHit = h;
                    found = true;
                }
            }
            for (int i = 0; i < MAX_PLANES; ++i) {
                if (i >= uPlaneCount) break;
                Hit h;
                if (intersectPlane(ro, rd, i, tMin, closestHit.t, h)) {
                    closestHit = h;
                    found = true;
                }
            }

            return found;
        }
    )"
    R"(

        float schlick(float cosine, float refractiveIndex)
        {
            float r0 = (1.0 - refractiveIndex) / (1.0 + refractiveIndex);
            r0 = r0 * r0;
            return r0 + (1.0 - r0) * pow(1.0 - cosine, 5.0);
        }

        vec3 checkerTexture(vec2 uv, MaterialData material)
        {
            float safeScale = max(material.textureScale, 0.001);
            vec2 centered = uv - vec2(0.5);
            float c = cos(material.textureRotation);
            float s = sin(material.textureRotation);
            vec2 rotated = vec2(centered.x * c - centered.y * s,
                                centered.x * s + centered.y * c) + vec2(0.5);
            vec2 p = rotated + material.textureOffset;
            vec2 cell = floor(p * safeScale);
            float parity = mod(cell.x + cell.y, 2.0);
            return parity < 1.0 ? material.checkerColorA : material.checkerColorB;
        }

        MaterialData materialAtUv(MaterialData material, vec2 uv)
        {
            if (material.useTexture == 0) {
                return material;
            }

            vec3 sampled = material.fallbackColor;
            if (material.textureType == 0 && uMaterialImageEnabled == 1) {
                vec2 centered = uv - vec2(0.5);
                float c = cos(material.textureRotation);
                float s = sin(material.textureRotation);
                vec2 rotated = vec2(centered.x * c - centered.y * s,
                                    centered.x * s + centered.y * c) + vec2(0.5);
                vec2 p = rotated + material.textureOffset;
                sampled = texture2D(uMaterialImageMap, fract(p * max(material.textureScale, 0.001))).rgb;
            } else if (material.textureType == 1) {
                sampled = checkerTexture(uv, material);
            }
            float strength = clamp(material.textureStrength, 0.0, 1.0);
            material.albedo = mix(material.albedo, sampled, strength);
            return material;
        }

        vec3 directLighting(vec3 point, vec3 normal, vec3 viewDirection, MaterialData material)
        {
            vec3 color = vec3(0.03) * material.albedo;
            for (int i = 0; i < MAX_LIGHTS; ++i) {
                if (i >= uLightCount) break;
                vec3 toLight = uLightPosition[i] - point;
                float distanceToLight = length(toLight);
                vec3 lightDirection = toLight / max(distanceToLight, 0.0001);

                Hit shadowHit;
                if (intersectScene(point + normal * 0.002, lightDirection, 0.002, distanceToLight - 0.01, shadowHit)) {
                    continue;
                }

                float attenuation = uLightIntensity[i] / max(distanceToLight * distanceToLight, 0.2);
                float ndotl = max(dot(normal, lightDirection), 0.0);
                vec3 halfVector = normalize(lightDirection + viewDirection);
                float specular = pow(max(dot(normal, halfVector), 0.0), material.type == 1 ? 96.0 : 32.0);
                float specularScale = material.type == 0 ? 0.08 : 0.45;
                color += uLightColor[i] * attenuation * (material.albedo * ndotl + vec3(specular * specularScale));
            }
            return color;
        }

        vec3 tracePath(vec3 ro, vec3 rd, vec3 seed)
        {
            vec3 radiance = vec3(0.0);
            vec3 throughput = vec3(1.0);

            for (int bounce = 0; bounce < 4; ++bounce) {
                Hit hit;
                if (!intersectScene(ro, rd, 0.002, 1000.0, hit)) {
                    radiance += throughput * sampleEnvironment(rd);
                    break;
                }

                vec3 normal = normalize(hit.normal);
                if (hit.objectId == uSelectedId) {
                    radiance += throughput * vec3(0.8, 0.55, 0.08) * 0.12;
                }

                if (hit.material.type == 3) {
                    radiance += throughput * hit.material.emissionColor * max(hit.material.emissionStrength, 0.0);
                    break;
                }

                MaterialData shadedMaterial = materialAtUv(hit.material, hit.uv);
                vec3 viewDirection = normalize(-rd);
                radiance += throughput * directLighting(hit.point, normal, viewDirection, shadedMaterial);
                if (shadedMaterial.type == 1) {
                    vec3 envReflection = sampleEnvironment(reflect(rd, normal));
                    radiance += throughput * envReflection * mix(0.10, 0.42, 1.0 - clamp(shadedMaterial.roughness, 0.0, 1.0));
                } else if (shadedMaterial.type == 2) {
                    radiance += throughput * sampleEnvironment(reflect(rd, normal)) * 0.22;
                }

                if (shadedMaterial.type == 1) {
                    vec3 reflected = reflect(rd, normal);
                    float roughness = clamp(shadedMaterial.roughness, 0.0, 1.0);
                    rd = normalize(mix(reflected, reflected + randomUnitVector(seed + float(bounce) * 11.3), roughness * roughness));
                    ro = hit.point + normal * 0.003;
                    throughput *= shadedMaterial.albedo;
                } else if (shadedMaterial.type == 2) {
                    float eta = dot(rd, normal) < 0.0 ? 1.0 / max(shadedMaterial.refractiveIndex, 1.01) : max(shadedMaterial.refractiveIndex, 1.01);
                    vec3 outwardNormal = dot(rd, normal) < 0.0 ? normal : -normal;
                    float cosTheta = min(dot(-rd, outwardNormal), 1.0);
                    vec3 refracted = refract(rd, outwardNormal, eta);
                    float reflectChance = length(refracted) < 0.001 ? 1.0 : schlick(cosTheta, shadedMaterial.refractiveIndex);
                    if (hash(seed + float(bounce) * 23.7) < reflectChance) {
                        rd = normalize(reflect(rd, normal));
                        ro = hit.point + normal * 0.003;
                    } else {
                        rd = normalize(refracted);
                        ro = hit.point - outwardNormal * 0.003;
                    }
                    throughput *= mix(shadedMaterial.albedo, vec3(1.0), 0.65);
                } else {
                    vec3 target = normal + randomUnitVector(seed + float(bounce) * 7.1);
                    rd = normalize(target);
                    ro = hit.point + normal * 0.003;
                    throughput *= shadedMaterial.albedo;
                }

                if (max(max(throughput.r, throughput.g), throughput.b) < 0.03) {
                    break;
                }
            }

            return radiance;
        }

        void main()
        {
            vec2 pixel = gl_FragCoord.xy;
            vec3 seed = vec3(pixel, float(uFrameIndex) + 1.0);
            vec2 jitter = vec2(hash(seed), hash(seed + 31.7)) - vec2(0.5);
            vec2 uv = ((pixel + jitter) / uResolution) * 2.0 - vec2(1.0);
            uv.x *= uResolution.x / max(uResolution.y, 1.0);

            float focal = 1.0 / tan(radians(uCameraFov) * 0.5);
            vec3 rd = normalize(uCameraForward * focal + uCameraRight * uv.x + uCameraUp * uv.y);
            vec3 sampleColor = tracePath(uCameraPosition, rd, seed);
            sampleColor = max(sampleColor, vec3(0.0));

            vec3 previous = texture2D(uPreviousFrame, vUv).rgb;
            float frameWeight = float(uFrameIndex) / float(uFrameIndex + 1);
            vec3 accumulated = uFrameIndex == 0 ? sampleColor : mix(sampleColor, previous, frameWeight);
            gl_FragColor = vec4(accumulated, 1.0);
        }
    )";

    static const char* displayFragmentShaderSource = R"(
        #version 120
        uniform sampler2D uImage;
        uniform float uExposure;
        varying vec2 vUv;

        void main()
        {
            vec3 color = texture2D(uImage, vUv).rgb;
            color = vec3(1.0) - exp(-max(color, vec3(0.0)) * uExposure);
            color = pow(max(color, vec3(0.0)), vec3(1.0 / 2.2));
            gl_FragColor = vec4(color, 1.0);
        }
    )";

    pathTraceProgram_ = std::make_unique<QOpenGLShaderProgram>(this);
    displayProgram_ = std::make_unique<QOpenGLShaderProgram>(this);

    pathTracerReady_ =
        pathTraceProgram_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)
        && pathTraceProgram_->addShaderFromSourceCode(QOpenGLShader::Fragment, pathTraceFragmentShaderSource)
        && pathTraceProgram_->link()
        && displayProgram_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)
        && displayProgram_->addShaderFromSourceCode(QOpenGLShader::Fragment, displayFragmentShaderSource)
        && displayProgram_->link();
}

void GLPreviewWidget::recreateAccumulationBuffers()
{
    if (!context()) {
        return;
    }

    const QSize bufferSize = renderBufferSize();
    if (accumulationBuffers_[0] && accumulationBuffers_[0]->size() == bufferSize) {
        return;
    }

    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::NoAttachment);
    format.setTextureTarget(GL_TEXTURE_2D);
    format.setInternalTextureFormat(GL_RGBA16F);

    accumulationBuffers_[0] = std::make_unique<QOpenGLFramebufferObject>(bufferSize, format);
    accumulationBuffers_[1] = std::make_unique<QOpenGLFramebufferObject>(bufferSize, format);
    accumulationReadIndex_ = 0;
    resetPathTraceAccumulation();
}

QSize GLPreviewWidget::renderBufferSize() const
{
    const qreal scale = devicePixelRatioF();
    return QSize(std::max(1, qRound(static_cast<qreal>(width()) * scale)),
                 std::max(1, qRound(static_cast<qreal>(height()) * scale)));
}

void GLPreviewWidget::resetPathTraceAccumulation()
{
    pathTraceFrameIndex_ = 0;
}

void GLPreviewWidget::renderPathTracedScene()
{
    if (!accumulationBuffers_[0] || !accumulationBuffers_[1]) {
        recreateAccumulationBuffers();
    }
    if (!accumulationBuffers_[0] || !accumulationBuffers_[1]) {
        return;
    }

    const int writeIndex = 1 - accumulationReadIndex_;
    QOpenGLFramebufferObject* writeBuffer = accumulationBuffers_[writeIndex].get();
    QOpenGLFramebufferObject* readBuffer = accumulationBuffers_[accumulationReadIndex_].get();

    writeBuffer->bind();
    glViewport(0, 0, writeBuffer->width(), writeBuffer->height());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);

    pathTraceProgram_->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, readBuffer->texture());
    pathTraceProgram_->setUniformValue("uPreviousFrame", 0);
    pathTraceProgram_->setUniformValue("uResolution", QVector2D(static_cast<float>(writeBuffer->width()),
                                                                 static_cast<float>(writeBuffer->height())));
    pathTraceProgram_->setUniformValue("uFrameIndex", pathTraceFrameIndex_);
    uploadPathTraceScene(*pathTraceProgram_);
    drawFullScreenTriangle(*pathTraceProgram_);
    pathTraceProgram_->release();
    setupCameraMatrices();
    drawRealtimeOverlay();
    writeBuffer->release();

    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    const QSize defaultBufferSize = renderBufferSize();
    glViewport(0, 0, defaultBufferSize.width(), defaultBufferSize.height());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    static BloomPass bloomPass;
    const bool bloomRendered = scene_.bloomSettings.enabled
        && bloomPass.render(writeBuffer->texture(),
                            static_cast<GLuint>(defaultFramebufferObject()),
                            defaultBufferSize,
                            scene_.bloomSettings);

    if (!bloomRendered) {
        displayProgram_->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, writeBuffer->texture());
        displayProgram_->setUniformValue("uImage", 0);
        displayProgram_->setUniformValue("uExposure", static_cast<float>(scene_.bloomSettings.safeExposure()));
        drawFullScreenTriangle(*displayProgram_);
        displayProgram_->release();
    }

    setupCameraMatrices();
    drawLightMarkersOverlay();
    drawTransformGizmo();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    accumulationReadIndex_ = writeIndex;
    pathTraceFrameIndex_ = std::min(pathTraceFrameIndex_ + 1, 8192);
    if (pathTraceFrameIndex_ < 8192) {
        update();
    }
}

void GLPreviewWidget::drawLightMarkersOverlay()
{
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);

    for (int index = 0; index < static_cast<int>(scene_.lights.size()); ++index) {
        drawLightMarker(scene_.lights[static_cast<std::size_t>(index)], index == selectedLightIndex_);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void GLPreviewWidget::drawTransformGizmo()
{
    tinyray::Vec3 center;
    if (!gizmoCenter(center)) {
        return;
    }

    const double cameraDistance = std::max((center - camera_.position()).length(), 1.0);
    const double axisLength = std::clamp(cameraDistance * 0.18, 0.55, 2.2);
    struct AxisItem {
        GizmoAxis axis;
        tinyray::Vec3 direction;
        double red;
        double green;
        double blue;
    };
    const AxisItem axes[] = {
        {GizmoAxis::X, tinyray::Vec3(1.0, 0.0, 0.0), 1.0, 0.12, 0.10},
        {GizmoAxis::Y, tinyray::Vec3(0.0, 1.0, 0.0), 0.22, 0.92, 0.22},
        {GizmoAxis::Z, tinyray::Vec3(0.0, 0.0, 1.0), 0.18, 0.36, 1.0}
    };

    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glLineWidth(4.0f);
    glBegin(GL_LINES);
    for (const AxisItem& item : axes) {
        const bool active = activeGizmoAxis_ == item.axis;
        glColor3d(active ? 1.0 : item.red,
                  active ? 0.92 : item.green,
                  active ? 0.18 : item.blue);
        const tinyray::Vec3 end = center + item.direction * axisLength;
        glVertex3d(center.x, center.y, center.z);
        glVertex3d(end.x, end.y, end.z);
    }
    glEnd();

    glPointSize(11.0f);
    glBegin(GL_POINTS);
    for (const AxisItem& item : axes) {
        const bool active = activeGizmoAxis_ == item.axis;
        glColor3d(active ? 1.0 : item.red,
                  active ? 0.92 : item.green,
                  active ? 0.18 : item.blue);
        const tinyray::Vec3 end = center + item.direction * axisLength;
        glVertex3d(end.x, end.y, end.z);
    }
    glEnd();

    glLineWidth(1.0f);
    glPointSize(1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
}

void GLPreviewWidget::drawOverlayLabels()
{
    if (!scene_.overlayLabelsEnabled) {
        return;
    }

    QStringList lines;
    tinyray::Vec3 anchorWorld;
    bool hasAnchor = false;

    if (selectedObjectId_ >= 0) {
        const tinyray::Object* object = scene_.objectById(selectedObjectId_);
        if (!object) {
            return;
        }

        anchorWorld = objectOverlayPosition(object);
        hasAnchor = true;
        lines << QStringLiteral("%1 #%2").arg(objectTypeName(object)).arg(object->id());
        lines << QStringLiteral("Type: %1").arg(objectTypeName(object));
        if (scene_.overlayShowPosition) {
            lines << QStringLiteral("Pos: %1").arg(vec3Text(objectPosition(object)));
        }

        if (scene_.overlayShowMaterialInfo) {
            if (const tinyray::Material* material = objectMaterial(object)) {
                lines << QStringLiteral("Mat: %1").arg(materialTypeName(material->type));
                if (material->useTexture) {
                    lines << QStringLiteral("Tex: %1").arg(
                        material->textureType == tinyray::TextureType::Checkerboard
                            ? QStringLiteral("Checkerboard")
                            : QStringLiteral("Image"));
                }
                if (material->type == tinyray::MaterialType::Emissive) {
                    lines << QStringLiteral("Intensity: %1").arg(material->safeEmissionStrength(), 0, 'f', 2);
                }
            }
        }
    } else if (selectedLightIndex_ >= 0 && selectedLightIndex_ < static_cast<int>(scene_.lights.size())) {
        const tinyray::Light& light = scene_.lights[static_cast<std::size_t>(selectedLightIndex_)];
        anchorWorld = light.position;
        hasAnchor = true;
        lines << QStringLiteral("%1 Light #%2")
                     .arg(light.type == tinyray::LightType::Area ? QStringLiteral("Area") : QStringLiteral("Point"))
                     .arg(selectedLightIndex_ + 1);
        lines << QStringLiteral("Type: %1").arg(light.type == tinyray::LightType::Area
            ? QStringLiteral("Area Light")
            : QStringLiteral("Point Light"));
        if (scene_.overlayShowPosition) {
            lines << QStringLiteral("Pos: %1").arg(vec3Text(light.position));
        }
        if (scene_.overlayShowMaterialInfo) {
            lines << QStringLiteral("Intensity: %1").arg(light.intensity, 0, 'f', 2);
            if (light.type == tinyray::LightType::Area) {
                lines << QStringLiteral("Size: %1 x %2")
                             .arg(light.width, 0, 'f', 2)
                             .arg(light.height, 0, 'f', 2);
            }
        }
    }

    if (!hasAnchor || lines.isEmpty()) {
        return;
    }

    const tinyray::ScreenProjection projected = tinyray::worldToScreen(anchorWorld, size(), camera_);
    if (!projected.visible) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);

    const QFontMetrics metrics(font);
    int textWidth = 0;
    for (const QString& line : lines) {
        textWidth = std::max(textWidth, metrics.horizontalAdvance(line));
    }
    const int lineHeight = metrics.height();
    const qreal paddingX = 10.0;
    const qreal paddingY = 7.0;
    const QSizeF boxSize(static_cast<qreal>(textWidth) + paddingX * 2.0,
                         static_cast<qreal>(lineHeight * lines.size()) + paddingY * 2.0);

    QPointF topLeft = projected.position + QPointF(16.0, -boxSize.height() - 12.0);
    const double maxX = std::max(8.0, static_cast<double>(width()) - boxSize.width() - 8.0);
    const double maxY = std::max(8.0, static_cast<double>(height()) - boxSize.height() - 8.0);
    topLeft.setX(std::clamp(topLeft.x(), 8.0, maxX));
    topLeft.setY(std::clamp(topLeft.y(), 8.0, maxY));
    const QRectF box(topLeft, boxSize);

    painter.setPen(QPen(QColor(255, 214, 92, 185), 1.0));
    painter.drawLine(projected.position, QPointF(box.left() + 8.0, box.bottom() - 8.0));

    painter.setPen(QPen(QColor(255, 255, 255, 55), 1.0));
    painter.setBrush(QColor(12, 16, 24, 218));
    painter.drawRoundedRect(box, 7.0, 7.0);

    painter.setPen(QColor(244, 247, 252));
    qreal y = box.top() + paddingY + metrics.ascent();
    for (int index = 0; index < lines.size(); ++index) {
        if (index == 0) {
            QFont boldFont = font;
            boldFont.setBold(true);
            painter.setFont(boldFont);
            painter.setPen(QColor(255, 232, 148));
        } else {
            painter.setFont(font);
            painter.setPen(QColor(230, 236, 246));
        }
        painter.drawText(QPointF(box.left() + paddingX, y), lines[index]);
        y += lineHeight;
    }
}

void GLPreviewWidget::uploadPathTraceScene(QOpenGLShaderProgram& program)
{
    const tinyray::Vec3 cameraPosition = camera_.position();
    tinyray::Vec3 forward = (camera_.target - cameraPosition).normalized();
    if (forward.nearZero()) {
        forward = tinyray::Vec3(0.0, 0.0, -1.0);
    }

    tinyray::Vec3 right = cross(forward, camera_.up).normalized();
    if (right.nearZero()) {
        right = tinyray::Vec3(1.0, 0.0, 0.0);
    }
    tinyray::Vec3 up = cross(right, forward).normalized();
    if (up.nearZero()) {
        up = tinyray::Vec3(0.0, 1.0, 0.0);
    }

    program.setUniformValue("uSelectedId", selectedObjectId_);
    program.setUniformValue("uCameraPosition", toQVector3D(cameraPosition));
    program.setUniformValue("uCameraForward", toQVector3D(forward));
    program.setUniformValue("uCameraRight", toQVector3D(right));
    program.setUniformValue("uCameraUp", toQVector3D(up));
    program.setUniformValue("uCameraFov", static_cast<float>(camera_.fov));

    static SkyboxRenderer skyboxRenderer;
    const GLuint environmentTexture = skyboxRenderer.texture(this, scene_.environment);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, environmentTexture);
    program.setUniformValue("uEnvironmentMap", 1);
    program.setUniformValue("uEnvironmentHasImage", environmentTexture != 0 ? 1 : 0);
    program.setUniformValue("uEnvironmentMode", environmentModeIndex(scene_.environment.mode));
    program.setUniformValue("uEnvironmentTopColor", toQVector3D(scene_.environment.topColor));
    program.setUniformValue("uEnvironmentBottomColor", toQVector3D(scene_.environment.bottomColor));
    program.setUniformValue("uEnvironmentSolidColor", toQVector3D(scene_.environment.solidColor));
    program.setUniformValue("uEnvironmentExposure", static_cast<float>(std::max(scene_.environment.exposure, 0.0)));
    program.setUniformValue("uEnvironmentIntensity", static_cast<float>(std::max(scene_.environment.intensity, 0.0)));
    program.setUniformValue("uEnvironmentRotationY", static_cast<float>(scene_.environment.rotationY));
    static SkyboxRenderer materialTextureRenderer;
    tinyray::Environment materialTextureEnvironment;
    materialTextureEnvironment.mode = tinyray::EnvironmentMode::Image;
    materialTextureEnvironment.imagePath = firstMaterialTexturePath(scene_);
    const GLuint materialTexture = materialTextureRenderer.texture(this, materialTextureEnvironment);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, materialTexture);
    program.setUniformValue("uMaterialImageMap", 2);
    program.setUniformValue("uMaterialImageEnabled", materialTexture != 0 ? 1 : 0);
    glActiveTexture(GL_TEXTURE0);

    int sphereCount = 0;
    int boxCount = 0;
    int cylinderCount = 0;
    int planeCount = 0;
    constexpr int maxSpheres = 16;
    constexpr int maxBoxes = 16;
    constexpr int maxCylinders = 16;
    constexpr int maxPlanes = 8;

    for (const auto& object : scene_.objects) {
        if (const auto* sphere = dynamic_cast<const tinyray::Sphere*>(object.get())) {
            if (sphereCount >= maxSpheres) {
                continue;
            }
            const QString prefix = QStringLiteral("uSphereMaterial[%1]").arg(sphereCount);
            program.setUniformValue(QStringLiteral("uSphereCenter[%1]").arg(sphereCount).toUtf8().constData(),
                                    toQVector3D(sphere->center));
            program.setUniformValue(QStringLiteral("uSphereRadius[%1]").arg(sphereCount).toUtf8().constData(),
                                    static_cast<float>(std::max(sphere->radius, 0.001)));
            program.setUniformValue(QStringLiteral("uSphereId[%1]").arg(sphereCount).toUtf8().constData(),
                                    sphere->id());
            setMaterialUniform(program, prefix, sphere->material);
            ++sphereCount;
        } else if (const auto* box = dynamic_cast<const tinyray::Box*>(object.get())) {
            if (boxCount >= maxBoxes) {
                continue;
            }
            const QString prefix = QStringLiteral("uBoxMaterial[%1]").arg(boxCount);
            program.setUniformValue(QStringLiteral("uBoxMin[%1]").arg(boxCount).toUtf8().constData(),
                                    toQVector3D(box->minCorner()));
            program.setUniformValue(QStringLiteral("uBoxMax[%1]").arg(boxCount).toUtf8().constData(),
                                    toQVector3D(box->maxCorner()));
            program.setUniformValue(QStringLiteral("uBoxId[%1]").arg(boxCount).toUtf8().constData(),
                                    box->id());
            setMaterialUniform(program, prefix, box->material);
            ++boxCount;
        } else if (const auto* cylinder = dynamic_cast<const tinyray::Cylinder*>(object.get())) {
            if (cylinderCount >= maxCylinders) {
                continue;
            }
            const QString prefix = QStringLiteral("uCylinderMaterial[%1]").arg(cylinderCount);
            program.setUniformValue(QStringLiteral("uCylinderCenter[%1]").arg(cylinderCount).toUtf8().constData(),
                                    toQVector3D(cylinder->center));
            program.setUniformValue(QStringLiteral("uCylinderRadius[%1]").arg(cylinderCount).toUtf8().constData(),
                                    static_cast<float>(std::max(std::abs(cylinder->radius), 0.001)));
            program.setUniformValue(QStringLiteral("uCylinderHeight[%1]").arg(cylinderCount).toUtf8().constData(),
                                    static_cast<float>(std::max(std::abs(cylinder->height), 0.001)));
            program.setUniformValue(QStringLiteral("uCylinderId[%1]").arg(cylinderCount).toUtf8().constData(),
                                    cylinder->id());
            setMaterialUniform(program, prefix, cylinder->material);
            ++cylinderCount;
        } else if (const auto* plane = dynamic_cast<const tinyray::Plane*>(object.get())) {
            if (planeCount >= maxPlanes) {
                continue;
            }
            const QString prefix = QStringLiteral("uPlaneMaterial[%1]").arg(planeCount);
            tinyray::Vec3 normal = plane->normal.normalized();
            if (normal.nearZero()) {
                normal = tinyray::Vec3(0.0, 1.0, 0.0);
            }
            program.setUniformValue(QStringLiteral("uPlanePoint[%1]").arg(planeCount).toUtf8().constData(),
                                    toQVector3D(plane->point));
            program.setUniformValue(QStringLiteral("uPlaneNormal[%1]").arg(planeCount).toUtf8().constData(),
                                    toQVector3D(normal));
            program.setUniformValue(QStringLiteral("uPlaneId[%1]").arg(planeCount).toUtf8().constData(),
                                    plane->id());
            setMaterialUniform(program, prefix, plane->material);
            ++planeCount;
        }
    }

    program.setUniformValue("uSphereCount", sphereCount);
    program.setUniformValue("uBoxCount", boxCount);
    program.setUniformValue("uCylinderCount", cylinderCount);
    program.setUniformValue("uPlaneCount", planeCount);

    int lightCount = 0;
    constexpr int maxLights = 8;
    for (const tinyray::Light& light : scene_.lights) {
        if (lightCount >= maxLights) {
            break;
        }
        program.setUniformValue(QStringLiteral("uLightPosition[%1]").arg(lightCount).toUtf8().constData(),
                                toQVector3D(light.position));
        program.setUniformValue(QStringLiteral("uLightColor[%1]").arg(lightCount).toUtf8().constData(),
                                toQVector3D(light.color));
        program.setUniformValue(QStringLiteral("uLightIntensity[%1]").arg(lightCount).toUtf8().constData(),
                                static_cast<float>(light.intensity));
        ++lightCount;
    }
    program.setUniformValue("uLightCount", lightCount);
}

void GLPreviewWidget::drawFullScreenTriangle(QOpenGLShaderProgram& program)
{
    static const GLfloat vertices[] = {
        -1.0f, -1.0f,
         3.0f, -1.0f,
        -1.0f,  3.0f
    };

    const int positionLocation = program.attributeLocation("aPosition");
    if (positionLocation < 0) {
        return;
    }

    program.enableAttributeArray(positionLocation);
    program.setAttributeArray(positionLocation, GL_FLOAT, vertices, 2);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    program.disableAttributeArray(positionLocation);
}

void GLPreviewWidget::drawScene()
{
    for (const auto& object : scene_.objects) {
        if (const auto* sphere = dynamic_cast<const tinyray::Sphere*>(object.get())) {
            drawSphere(*sphere);
        } else if (const auto* box = dynamic_cast<const tinyray::Box*>(object.get())) {
            drawBox(*box);
        } else if (const auto* cylinder = dynamic_cast<const tinyray::Cylinder*>(object.get())) {
            drawCylinder(*cylinder);
        } else if (const auto* plane = dynamic_cast<const tinyray::Plane*>(object.get())) {
            drawPlane(*plane);
        }
    }

    for (const tinyray::Light& light : scene_.lights) {
        drawLightMarker(light, false);
    }
}

void GLPreviewWidget::drawBox(const tinyray::Box& box)
{
    applyMaterial(box.material, box.id() == selectedObjectId_);

    const tinyray::Vec3 minimum = box.minCorner();
    const tinyray::Vec3 maximum = box.maxCorner();

    const tinyray::Vec3 points[8] = {
        tinyray::Vec3(minimum.x, minimum.y, minimum.z),
        tinyray::Vec3(maximum.x, minimum.y, minimum.z),
        tinyray::Vec3(maximum.x, maximum.y, minimum.z),
        tinyray::Vec3(minimum.x, maximum.y, minimum.z),
        tinyray::Vec3(minimum.x, minimum.y, maximum.z),
        tinyray::Vec3(maximum.x, minimum.y, maximum.z),
        tinyray::Vec3(maximum.x, maximum.y, maximum.z),
        tinyray::Vec3(minimum.x, maximum.y, maximum.z)
    };

    const int faces[6][4] = {
        {0, 3, 2, 1},
        {4, 5, 6, 7},
        {0, 1, 5, 4},
        {3, 7, 6, 2},
        {1, 2, 6, 5},
        {0, 4, 7, 3}
    };
    const tinyray::Vec3 normals[6] = {
        tinyray::Vec3(0.0, 0.0, -1.0),
        tinyray::Vec3(0.0, 0.0, 1.0),
        tinyray::Vec3(0.0, -1.0, 0.0),
        tinyray::Vec3(0.0, 1.0, 0.0),
        tinyray::Vec3(1.0, 0.0, 0.0),
        tinyray::Vec3(-1.0, 0.0, 0.0)
    };

    glBegin(GL_QUADS);
    for (int face = 0; face < 6; ++face) {
        glNormal3d(normals[face].x, normals[face].y, normals[face].z);
        for (int corner = 0; corner < 4; ++corner) {
            const tinyray::Vec3& point = points[faces[face][corner]];
            glVertex3d(point.x, point.y, point.z);
        }
    }
    glEnd();
}

void GLPreviewWidget::drawCylinder(const tinyray::Cylinder& cylinder)
{
    applyMaterial(cylinder.material, cylinder.id() == selectedObjectId_);

    constexpr int slices = 32;
    const double radius = std::max(std::abs(cylinder.radius), 0.01);
    const double halfHeight = std::max(std::abs(cylinder.height), 0.01) * 0.5;
    const double bottomY = cylinder.center.y - halfHeight;
    const double topY = cylinder.center.y + halfHeight;

    glBegin(GL_QUAD_STRIP);
    for (int slice = 0; slice <= slices; ++slice) {
        const double theta = 2.0 * tinyray::pi * static_cast<double>(slice) / slices;
        const double x = std::cos(theta);
        const double z = std::sin(theta);
        glNormal3d(x, 0.0, z);
        glVertex3d(cylinder.center.x + x * radius, bottomY, cylinder.center.z + z * radius);
        glVertex3d(cylinder.center.x + x * radius, topY, cylinder.center.z + z * radius);
    }
    glEnd();

    glBegin(GL_TRIANGLE_FAN);
    glNormal3d(0.0, 1.0, 0.0);
    glVertex3d(cylinder.center.x, topY, cylinder.center.z);
    for (int slice = 0; slice <= slices; ++slice) {
        const double theta = 2.0 * tinyray::pi * static_cast<double>(slice) / slices;
        glVertex3d(cylinder.center.x + std::cos(theta) * radius,
                   topY,
                   cylinder.center.z + std::sin(theta) * radius);
    }
    glEnd();

    glBegin(GL_TRIANGLE_FAN);
    glNormal3d(0.0, -1.0, 0.0);
    glVertex3d(cylinder.center.x, bottomY, cylinder.center.z);
    for (int slice = slices; slice >= 0; --slice) {
        const double theta = 2.0 * tinyray::pi * static_cast<double>(slice) / slices;
        glVertex3d(cylinder.center.x + std::cos(theta) * radius,
                   bottomY,
                   cylinder.center.z + std::sin(theta) * radius);
    }
    glEnd();
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

void GLPreviewWidget::drawLightMarker(const tinyray::Light& light, bool selected)
{
    glDisable(GL_LIGHTING);
    glLineWidth(selected ? 4.0f : 2.0f);
    if (selected) {
        glColor3d(1.0, 0.82, 0.12);
    } else {
        glColor3d(std::clamp(light.color.x, 0.0, 1.0),
                  std::clamp(light.color.y, 0.0, 1.0),
                  std::clamp(light.color.z, 0.0, 1.0));
    }

    const tinyray::Vec3 p = light.position;
    if (light.type == tinyray::LightType::Area) {
        const tinyray::Vec3 t = light.tangent();
        const tinyray::Vec3 b = light.bitangent();
        const double halfWidth = std::max(light.width, 0.01) * 0.5;
        const double halfHeight = std::max(light.height, 0.01) * 0.5;
        const tinyray::Vec3 corners[4] = {
            p - t * halfWidth - b * halfHeight,
            p + t * halfWidth - b * halfHeight,
            p + t * halfWidth + b * halfHeight,
            p - t * halfWidth + b * halfHeight
        };

        glColor4d(std::clamp(light.color.x, 0.0, 1.0),
                  std::clamp(light.color.y, 0.0, 1.0),
                  std::clamp(light.color.z, 0.0, 1.0),
                  selected ? 0.72 : 0.48);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBegin(GL_QUADS);
        for (const tinyray::Vec3& corner : corners) {
            glVertex3d(corner.x, corner.y, corner.z);
        }
        glEnd();

        glColor3d(selected ? 1.0 : std::clamp(light.color.x, 0.0, 1.0),
                  selected ? 0.82 : std::clamp(light.color.y, 0.0, 1.0),
                  selected ? 0.12 : std::clamp(light.color.z, 0.0, 1.0));
        glBegin(GL_LINE_LOOP);
        for (const tinyray::Vec3& corner : corners) {
            glVertex3d(corner.x, corner.y, corner.z);
        }
        glEnd();
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
        return;
    }

    const double size = selected ? 0.34 : 0.24;

    glBegin(GL_LINES);
    glVertex3d(p.x - size, p.y, p.z);
    glVertex3d(p.x + size, p.y, p.z);
    glVertex3d(p.x, p.y - size, p.z);
    glVertex3d(p.x, p.y + size, p.z);
    glVertex3d(p.x, p.y, p.z - size);
    glVertex3d(p.x, p.y, p.z + size);
    glEnd();

    glPointSize(selected ? 11.0f : 8.0f);
    glBegin(GL_POINTS);
    glVertex3d(p.x, p.y, p.z);
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
    } else if (material.type == tinyray::MaterialType::Emissive) {
        const double strength = material.safeEmissionStrength();
        color = material.emissionColor * (0.35 + std::min(strength, 8.0) * 0.12);
        shininess = 8.0f;
        specular[0] = specular[1] = specular[2] = 0.05f;
        emission[0] = static_cast<GLfloat>(std::clamp(material.emissionColor.x * (0.20 + strength * 0.08), 0.0, 1.0));
        emission[1] = static_cast<GLfloat>(std::clamp(material.emissionColor.y * (0.20 + strength * 0.08), 0.0, 1.0));
        emission[2] = static_cast<GLfloat>(std::clamp(material.emissionColor.z * (0.20 + strength * 0.08), 0.0, 1.0));
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
    const tinyray::Ray ray = tinyray::makePickingRay(screenPosition, size(), camera_);
    const int lightIndex = pickLight(ray);
    if (lightIndex >= 0) {
        selectedObjectId_ = -1;
        selectedLightIndex_ = lightIndex;
        scene_.selectedObjectId = -1;
        emit lightSelected(lightIndex);
        resetPathTraceAccumulation();
        update();
        return;
    }

    const int objectId = tinyray::pickObjectId(screenPosition, size(), camera_, scene_);
    if (selectedLightIndex_ < 0 && selectedObjectId_ == objectId) {
        return;
    }

    selectedObjectId_ = objectId;
    selectedLightIndex_ = -1;
    scene_.selectedObjectId = objectId;
    emit objectSelected(objectId);
    if (objectId < 0) {
        emit lightSelected(-1);
    }
    resetPathTraceAccumulation();
    update();
}

bool GLPreviewWidget::beginObjectDrag(const QPoint& screenPosition)
{
    const tinyray::Ray ray = tinyray::makePickingRay(screenPosition, size(), camera_);
    const int lightIndex = pickLight(ray);
    if (lightIndex >= 0) {
        tinyray::Vec3 lightCenter = scene_.lights[static_cast<std::size_t>(lightIndex)].position;
        dragStartObjectCenter_ = lightCenter;
        tinyray::Vec3 dragPoint;
        if (isAxisDragActive()) {
            dragPoint = lightCenter;
        } else if (!rayIntersectsDragPlane(ray, dragPoint)) {
            dragPoint = lightCenter;
        }

        selectedObjectId_ = -1;
        selectedLightIndex_ = lightIndex;
        draggedObjectId_ = -1;
        draggedLightIndex_ = lightIndex;
        isDraggingObject_ = false;
        isDraggingLight_ = true;
        scene_.selectedObjectId = -1;
        dragStartGroundPoint_ = dragPoint;
        dragOffset_ = dragStartObjectCenter_ - dragStartGroundPoint_;

        emit lightSelected(lightIndex);
        emit interactionStatusChanged(QStringLiteral("Light dragging"));
        resetPathTraceAccumulation();
        update();
        return true;
    }

    tinyray::HitRecord record;
    if (!scene_.intersect(ray, 0.001, tinyray::infinity, record)) {
        return false;
    }

    tinyray::Vec3 objectCenter;
    if (!draggableObjectCenter(record.hitObjectId, objectCenter)) {
        return false;
    }

    dragStartObjectCenter_ = objectCenter;
    tinyray::Vec3 groundPoint;
    if (isAxisDragActive()) {
        groundPoint = objectCenter;
    } else if (!rayIntersectsDragPlane(ray, groundPoint)) {
        return false;
    }

    selectedObjectId_ = record.hitObjectId;
    selectedLightIndex_ = -1;
    scene_.selectedObjectId = selectedObjectId_;
    draggedObjectId_ = selectedObjectId_;
    isDraggingObject_ = true;
    dragStartGroundPoint_ = groundPoint;
    dragOffset_ = dragStartObjectCenter_ - dragStartGroundPoint_;

    emit objectSelected(selectedObjectId_);
    emit interactionStatusChanged(QStringLiteral("Object dragging"));
    resetPathTraceAccumulation();
    update();
    return true;
}

bool GLPreviewWidget::beginGizmoDrag(const QPoint& screenPosition)
{
    const GizmoAxis pickedAxis = pickGizmoAxis(screenPosition);
    if (pickedAxis == GizmoAxis::None) {
        return false;
    }

    tinyray::Vec3 center;
    if (!gizmoCenter(center)) {
        return false;
    }

    activeGizmoAxis_ = pickedAxis;
    dragStartObjectCenter_ = center;
    dragStartGroundPoint_ = center;
    dragOffset_ = tinyray::Vec3(0.0, 0.0, 0.0);

    if (selectedLightIndex_ >= 0 && selectedLightIndex_ < static_cast<int>(scene_.lights.size())) {
        draggedLightIndex_ = selectedLightIndex_;
        draggedObjectId_ = -1;
        isDraggingLight_ = true;
        isDraggingObject_ = false;
        scene_.selectedObjectId = -1;
        emit lightSelected(selectedLightIndex_);
    } else if (selectedObjectId_ >= 0 && draggableObjectCenter(selectedObjectId_, center)) {
        draggedObjectId_ = selectedObjectId_;
        draggedLightIndex_ = -1;
        isDraggingObject_ = true;
        isDraggingLight_ = false;
        scene_.selectedObjectId = selectedObjectId_;
        emit objectSelected(selectedObjectId_);
    } else {
        activeGizmoAxis_ = GizmoAxis::None;
        return false;
    }

    emit interactionStatusChanged(QStringLiteral("Transform gizmo dragging"));
    resetPathTraceAccumulation();
    update();
    return true;
}

void GLPreviewWidget::updateObjectDrag(const QPoint& screenPosition)
{
    if (isDraggingLight_) {
        if (draggedLightIndex_ < 0 || draggedLightIndex_ >= static_cast<int>(scene_.lights.size())) {
            finishObjectDrag();
            return;
        }

        tinyray::Vec3 newCenter;
        if (isAxisDragActive()) {
            newCenter = axisDragCenter(screenPosition);
        } else {
            tinyray::Vec3 groundPoint;
            const tinyray::Ray ray = tinyray::makePickingRay(screenPosition, size(), camera_);
            if (!rayIntersectsDragPlane(ray, groundPoint)) {
                return;
            }
            newCenter = groundPoint + dragOffset_;
        }

        scene_.lights[static_cast<std::size_t>(draggedLightIndex_)].position = newCenter;
        selectedObjectId_ = -1;
        selectedLightIndex_ = draggedLightIndex_;
        scene_.selectedObjectId = -1;
        updateTurntableTarget();

        emit lightMoved(draggedLightIndex_, newCenter.x, newCenter.y, newCenter.z);
        emit interactionStatusChanged(QStringLiteral("Light dragging"));
        resetPathTraceAccumulation();
        update();
        return;
    }

    if (!isDraggingObject_) {
        return;
    }

    tinyray::Vec3 currentCenter;
    if (!draggableObjectCenter(draggedObjectId_, currentCenter)) {
        finishObjectDrag();
        return;
    }

    tinyray::Vec3 newCenter;
    if (isAxisDragActive()) {
        newCenter = axisDragCenter(screenPosition);
    } else {
        tinyray::Vec3 groundPoint;
        const tinyray::Ray ray = tinyray::makePickingRay(screenPosition, size(), camera_);
        if (!rayIntersectsDragPlane(ray, groundPoint)) {
            return;
        }
        newCenter = groundPoint + dragOffset_;
    }
    if (!setDraggableObjectCenter(draggedObjectId_, newCenter)) {
        finishObjectDrag();
        return;
    }

    scene_.selectedObjectId = draggedObjectId_;
    updateTurntableTarget();

    emit objectMoved(draggedObjectId_, newCenter.x, newCenter.y, newCenter.z);
    emit interactionStatusChanged(QStringLiteral("Object dragging"));
    resetPathTraceAccumulation();
    update();
}

void GLPreviewWidget::finishObjectDrag()
{
    isDraggingObject_ = false;
    isDraggingLight_ = false;
    draggedObjectId_ = -1;
    draggedLightIndex_ = -1;
    activeGizmoAxis_ = GizmoAxis::None;
}

void GLPreviewWidget::pauseTurntableForUserInput()
{
    if (!camera_.turntableEnabled) {
        return;
    }

    camera_.turntableEnabled = false;
    turntablePausedByUserInputPending_ = true;
    emit turntablePausedByUserInput();
    emit interactionStatusChanged(QStringLiteral("Turntable paused by user input"));
}

void GLPreviewWidget::updateTurntableTarget()
{
    const tinyray::Vec3 center = sceneCenter();
    tinyray::Vec3 selectedCenter;
    const bool hasSelectedCenter = selectedObjectCenter(selectedCenter);
    const tinyray::Vec3* selectedCenterPointer = hasSelectedCenter ? &selectedCenter : nullptr;
    tinyray::Vec3 newTarget = camera_.resolvedTurntableTarget(center, selectedCenterPointer);

    camera_.target = newTarget;
}

tinyray::Vec3 GLPreviewWidget::sceneCenter() const
{
    if (scene_.objects.empty()) {
        return tinyray::Vec3(0.0, 0.0, 0.0);
    }

    tinyray::Vec3 sum(0.0, 0.0, 0.0);
    int count = 0;
    for (const auto& object : scene_.objects) {
        if (!object) {
            continue;
        }
        const tinyray::Vec3 position = objectPosition(object.get());
        if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z)) {
            continue;
        }
        sum += position;
        ++count;
    }

    if (count == 0) {
        return tinyray::Vec3(0.0, 0.0, 0.0);
    }
    return sum / static_cast<double>(count);
}

bool GLPreviewWidget::selectedObjectCenter(tinyray::Vec3& center) const
{
    const tinyray::Object* object = scene_.objectById(selectedObjectId_);
    if (object == nullptr) {
        return false;
    }

    center = objectPosition(object);
    return std::isfinite(center.x) && std::isfinite(center.y) && std::isfinite(center.z);
}

int GLPreviewWidget::pickLight(const tinyray::Ray& ray) const
{
    int bestIndex = -1;
    double bestT = tinyray::infinity;
    constexpr double markerRadius = 0.28;

    for (int index = 0; index < static_cast<int>(scene_.lights.size()); ++index) {
        const tinyray::Vec3 oc = ray.origin - scene_.lights[static_cast<std::size_t>(index)].position;
        const double a = dot(ray.direction, ray.direction);
        const double halfB = dot(oc, ray.direction);
        const double c = dot(oc, oc) - markerRadius * markerRadius;
        const double discriminant = halfB * halfB - a * c;
        if (discriminant < 0.0) {
            continue;
        }

        const double root = std::sqrt(discriminant);
        double t = (-halfB - root) / a;
        if (t <= 0.001) {
            t = (-halfB + root) / a;
        }
        if (t > 0.001 && t < bestT) {
            bestT = t;
            bestIndex = index;
        }
    }

    return bestIndex;
}

bool GLPreviewWidget::draggableObjectCenter(int objectId, tinyray::Vec3& center) const
{
    const auto* sphere = dynamic_cast<const tinyray::Sphere*>(scene_.objectById(objectId));
    if (sphere != nullptr) {
        center = sphere->center;
        return true;
    }

    const auto* box = dynamic_cast<const tinyray::Box*>(scene_.objectById(objectId));
    if (box != nullptr) {
        center = box->center;
        return true;
    }

    const auto* cylinder = dynamic_cast<const tinyray::Cylinder*>(scene_.objectById(objectId));
    if (cylinder != nullptr) {
        center = cylinder->center;
        return true;
    }

    return false;
}

bool GLPreviewWidget::setDraggableObjectCenter(int objectId, const tinyray::Vec3& center)
{
    auto* sphere = dynamic_cast<tinyray::Sphere*>(scene_.objectById(objectId));
    if (sphere != nullptr) {
        sphere->center = center;
        return true;
    }

    auto* box = dynamic_cast<tinyray::Box*>(scene_.objectById(objectId));
    if (box != nullptr) {
        box->center = center;
        return true;
    }

    auto* cylinder = dynamic_cast<tinyray::Cylinder*>(scene_.objectById(objectId));
    if (cylinder != nullptr) {
        cylinder->center = center;
        return true;
    }

    return false;
}

bool GLPreviewWidget::gizmoCenter(tinyray::Vec3& center) const
{
    if (selectedLightIndex_ >= 0 && selectedLightIndex_ < static_cast<int>(scene_.lights.size())) {
        center = scene_.lights[static_cast<std::size_t>(selectedLightIndex_)].position;
        return std::isfinite(center.x) && std::isfinite(center.y) && std::isfinite(center.z);
    }

    return selectedObjectId_ >= 0 && draggableObjectCenter(selectedObjectId_, center);
}

GLPreviewWidget::GizmoAxis GLPreviewWidget::pickGizmoAxis(const QPoint& screenPosition) const
{
    tinyray::Vec3 center;
    if (!gizmoCenter(center)) {
        return GizmoAxis::None;
    }

    const tinyray::ScreenProjection centerProjection = tinyray::worldToScreen(center, size(), camera_);
    if (!centerProjection.visible) {
        return GizmoAxis::None;
    }

    const double cameraDistance = std::max((center - camera_.position()).length(), 1.0);
    const double axisLength = std::clamp(cameraDistance * 0.18, 0.55, 2.2);
    const QPointF cursor(static_cast<qreal>(screenPosition.x()), static_cast<qreal>(screenPosition.y()));

    auto distanceToSegment = [](const QPointF& point, const QPointF& start, const QPointF& end) {
        const double vx = end.x() - start.x();
        const double vy = end.y() - start.y();
        const double lengthSquared = vx * vx + vy * vy;
        if (lengthSquared <= 1.0e-8) {
            const double dx = point.x() - start.x();
            const double dy = point.y() - start.y();
            return std::sqrt(dx * dx + dy * dy);
        }

        const double wx = point.x() - start.x();
        const double wy = point.y() - start.y();
        const double t = std::clamp((wx * vx + wy * vy) / lengthSquared, 0.0, 1.0);
        const double closestX = start.x() + vx * t;
        const double closestY = start.y() + vy * t;
        const double dx = point.x() - closestX;
        const double dy = point.y() - closestY;
        return std::sqrt(dx * dx + dy * dy);
    };

    GizmoAxis bestAxis = GizmoAxis::None;
    double bestDistance = 13.0;
    const GizmoAxis axes[] = {GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z};
    for (const GizmoAxis axis : axes) {
        const tinyray::Vec3 end = center + gizmoAxisVector(axis) * axisLength;
        const tinyray::ScreenProjection endProjection = tinyray::worldToScreen(end, size(), camera_);
        if (!endProjection.visible) {
            continue;
        }

        const double distance = distanceToSegment(cursor, centerProjection.position, endProjection.position);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestAxis = axis;
        }
    }

    return bestAxis;
}

bool GLPreviewWidget::isAxisDragActive() const
{
    return activeGizmoAxis_ != GizmoAxis::None
        || objectDragMode_ == tinyray::ObjectDragMode::AxisX
        || objectDragMode_ == tinyray::ObjectDragMode::AxisY
        || objectDragMode_ == tinyray::ObjectDragMode::AxisZ;
}

tinyray::Vec3 GLPreviewWidget::gizmoAxisVector(GizmoAxis axis) const
{
    if (axis == GizmoAxis::Y) {
        return tinyray::Vec3(0.0, 1.0, 0.0);
    }
    if (axis == GizmoAxis::Z) {
        return tinyray::Vec3(0.0, 0.0, 1.0);
    }
    return tinyray::Vec3(1.0, 0.0, 0.0);
}

bool GLPreviewWidget::rayIntersectsDragPlane(const tinyray::Ray& ray, tinyray::Vec3& hitPoint) const
{
    constexpr double epsilon = 1.0e-8;
    tinyray::Vec3 planePoint = dragStartObjectCenter_;
    tinyray::Vec3 planeNormal(0.0, 1.0, 0.0);

    if (objectDragMode_ == tinyray::ObjectDragMode::PlaneXY) {
        planeNormal = tinyray::Vec3(0.0, 0.0, 1.0);
    } else if (objectDragMode_ == tinyray::ObjectDragMode::PlaneYZ) {
        planeNormal = tinyray::Vec3(1.0, 0.0, 0.0);
    }

    const double denominator = dot(ray.direction, planeNormal);
    if (std::abs(denominator) < epsilon) {
        return false;
    }

    const double t = dot(planePoint - ray.origin, planeNormal) / denominator;
    if (!std::isfinite(t) || t <= epsilon) {
        return false;
    }

    hitPoint = ray.at(t);
    return std::isfinite(hitPoint.x) && std::isfinite(hitPoint.y) && std::isfinite(hitPoint.z);
}

tinyray::Vec3 GLPreviewWidget::axisDragCenter(const QPoint& screenPosition) const
{
    const tinyray::Vec3 axis = dragAxisVector();
    const QPoint delta = screenPosition - pressMousePosition_;
    const QSize bufferSize = renderBufferSize();
    const double normalizedDx = static_cast<double>(delta.x()) / static_cast<double>(std::max(bufferSize.width(), 1));
    const double normalizedDy = static_cast<double>(delta.y()) / static_cast<double>(std::max(bufferSize.height(), 1));

    const tinyray::Vec3 cameraPosition = camera_.position();
    tinyray::Vec3 forward = (camera_.target - cameraPosition).normalized();
    if (forward.nearZero()) {
        forward = tinyray::Vec3(0.0, 0.0, -1.0);
    }

    tinyray::Vec3 right = cross(forward, camera_.up).normalized();
    if (right.nearZero()) {
        right = tinyray::Vec3(1.0, 0.0, 0.0);
    }
    tinyray::Vec3 up = cross(right, forward).normalized();
    if (up.nearZero()) {
        up = tinyray::Vec3(0.0, 1.0, 0.0);
    }

    const double projectedX = dot(axis, right);
    const double projectedY = -dot(axis, up);
    const double screenProjectionLength = std::sqrt(projectedX * projectedX + projectedY * projectedY);
    if (screenProjectionLength < 1.0e-6) {
        return dragStartObjectCenter_;
    }

    const double movementOnScreen = (normalizedDx * projectedX + normalizedDy * projectedY) / screenProjectionLength;
    const double distanceScale = std::max((dragStartObjectCenter_ - cameraPosition).length(), 1.0);
    const double worldDelta = movementOnScreen * distanceScale * 2.2;
    return dragStartObjectCenter_ + axis * worldDelta;
}

tinyray::Vec3 GLPreviewWidget::dragAxisVector() const
{
    if (activeGizmoAxis_ != GizmoAxis::None) {
        return gizmoAxisVector(activeGizmoAxis_);
    }
    if (objectDragMode_ == tinyray::ObjectDragMode::AxisY) {
        return tinyray::Vec3(0.0, 1.0, 0.0);
    }
    if (objectDragMode_ == tinyray::ObjectDragMode::AxisZ) {
        return tinyray::Vec3(0.0, 0.0, 1.0);
    }
    return tinyray::Vec3(1.0, 0.0, 0.0);
}
