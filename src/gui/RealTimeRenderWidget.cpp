#include "gui/RealTimeRenderWidget.h"

#include <algorithm>
#include <cmath>

#include <QtGui/qopengl.h>

#include "core/Box.h"
#include "core/Cylinder.h"
#include "core/Plane.h"
#include "core/Sphere.h"

RealTimeRenderWidget::RealTimeRenderWidget(QWidget* parent)
    : GLPreviewWidget(parent)
    , particleSystem_(5000)
{
    rainEmitter_.setSettings(rainSettings_);
    splashEmitter_.setEnabled(rainSettings_.splashEnabled);
    splashEmitter_.setIntensity(rainSettings_.splashIntensity);
    splashEmitter_.setParticleSize(rainSettings_.particleSize * 0.60);
    splashEmitter_.setGravity(rainSettings_.gravity);

    turntableTimer_.setInterval(16);
    connect(&turntableTimer_, &QTimer::timeout,
            this, &RealTimeRenderWidget::handleTurntableTick);
    turntableClock_.start();
    turntableTimer_.start();
}

void RealTimeRenderWidget::setRainSettings(const tinyray::RainSettings& settings)
{
    const bool wasEnabled = rainSettings_.rainEnabled;
    rainSettings_ = settings;
    rainSettings_.rainRate = std::clamp(rainSettings_.rainRate, 0.0, 3000.0);
    rainSettings_.dropSpeed = std::clamp(rainSettings_.dropSpeed, 0.1, 80.0);
    rainSettings_.gravity = std::clamp(rainSettings_.gravity, 0.0, 120.0);
    rainSettings_.spawnAreaSize = std::clamp(rainSettings_.spawnAreaSize, 0.5, 80.0);
    rainSettings_.spawnHeight = std::clamp(rainSettings_.spawnHeight, 0.5, 80.0);
    rainSettings_.splashIntensity = std::clamp(rainSettings_.splashIntensity, 0.0, 5.0);
    rainSettings_.particleSize = std::clamp(rainSettings_.particleSize, 0.002, 0.18);

    rainEmitter_.setSettings(rainSettings_);
    splashEmitter_.setEnabled(rainSettings_.splashEnabled);
    splashEmitter_.setIntensity(rainSettings_.splashIntensity);
    splashEmitter_.setParticleSize(rainSettings_.particleSize * 0.60);
    splashEmitter_.setGravity(rainSettings_.gravity);

    if (wasEnabled && !rainSettings_.rainEnabled) {
        particleSystem_.clear();
    }
    update();
}

tinyray::RainSettings RealTimeRenderWidget::rainSettings() const
{
    return rainSettings_;
}

void RealTimeRenderWidget::clearParticles()
{
    particleSystem_.clear();
    update();
}

void RealTimeRenderWidget::handleTurntableTick()
{
    const qint64 elapsedMilliseconds = turntableClock_.restart();
    if (elapsedMilliseconds <= 0) {
        return;
    }

    const double dt = std::min(static_cast<double>(elapsedMilliseconds) / 1000.0, 0.05);
    if (turntableEnabled()) {
        advanceTurntable(dt);
    }
    updateParticles(dt);
}

