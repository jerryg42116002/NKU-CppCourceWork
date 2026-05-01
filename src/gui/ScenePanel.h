#pragma once

#include <QColor>
#include <QWidget>

#include "core/Scene.h"

class QComboBox;
class QCheckBox;
class QDoubleSpinBox;
class QListWidget;
class QPushButton;
class QSpinBox;
class QStackedWidget;

class ScenePanel : public QWidget
{
    Q_OBJECT

public:
    explicit ScenePanel(QWidget* parent = nullptr);

    void setScene(const tinyray::Scene& scene);
    void setSelectedObjectId(int objectId);
    void setSelectedLightIndex(int lightIndex);
    tinyray::Scene scene() const;

signals:
    void sceneChanged(const tinyray::Scene& scene);

private slots:
    void handlePresetChanged(int index);
    void handleSelectionChanged();
    void handleAddSphere();
    void handleAddBox();
    void handleAddCylinder();
    void handleAddPlane();
    void handleAddLight();
    void handleAddAreaLight();
    void handleDeleteSelected();
    void handleSphereChanged();
    void handleBoxChanged();
    void handleCylinderChanged();
    void handlePlaneChanged();
    void handleLightChanged();
    void handleBloomChanged();
    void handleSoftShadowChanged();
    void chooseSphereAlbedo();
    void chooseSphereEmissionColor();
    void chooseBoxAlbedo();
    void chooseBoxEmissionColor();
    void chooseCylinderAlbedo();
    void chooseCylinderEmissionColor();
    void choosePlaneAlbedo();
    void choosePlaneEmissionColor();
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
    void syncBloomControls();

    void setSphereEditor(const tinyray::Sphere& sphere);
    void setBoxEditor(const tinyray::Box& box);
    void setCylinderEditor(const tinyray::Cylinder& cylinder);
    void setPlaneEditor(const tinyray::Plane& plane);
    void setLightEditor(const tinyray::Light& light);

    tinyray::Sphere* selectedSphere();
    tinyray::Box* selectedBox();
    tinyray::Cylinder* selectedCylinder();
    tinyray::Plane* selectedPlane();
    tinyray::Light* selectedLight();

    QDoubleSpinBox* createDoubleSpinBox(double minimum, double maximum, double step);
    void updateColorButton(QPushButton* button, const QColor& color);

    tinyray::Material readSphereMaterial() const;
    tinyray::Material readBoxMaterial() const;
    tinyray::Material readCylinderMaterial() const;
    tinyray::Material readPlaneMaterial() const;
    void writeMaterialToControls(const tinyray::Material& material,
                                 QComboBox* typeCombo,
                                 QDoubleSpinBox* roughnessSpin,
                                 QDoubleSpinBox* refractiveSpin,
                                 QColor& colorCache,
                                 QPushButton* colorButton);
    void writeMaterialToControls(const tinyray::Material& material,
                                 QComboBox* typeCombo,
                                 QDoubleSpinBox* roughnessSpin,
                                 QDoubleSpinBox* refractiveSpin,
                                 QColor& emissionColorCache,
                                 QPushButton* emissionColorButton,
                                 QDoubleSpinBox* emissionStrengthSpin,
                                 QColor& colorCache,
                                 QPushButton* colorButton);

    tinyray::Scene scene_;
    bool updating_ = false;

    QComboBox* presetComboBox_ = nullptr;
    QListWidget* objectList_ = nullptr;
    QPushButton* addSphereButton_ = nullptr;
    QPushButton* addBoxButton_ = nullptr;
    QPushButton* addCylinderButton_ = nullptr;
    QPushButton* addPlaneButton_ = nullptr;
    QPushButton* addLightButton_ = nullptr;
    QPushButton* addAreaLightButton_ = nullptr;
    QPushButton* deleteButton_ = nullptr;
    QStackedWidget* editorStack_ = nullptr;

    QCheckBox* softShadowsEnabled_ = nullptr;
    QSpinBox* areaLightSamples_ = nullptr;

