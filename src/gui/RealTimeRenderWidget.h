#pragma once

#include <QElapsedTimer>
#include <QTimer>

#include "gui/GLPreviewWidget.h"

class RealTimeRenderWidget : public GLPreviewWidget
{
    Q_OBJECT

public:
    explicit RealTimeRenderWidget(QWidget* parent = nullptr);

private slots:
    void handleTurntableTick();

private:
    QTimer turntableTimer_;
    QElapsedTimer turntableClock_;
};
