#include "gui/RealTimeRenderWidget.h"

RealTimeRenderWidget::RealTimeRenderWidget(QWidget* parent)
    : GLPreviewWidget(parent)
{
    turntableTimer_.setInterval(16);
    connect(&turntableTimer_, &QTimer::timeout,
            this, &RealTimeRenderWidget::handleTurntableTick);
    turntableClock_.start();
    turntableTimer_.start();
}

void RealTimeRenderWidget::handleTurntableTick()
{
    const qint64 elapsedMilliseconds = turntableClock_.restart();
    if (!turntableEnabled() || elapsedMilliseconds <= 0) {
        return;
    }

    advanceTurntable(static_cast<double>(elapsedMilliseconds) / 1000.0);
}