    QCheckBox* bloomEnabled_ = nullptr;
    QDoubleSpinBox* bloomExposure_ = nullptr;
    QDoubleSpinBox* bloomThreshold_ = nullptr;
    QDoubleSpinBox* bloomStrength_ = nullptr;
    QSpinBox* bloomBlurPassCount_ = nullptr;

    QDoubleSpinBox* sphereCenterX_ = nullptr;
    QDoubleSpinBox* sphereCenterY_ = nullptr;
    QDoubleSpinBox* sphereCenterZ_ = nullptr;
    QDoubleSpinBox* sphereRadius_ = nullptr;
    QComboBox* sphereMaterialType_ = nullptr;
    QPushButton* sphereAlbedoButton_ = nullptr;
    QDoubleSpinBox* sphereRoughness_ = nullptr;
    QDoubleSpinBox* sphereRefractiveIndex_ = nullptr;
    QPushButton* sphereEmissionColorButton_ = nullptr;
    QDoubleSpinBox* sphereEmissionStrength_ = nullptr;
    QColor sphereAlbedo_ = QColor(204, 204, 204);
    QColor sphereEmissionColor_ = QColor(255, 153, 26);

    QDoubleSpinBox* boxCenterX_ = nullptr;
    QDoubleSpinBox* boxCenterY_ = nullptr;
    QDoubleSpinBox* boxCenterZ_ = nullptr;
    QDoubleSpinBox* boxSizeX_ = nullptr;
    QDoubleSpinBox* boxSizeY_ = nullptr;
    QDoubleSpinBox* boxSizeZ_ = nullptr;
    QComboBox* boxMaterialType_ = nullptr;
    QPushButton* boxAlbedoButton_ = nullptr;
    QDoubleSpinBox* boxRoughness_ = nullptr;
    QDoubleSpinBox* boxRefractiveIndex_ = nullptr;
    QPushButton* boxEmissionColorButton_ = nullptr;
    QDoubleSpinBox* boxEmissionStrength_ = nullptr;
    QColor boxAlbedo_ = QColor(204, 204, 204);
    QColor boxEmissionColor_ = QColor(255, 153, 26);

    QDoubleSpinBox* cylinderCenterX_ = nullptr;
    QDoubleSpinBox* cylinderCenterY_ = nullptr;
    QDoubleSpinBox* cylinderCenterZ_ = nullptr;
    QDoubleSpinBox* cylinderRadius_ = nullptr;
    QDoubleSpinBox* cylinderHeight_ = nullptr;
    QComboBox* cylinderMaterialType_ = nullptr;
    QPushButton* cylinderAlbedoButton_ = nullptr;
    QDoubleSpinBox* cylinderRoughness_ = nullptr;
    QDoubleSpinBox* cylinderRefractiveIndex_ = nullptr;
    QPushButton* cylinderEmissionColorButton_ = nullptr;
    QDoubleSpinBox* cylinderEmissionStrength_ = nullptr;
    QColor cylinderAlbedo_ = QColor(204, 204, 204);
    QColor cylinderEmissionColor_ = QColor(255, 153, 26);

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
    QPushButton* planeEmissionColorButton_ = nullptr;
    QDoubleSpinBox* planeEmissionStrength_ = nullptr;
    QColor planeAlbedo_ = QColor(204, 204, 204);
    QColor planeEmissionColor_ = QColor(255, 153, 26);

    QDoubleSpinBox* lightPositionX_ = nullptr;
    QDoubleSpinBox* lightPositionY_ = nullptr;
    QDoubleSpinBox* lightPositionZ_ = nullptr;
    QDoubleSpinBox* lightNormalX_ = nullptr;
    QDoubleSpinBox* lightNormalY_ = nullptr;
    QDoubleSpinBox* lightNormalZ_ = nullptr;
    QDoubleSpinBox* lightWidth_ = nullptr;
    QDoubleSpinBox* lightHeight_ = nullptr;
    QPushButton* lightColorButton_ = nullptr;
    QDoubleSpinBox* lightIntensity_ = nullptr;
    QColor lightColor_ = QColor(255, 255, 255);
};
