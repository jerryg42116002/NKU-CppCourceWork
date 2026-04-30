#pragma once

#include "gui/GLPreviewWidget.h"

class RealTimeRenderWidget : public GLPreviewWidget
{
    Q_OBJECT

public:
    explicit RealTimeRenderWidget(QWidget* parent = nullptr);
};
