#include "gui/RenderWidget.h"

#include <QColor>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QSizePolicy>

RenderWidget::RenderWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(640, 360);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAutoFillBackground(false);
}

void RenderWidget::setImage(const QImage& image)
{
    image_ = image;
    update();
}

void RenderWidget::clearImage()
{
    image_ = QImage();
    update();
}

const QImage& RenderWidget::image() const
{
    return image_;
}

QSize RenderWidget::sizeHint() const
{
    return QSize(900, 540);
}

void RenderWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), QColor(24, 26, 30));

    QRect previewRect = rect().adjusted(12, 12, -12, -12);
    painter.setPen(QPen(QColor(58, 62, 70), 1));
    painter.drawRect(previewRect);

    if (image_.isNull()) {
        return;
    }

    QSize scaledSize = image_.size();
    scaledSize.scale(previewRect.size(), Qt::KeepAspectRatio);

    QRect targetRect(QPoint(0, 0), scaledSize);
    targetRect.moveCenter(previewRect.center());

    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.drawImage(targetRect, image_);

    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.setPen(QPen(QColor(90, 95, 104), 1));
    painter.drawRect(targetRect);
}
