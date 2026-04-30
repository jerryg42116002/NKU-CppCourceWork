#pragma once

#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QPoint>

#include "core/OrbitCamera.h"
#include "core/Ray.h"
#include "core/Scene.h"
#include "core/Vec3.h"

class QMouseEvent;
class QWheelEvent;

class GLPreviewWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit GLPreviewWidget(QWidget* parent = nullptr);

    void setScene(const tinyray::Scene& scene);
    void setSelectedObjectId(int objectId);
    int selectedObjectId() const;
    const tinyray::OrbitCamera& orbitCamera() const;

signals:
    void objectSelected(int objectId);
    void sphereMoved(int objectId, double x, double y, double z);

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    enum class DragMode
    {
        None,
        Orbit,
        Pan
    };

    void setupCameraMatrices();
    void setupLights();
    void drawScene();
    void drawSphere(const tinyray::Sphere& sphere);
    void drawPlane(const tinyray::Plane& plane);
    void drawLightMarker(const tinyray::Light& light);
    void applyMaterial(const tinyray::Material& material, bool selected);
    bool isSelectionClick(const QPoint& releasePosition) const;
    void updateSelectionFromClick(const QPoint& screenPosition);
    bool beginObjectDrag(const QPoint& screenPosition);
    void updateObjectDrag(const QPoint& screenPosition);
    void finishObjectDrag();
    bool rayIntersectsGroundPlane(const tinyray::Ray& ray, tinyray::Vec3& hitPoint) const;

    tinyray::Scene scene_;
    tinyray::OrbitCamera camera_;
    QPoint pressMousePosition_;
    QPoint lastMousePosition_;
    int selectedObjectId_ = -1;
    int draggedObjectId_ = -1;
    bool isDraggingObject_ = false;
    tinyray::Vec3 dragStartObjectCenter_;
    tinyray::Vec3 dragStartGroundPoint_;
    tinyray::Vec3 dragOffset_;
    DragMode dragMode_ = DragMode::None;
};
