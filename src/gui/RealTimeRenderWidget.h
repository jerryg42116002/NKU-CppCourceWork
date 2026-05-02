#pragma once

#include <QElapsedTimer>
#include <QTimer>

#include "gui/GLPreviewWidget.h"
#include "particle/Emitter.h"
#include "particle/ParticleSystem.h"

class RealTimeRenderWidget : public GLPreviewWidget
{
    Q_OBJECT

public:
    explicit RealTimeRenderWidget(QWidget* parent = nullptr);
    void setRainSettings(const tinyray::RainSettings& settings);
    tinyray::RainSettings rainSettings() const;
    void clearParticles();

private slots:
    void handleTurntableTick();

protected:
    void drawRealtimeOverlay() override;

private:
    void updateParticles(double deltaTimeSeconds);
    double collisionHeightAt(double x, double z, double fallbackGroundY) const;
    double groundPlaneY() const;

    QTimer turntableTimer_;
    QElapsedTimer turntableClock_;
    tinyray::ParticleSystem particleSystem_;
    tinyray::RainEmitter rainEmitter_;
    tinyray::SplashEmitter splashEmitter_;
    tinyray::RainSettings rainSettings_;
};
