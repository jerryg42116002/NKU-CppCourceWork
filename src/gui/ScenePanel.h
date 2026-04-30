#pragma once

#include <QColor>
#include <QWidget>

#include "core/Scene.h"

class QComboBox;
class QDoubleSpinBox;
class QListWidget;
class QPushButton;
class QStackedWidget;

class ScenePanel : public QWidget
{
    Q_OBJECT

public:
    explicit ScenePanel(QWidget* parent = nullptr);

    void setScene(const tinyray::Scene& scene);
    tinyray::Scene scene() const;

signals:
    void sceneChanged(const tinyray::Scene& scene);

private slots:
    void handlePresetChanged(int index);
    void handleSelectionChanged();
    void handleAddSphere();
    void handleAddPlane();
    void handleAddLight();
    void handleDeleteSelected();
    void handleSphereChanged();
    void handlePlaneChanged();
    void handleLightChanged();
    void chooseSphereAlbedo();
    void choosePlaneAlbedo();
    void chooseLightColor();

private:
    enum class SelectionKind
    {
        None,
        Object,
        Light
    };

    void createUi();
    void rebuildObjectList(int preferredKind = 0, int preferredIndex = -1);
    void loadSelectedEditor();
    void emitSceneChanged();

    void setSphereEditor(const tinyray::Sphere& sphere);
    void setPlaneEditor(const tinyray::Plane& plane);
    void setLightEditor(const tinyray::Light& light);

    tinyray::Sphere* selectedSphere();
    tinyray::Plane* selectedPlane();
    tinyray::Light* selectedLight();

    QDoubleSpinBox* createDoubleSpinBox(double minimum, double maximum, double step);
    void updateColorButton(QPushButton* button, const QColor& color);

    tinyray::Material readSphereMaterial() const;
    tinyray::Material readPlaneMaterial() const;
    void writeMaterialToControls(const tinyray::Material& material,
                                 QComboBox* typeCombo,
                                 QDoubleSpinBox* roughnessSpin,
                                 QDoubleSpinBox* refractiveSpin,
                                 QColor& colorCache,
                                 QPushButton* colorButton);

    tinyray::Scene scene_;
    bool updating_ = false;

    QComboBox* presetComboBox_ = nullptr;
    QListWidget* objectList_ = nullptr;
    QPushButton* addSphereButton_ = nullptr;
    QPushButton* addPlaneButton_ = nullptr;
    QPushButton* addLightButton_ = nullptr;
    QPushButton* deleteButton_ = nullptr;
    QStackedWidget* editorStack_ = nullptr;

    QDoubleSpinBox* sphereCenterX_ = nullptr;
    QDoubleSpinBox* sphereCenterY_ = nullptr;
    QDoubleSpinBox* sphereCenterZ_ = nullptr;
    QDoubleSpinBox* sphereRadius_ = nullptr;
    QComboBox* sphereMaterialType_ = nullptr;
    QPushButton* sphereAlbedoButton_ = nullptr;
    QDoubleSpinBox* sphereRoughness_ = nullptr;
    QDoubleSpinBox* sphereRefractiveIndex_ = nullptr;
    QColor sphereAlbedo_ = QColor(204, 204, 204);

    QDoubleSpinBox* planePointX_ = nullptr;
    QDoubleSpinBox* planePointY_ = nullptr;
    QDoubleSpinBox* planePointZ_ = nullptr;
    QDoubleSpinBox* planeNormalX_ = nullptr;
    QDoubleSpinBox* planeNormalY_ = nullptr;
    QDoubleSpinBox* planeNormalZ_ = nullptr;
    QComboBox* planeMaterialType_ = nullptr;
    QPushButton* planeAlbedoButton_ = nullptr;
    QDoubleSpinBox* planeRoughness_ = nullptr;
    QDoubleSpinBox* planeRefractiveIndex_ = nullptr;
    QColor planeAlbedo_ = QColor(204, 204, 204);

    QDoubleSpinBox* lightPositionX_ = nullptr;
    QDoubleSpinBox* lightPositionY_ = nullptr;
    QDoubleSpinBox* lightPositionZ_ = nullptr;
    QPushButton* lightColorButton_ = nullptr;
    QDoubleSpinBox* lightIntensity_ = nullptr;
    QColor lightColor_ = QColor(255, 255, 255);
};
