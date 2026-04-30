#pragma once

#include <QImage>
#include <QWidget>

class RenderWidget : public QWidget
{
public:
    explicit RenderWidget(QWidget* parent = nullptr);

    void setImage(const QImage& image);
    void clearImage();
    const QImage& image() const;

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QImage image_;
};
