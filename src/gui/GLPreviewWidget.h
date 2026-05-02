#pragma once

#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>
#include <QPoint>
#include <QString>

#include <memory>

#include "core/OrbitCamera.h"
#include "core/Ray.h"
#include "core/Scene.h"
#include "core/Vec3.h"
#include "interaction/DragMode.h"

class QMouseEvent;
class QWheelEvent;

class GLPreviewWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit GLPreviewWidget(QWidget* parent = nullptr);

    void setScene(const tinyray::Scene& scene);
    void setSelectedObjectId(int objectId);
    void setObjectDragMode(tinyray::ObjectDragMode mode);
    void setTurntableEnabled(bool enabled);
    bool turntableEnabled() const;
    void setTurntableSpeed(double degreesPerSecond);
    double turntableSpeed() const;
    void setTurntableDirection(tinyray::TurntableDirection direction);
    void setTurntableTargetMode(tinyray::TurntableTargetMode mode);
    tinyray::TurntableTargetMode turntableTargetMode() const;
    void setTurntableCustomTarget(const tinyray::Vec3& target);
    void advanceTurntable(double deltaTimeSeconds);
    void resetView();
    bool focusSelectedObject();
    tinyray::Camera currentCameraSnapshot() const;
    int selectedObjectId() const;
    const tinyray::OrbitCamera& orbitCamera() const;

signals:
    void objectSelected(int objectId);
    void lightSelected(int lightIndex);
    void objectMoved(int objectId, double x, double y, double z);
    void lightMoved(int lightIndex, double x, double y, double z);
    void turntablePausedByUserInput();
    void interactionStatusChanged(const QString& status);

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
    void initializePathTracer();
    QSize renderBufferSize() const;
    void recreateAccumulationBuffers();
    void resetPathTraceAccumulation();
    void renderPathTracedScene();
    void uploadPathTraceScene(QOpenGLShaderProgram& program);
    void drawFullScreenTriangle(QOpenGLShaderProgram& program);
    void drawLightMarkersOverlay();
    void drawOverlayLabels();
    void drawScene();
    void drawBox(const tinyray::Box& box);
    void drawCylinder(const tinyray::Cylinder& cylinder);
    void drawSphere(const tinyray::Sphere& sphere);
    void drawPlane(const tinyray::Plane& plane);
    void drawLightMarker(const tinyray::Light& light, bool selected);
    void applyMaterial(const tinyray::Material& material, bool selected);
    bool isSelectionClick(const QPoint& releasePosition) const;
    void updateSelectionFromClick(const QPoint& screenPosition);
    bool beginObjectDrag(const QPoint& screenPosition);
    void updateObjectDrag(const QPoint& screenPosition);
    void finishObjectDrag();
    void pauseTurntableForUserInput();
    void updateTurntableTarget();
    tinyray::Vec3 sceneCenter() const;
    bool selectedObjectCenter(tinyray::Vec3& center) const;
    int pickLight(const tinyray::Ray& ray) const;
    bool draggableObjectCenter(int objectId, tinyray::Vec3& center) const;
    bool setDraggableObjectCenter(int objectId, const tinyray::Vec3& center);
    bool rayIntersectsDragPlane(const tinyray::Ray& ray, tinyray::Vec3& hitPoint) const;
    tinyray::Vec3 axisDragCenter(const QPoint& screenPosition) const;
    tinyray::Vec3 dragAxisVector() const;

    tinyray::Scene scene_;
    tinyray::OrbitCamera camera_;
    std::unique_ptr<QOpenGLShaderProgram> pathTraceProgram_;
    std::unique_ptr<QOpenGLShaderProgram> displayProgram_;
    std::unique_ptr<QOpenGLFramebufferObject> accumulationBuffers_[2];
    int accumulationReadIndex_ = 0;
    int pathTraceFrameIndex_ = 0;
    bool pathTracerReady_ = false;
    QPoint pressMousePosition_;
    QPoint lastMousePosition_;
    int selectedObjectId_ = -1;
    int selectedLightIndex_ = -1;
    int draggedObjectId_ = -1;
    int draggedLightIndex_ = -1;
    bool isDraggingObject_ = false;
    bool isDraggingLight_ = false;
    bool turntablePausedByUserInputPending_ = false;
    tinyray::Vec3 dragStartObjectCenter_;
    tinyray::Vec3 dragStartGroundPoint_;
    tinyray::Vec3 dragOffset_;
    tinyray::ObjectDragMode objectDragMode_ = tinyray::ObjectDragMode::PlaneXZ;
    DragMode dragMode_ = DragMode::None;
};