void RealTimeRenderWidget::drawRealtimeOverlay()
{
    const auto& particles = particleSystem_.particles();
    if (particles.empty()) {
        return;
    }

    const tinyray::OrbitCamera& camera = previewCamera();
    const tinyray::Vec3 cameraPosition = camera.position();
    tinyray::Vec3 forward = (camera.target - cameraPosition).normalized();
    if (forward.nearZero()) {
        forward = tinyray::Vec3(0.0, 0.0, -1.0);
    }

    tinyray::Vec3 right = cross(forward, camera.up).normalized();
    if (right.nearZero()) {
        right = tinyray::Vec3(1.0, 0.0, 0.0);
    }
    tinyray::Vec3 up = cross(right, forward).normalized();
    if (up.nearZero()) {
        up = tinyray::Vec3(0.0, 1.0, 0.0);
    }

    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glBegin(GL_QUADS);
    for (const tinyray::Particle& particle : particles) {
        if (!particle.alive || !particle.isFinite()) {
            continue;
        }

        double fade = 1.0 - particle.normalizedAge();
        fade = particle.type == tinyray::ParticleType::Rain ? fade * fade : fade * fade * fade;
        const double alpha = std::clamp(particle.alpha * fade, 0.0, 1.0);
        if (alpha <= 0.01) {
            continue;
        }

        const double size = std::clamp(particle.size, 0.001, 0.20);
        const double stretch = particle.type == tinyray::ParticleType::Rain ? 2.2 : 0.9;
        const tinyray::Vec3 horizontal = right * (particle.type == tinyray::ParticleType::Rain ? size * 0.35 : size);
        const tinyray::Vec3 vertical = up * (size * stretch);
        const tinyray::Vec3 p0 = particle.position - horizontal - vertical;
        const tinyray::Vec3 p1 = particle.position + horizontal - vertical;
        const tinyray::Vec3 p2 = particle.position + horizontal + vertical;
        const tinyray::Vec3 p3 = particle.position - horizontal + vertical;

        const double brightness = particle.type == tinyray::ParticleType::Splash ? 1.35 : 1.0;
        glColor4d(std::clamp(particle.color.x * brightness, 0.0, 1.0),
                  std::clamp(particle.color.y * brightness, 0.0, 1.0),
                  std::clamp(particle.color.z * brightness, 0.0, 1.0),
                  alpha);
        glVertex3d(p0.x, p0.y, p0.z);
        glVertex3d(p1.x, p1.y, p1.z);
        glVertex3d(p2.x, p2.y, p2.z);
        glVertex3d(p3.x, p3.y, p3.z);
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
}

void RealTimeRenderWidget::updateParticles(double deltaTimeSeconds)
{
    if (!std::isfinite(deltaTimeSeconds) || deltaTimeSeconds <= 0.0) {
        return;
    }

    const bool hadParticles = !particleSystem_.empty();
    if (rainSettings_.rainEnabled) {
        rainEmitter_.emitParticles(particleSystem_, deltaTimeSeconds);
    }

    particleSystem_.update(deltaTimeSeconds);
    const double fallbackGroundY = groundPlaneY();
    particleSystem_.handleRainCollisions(
        [this, fallbackGroundY](const tinyray::Particle& particle, tinyray::Vec3& impactPoint) {
            const double hitY = collisionHeightAt(particle.position.x, particle.position.z, fallbackGroundY);
            if (particle.position.y > hitY) {
                return false;
            }
            impactPoint = tinyray::Vec3(particle.position.x, hitY, particle.position.z);
            return true;
        },
        [this](const tinyray::Vec3& impactPoint) {
            if (rainSettings_.splashEnabled) {
                splashEmitter_.emitSplash(particleSystem_, impactPoint);
            }
        });

    if (rainSettings_.rainEnabled || hadParticles || !particleSystem_.empty()) {
        invalidateRealtimeAccumulation();
        update();
    }
}

double RealTimeRenderWidget::collisionHeightAt(double x, double z, double fallbackGroundY) const
{
    double height = fallbackGroundY;
    const tinyray::Scene& scene = previewScene();
    for (const auto& object : scene.objects) {
        if (const auto* sphere = dynamic_cast<const tinyray::Sphere*>(object.get())) {
            const double dx = x - sphere->center.x;
            const double dz = z - sphere->center.z;
            const double radius = std::max(sphere->radius, 0.0);
            const double r2 = dx * dx + dz * dz;
            if (r2 <= radius * radius) {
                height = std::max(height, sphere->center.y + std::sqrt(std::max(0.0, radius * radius - r2)));
            }
        } else if (const auto* box = dynamic_cast<const tinyray::Box*>(object.get())) {
            const tinyray::Vec3 minimum = box->minCorner();
            const tinyray::Vec3 maximum = box->maxCorner();
            if (x >= minimum.x && x <= maximum.x && z >= minimum.z && z <= maximum.z) {
                height = std::max(height, maximum.y);
            }
        } else if (const auto* cylinder = dynamic_cast<const tinyray::Cylinder*>(object.get())) {
            const double dx = x - cylinder->center.x;
            const double dz = z - cylinder->center.z;
            const double radius = std::max(std::abs(cylinder->radius), 0.0);
            if (dx * dx + dz * dz <= radius * radius) {
                height = std::max(height, cylinder->center.y + std::max(std::abs(cylinder->height), 0.0) * 0.5);
            }
        }
    }
    return height;
}

double RealTimeRenderWidget::groundPlaneY() const
{
    const tinyray::Scene& scene = previewScene();
    double groundY = -0.5;
    bool foundGround = false;
    for (const auto& object : scene.objects) {
        const auto* plane = dynamic_cast<const tinyray::Plane*>(object.get());
        if (plane == nullptr) {
            continue;
        }
        tinyray::Vec3 normal = plane->normal.normalized();
        if (normal.nearZero() || std::abs(normal.y) < 0.65) {
            continue;
        }
        groundY = foundGround ? std::max(groundY, plane->point.y) : plane->point.y;
        foundGround = true;
    }
    return groundY;
}
