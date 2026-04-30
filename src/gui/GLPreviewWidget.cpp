#include "gui/GLPreviewWidget.h"

#include <algorithm>
#include <cmath>

#include <QOpenGLFramebufferObjectFormat>
#include <QMouseEvent>
#include <QSize>
#include <QVector2D>
#include <QVector3D>
#include <QWheelEvent>
#include <QtGui/qopengl.h>

#include "core/Light.h"
#include "core/HitRecord.h"
#include "core/Box.h"
#include "core/Cylinder.h"
#include "core/MathUtils.h"
#include "core/Plane.h"
#include "core/Sphere.h"
#include "interaction/Picking.h"

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
    case tinyray::MaterialType::Diffuse:
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
    resetPathTraceAccumulation();
    update();
}

void GLPreviewWidget::setSelectedObjectId(int objectId)
{
    selectedObjectId_ = objectId;
    scene_.selectedObjectId = objectId;
    resetPathTraceAccumulation();
    update();
}

void GLPreviewWidget::setObjectDragMode(tinyray::ObjectDragMode mode)
{
    objectDragMode_ = mode;
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
        return;
    }

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
        emit interactionStatusChanged(QStringLiteral("Camera orbiting"));
        resetPathTraceAccumulation();
        update();
    } else if (dragMode_ == DragMode::Pan) {
        camera_.pan(-delta.x(), delta.y());
        emit interactionStatusChanged(QStringLiteral("Camera panning"));
        resetPathTraceAccumulation();
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
    emit interactionStatusChanged(QStringLiteral("Real-time rendering"));
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
        };

        uniform sampler2D uPreviousFrame;
        uniform vec2 uResolution;
        uniform int uFrameIndex;
        uniform int uSelectedId;
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

        struct Hit {
            float t;
            vec3 point;
            vec3 normal;
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

        vec3 skyColor(vec3 direction)
        {
            float t = 0.5 * (normalize(direction).y + 1.0);
            vec3 horizon = vec3(0.78, 0.84, 0.92);
            vec3 zenith = vec3(0.12, 0.22, 0.38);
            return mix(horizon, zenith, t);
        }

        MaterialData fallbackMaterial()
        {
            MaterialData material;
            material.type = 0;
            material.albedo = vec3(0.8);
            material.roughness = 0.2;
            material.refractiveIndex = 1.5;
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

        float schlick(float cosine, float refractiveIndex)
        {
            float r0 = (1.0 - refractiveIndex) / (1.0 + refractiveIndex);
            r0 = r0 * r0;
            return r0 + (1.0 - r0) * pow(1.0 - cosine, 5.0);
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
                    radiance += throughput * skyColor(rd);
                    break;
                }

                vec3 normal = normalize(hit.normal);
                if (hit.objectId == uSelectedId) {
                    radiance += throughput * vec3(0.8, 0.55, 0.08) * 0.12;
                }

                vec3 viewDirection = normalize(-rd);
                radiance += throughput * directLighting(hit.point, normal, viewDirection, hit.material);

                if (hit.material.type == 1) {
                    vec3 reflected = reflect(rd, normal);
                    float roughness = clamp(hit.material.roughness, 0.0, 1.0);
                    rd = normalize(mix(reflected, reflected + randomUnitVector(seed + float(bounce) * 11.3), roughness * roughness));
                    ro = hit.point + normal * 0.003;
                    throughput *= hit.material.albedo;
                } else if (hit.material.type == 2) {
                    float eta = dot(rd, normal) < 0.0 ? 1.0 / max(hit.material.refractiveIndex, 1.01) : max(hit.material.refractiveIndex, 1.01);
                    vec3 outwardNormal = dot(rd, normal) < 0.0 ? normal : -normal;
                    float cosTheta = min(dot(-rd, outwardNormal), 1.0);
                    vec3 refracted = refract(rd, outwardNormal, eta);
                    float reflectChance = length(refracted) < 0.001 ? 1.0 : schlick(cosTheta, hit.material.refractiveIndex);
                    if (hash(seed + float(bounce) * 23.7) < reflectChance) {
                        rd = normalize(reflect(rd, normal));
                        ro = hit.point + normal * 0.003;
                    } else {
                        rd = normalize(refracted);
                        ro = hit.point - outwardNormal * 0.003;
                    }
                    throughput *= mix(hit.material.albedo, vec3(1.0), 0.65);
                } else {
                    vec3 target = normal + randomUnitVector(seed + float(bounce) * 7.1);
                    rd = normalize(target);
                    ro = hit.point + normal * 0.003;
                    throughput *= hit.material.albedo;
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
        varying vec2 vUv;

        void main()
        {
            vec3 color = texture2D(uImage, vUv).rgb;
            color = color / (color + vec3(1.0));
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
    writeBuffer->release();

    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    const QSize defaultBufferSize = renderBufferSize();
    glViewport(0, 0, defaultBufferSize.width(), defaultBufferSize.height());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    displayProgram_->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, writeBuffer->texture());
    displayProgram_->setUniformValue("uImage", 0);
    drawFullScreenTriangle(*displayProgram_);
    displayProgram_->release();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    accumulationReadIndex_ = writeIndex;
    pathTraceFrameIndex_ = std::min(pathTraceFrameIndex_ + 1, 8192);
    if (pathTraceFrameIndex_ < 8192) {
        update();
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
        drawLightMarker(light);
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
    resetPathTraceAccumulation();
    update();
}

bool GLPreviewWidget::beginObjectDrag(const QPoint& screenPosition)
{
    const tinyray::Ray ray = tinyray::makePickingRay(screenPosition, size(), camera_);
    tinyray::HitRecord record;
    if (!scene_.intersect(ray, 0.001, tinyray::infinity, record)) {
        return false;
    }

    tinyray::Vec3 objectCenter;
    if (!draggableObjectCenter(record.hitObjectId, objectCenter)) {
        return false;
    }

    tinyray::Vec3 groundPoint;
    if (objectDragMode_ == tinyray::ObjectDragMode::AxisX
        || objectDragMode_ == tinyray::ObjectDragMode::AxisY
        || objectDragMode_ == tinyray::ObjectDragMode::AxisZ) {
        groundPoint = objectCenter;
    } else if (!rayIntersectsDragPlane(ray, groundPoint)) {
        return false;
    }

    selectedObjectId_ = record.hitObjectId;
    scene_.selectedObjectId = selectedObjectId_;
    draggedObjectId_ = selectedObjectId_;
    isDraggingObject_ = true;
    dragStartObjectCenter_ = objectCenter;
    dragStartGroundPoint_ = groundPoint;
    dragOffset_ = dragStartObjectCenter_ - dragStartGroundPoint_;

    emit objectSelected(selectedObjectId_);
    emit interactionStatusChanged(QStringLiteral("Object dragging"));
    resetPathTraceAccumulation();
    update();
    return true;
}

void GLPreviewWidget::updateObjectDrag(const QPoint& screenPosition)
{
    if (!isDraggingObject_) {
        return;
    }

    tinyray::Vec3 currentCenter;
    if (!draggableObjectCenter(draggedObjectId_, currentCenter)) {
        finishObjectDrag();
        return;
    }

    tinyray::Vec3 newCenter;
    if (objectDragMode_ == tinyray::ObjectDragMode::AxisX
        || objectDragMode_ == tinyray::ObjectDragMode::AxisY
        || objectDragMode_ == tinyray::ObjectDragMode::AxisZ) {
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

    emit objectMoved(draggedObjectId_, newCenter.x, newCenter.y, newCenter.z);
    emit interactionStatusChanged(QStringLiteral("Object dragging"));
    resetPathTraceAccumulation();
    update();
}

void GLPreviewWidget::finishObjectDrag()
{
    isDraggingObject_ = false;
    draggedObjectId_ = -1;
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
    if (objectDragMode_ == tinyray::ObjectDragMode::AxisY) {
        return tinyray::Vec3(0.0, 1.0, 0.0);
    }
    if (objectDragMode_ == tinyray::ObjectDragMode::AxisZ) {
        return tinyray::Vec3(0.0, 0.0, 1.0);
    }
    return tinyray::Vec3(1.0, 0.0, 0.0);
}
