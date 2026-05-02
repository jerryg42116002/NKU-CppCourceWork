#include "gui/ScenePanel.h"

#include <algorithm>
#include <cstddef>

#include <QColorDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVariant>
#include <QVBoxLayout>

#include "core/Box.h"
#include "core/Cylinder.h"
#include "core/Plane.h"
#include "core/Sphere.h"

namespace {

tinyray::Vec3 colorToVec3(const QColor& color)
{
    return tinyray::Vec3(color.redF(), color.greenF(), color.blueF());
}

QColor vec3ToColor(const tinyray::Vec3& color)
{
    return QColor::fromRgbF(
        std::clamp(color.x, 0.0, 1.0),
        std::clamp(color.y, 0.0, 1.0),
        std::clamp(color.z, 0.0, 1.0));
}

int materialTypeToIndex(tinyray::MaterialType type)
{
    switch (type) {
    case tinyray::MaterialType::Emissive:
        return 3;
    case tinyray::MaterialType::Metal:
        return 1;
    case tinyray::MaterialType::Glass:
        return 2;
    case tinyray::MaterialType::Diffuse:
    default:
        return 0;
    }
}

tinyray::MaterialType materialTypeFromIndex(int index)
{
    if (index == 1) {
        return tinyray::MaterialType::Metal;
    }
    if (index == 2) {
        return tinyray::MaterialType::Glass;
    }
    if (index == 3) {
        return tinyray::MaterialType::Emissive;
    }
    return tinyray::MaterialType::Diffuse;
}

QString materialName(tinyray::MaterialType type)
{
    switch (type) {
    case tinyray::MaterialType::Emissive:
        return QStringLiteral("Emissive");
    case tinyray::MaterialType::Metal:
        return QStringLiteral("Metal");
    case tinyray::MaterialType::Glass:
        return QStringLiteral("Glass");
    case tinyray::MaterialType::Diffuse:
    default:
        return QStringLiteral("Diffuse");
    }
}

tinyray::Material makeDiffuseMaterial(const tinyray::Vec3& albedo)
{
    tinyray::Material material;
    material.type = tinyray::MaterialType::Diffuse;
    material.albedo = albedo;
    return material;
}

} // namespace

ScenePanel::ScenePanel(QWidget* parent)
    : QWidget(parent)
{
    createUi();
    setScene(tinyray::Scene::createDefaultScene());
}

void ScenePanel::setScene(const tinyray::Scene& scene)
{
    updating_ = true;
    scene_ = scene;
    syncEnvironmentControls();
    syncBloomControls();
    rebuildObjectList();
    updating_ = false;
    loadSelectedEditor();
}

tinyray::Scene ScenePanel::scene() const
{
    return scene_;
}

void ScenePanel::createUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    auto* presetGroup = new QGroupBox(QStringLiteral("Presets"), this);
    auto* presetLayout = new QVBoxLayout(presetGroup);
    presetComboBox_ = new QComboBox(presetGroup);
    presetComboBox_->addItem(QStringLiteral("Colored Lights Demo"), QStringLiteral("colored"));
    presetComboBox_->addItem(QStringLiteral("Reflection Demo"), QStringLiteral("reflection"));
    presetComboBox_->addItem(QStringLiteral("Glass Demo"), QStringLiteral("glass"));
    presetLayout->addWidget(presetComboBox_);
    layout->addWidget(presetGroup);

    auto* environmentGroup = new QGroupBox(QStringLiteral("Environment"), this);
    auto* environmentLayout = new QFormLayout(environmentGroup);
    environmentMode_ = new QComboBox(environmentGroup);
    environmentMode_->addItem(QStringLiteral("Gradient"), static_cast<int>(tinyray::EnvironmentMode::Gradient));
    environmentMode_->addItem(QStringLiteral("Solid Color"), static_cast<int>(tinyray::EnvironmentMode::SolidColor));
    environmentMode_->addItem(QStringLiteral("Image"), static_cast<int>(tinyray::EnvironmentMode::Image));
    environmentMode_->addItem(QStringLiteral("HDR Image"), static_cast<int>(tinyray::EnvironmentMode::HDRImage));
    loadEnvironmentImageButton_ = new QPushButton(QStringLiteral("Load Image"), environmentGroup);
    resetEnvironmentButton_ = new QPushButton(QStringLiteral("Reset"), environmentGroup);
    environmentExposure_ = createDoubleSpinBox(0.0, 8.0, 0.05);
    environmentIntensity_ = createDoubleSpinBox(0.0, 8.0, 0.05);
    environmentRotationY_ = createDoubleSpinBox(-6.283, 6.283, 0.05);
    environmentLayout->addRow(QStringLiteral("Mode"), environmentMode_);
    environmentLayout->addRow(QStringLiteral("Image"), loadEnvironmentImageButton_);
    environmentLayout->addRow(QStringLiteral("Exposure"), environmentExposure_);
    environmentLayout->addRow(QStringLiteral("Intensity"), environmentIntensity_);
    environmentLayout->addRow(QStringLiteral("Rotation Y"), environmentRotationY_);
    environmentLayout->addRow(QStringLiteral("Reset"), resetEnvironmentButton_);
    layout->addWidget(environmentGroup);

    auto* overlayGroup = new QGroupBox(QStringLiteral("Overlay"), this);
    auto* overlayLayout = new QVBoxLayout(overlayGroup);
    overlayLabelsEnabled_ = new QCheckBox(QStringLiteral("Show Overlay Labels"), overlayGroup);
    overlayShowPosition_ = new QCheckBox(QStringLiteral("Show Position"), overlayGroup);
    overlayShowMaterialInfo_ = new QCheckBox(QStringLiteral("Show Material Info"), overlayGroup);
    overlayLayout->addWidget(overlayLabelsEnabled_);
    overlayLayout->addWidget(overlayShowPosition_);
    overlayLayout->addWidget(overlayShowMaterialInfo_);
    layout->addWidget(overlayGroup);

    auto* postProcessGroup = new QGroupBox(QStringLiteral("Post Processing"), this);
    auto* postProcessLayout = new QFormLayout(postProcessGroup);
    bloomEnabled_ = new QCheckBox(QStringLiteral("Bloom"), postProcessGroup);
    bloomExposure_ = createDoubleSpinBox(0.05, 8.0, 0.05);
    bloomThreshold_ = createDoubleSpinBox(0.0, 20.0, 0.05);
    bloomStrength_ = createDoubleSpinBox(0.0, 5.0, 0.05);
    bloomBlurPassCount_ = new QSpinBox(postProcessGroup);
    bloomBlurPassCount_->setRange(1, 16);
    bloomBlurPassCount_->setSingleStep(1);
    postProcessLayout->addRow(QStringLiteral("Enabled"), bloomEnabled_);
    postProcessLayout->addRow(QStringLiteral("Exposure"), bloomExposure_);
    postProcessLayout->addRow(QStringLiteral("Threshold"), bloomThreshold_);
    postProcessLayout->addRow(QStringLiteral("Strength"), bloomStrength_);
    postProcessLayout->addRow(QStringLiteral("Blur Passes"), bloomBlurPassCount_);
    layout->addWidget(postProcessGroup);

    auto* shadowGroup = new QGroupBox(QStringLiteral("Shadows"), this);
    auto* shadowLayout = new QFormLayout(shadowGroup);
    softShadowsEnabled_ = new QCheckBox(QStringLiteral("Soft Shadows"), shadowGroup);
    areaLightSamples_ = new QSpinBox(shadowGroup);
    areaLightSamples_->setRange(1, 128);
    areaLightSamples_->setSingleStep(1);
    shadowLayout->addRow(QStringLiteral("Enabled"), softShadowsEnabled_);
    shadowLayout->addRow(QStringLiteral("Area Samples"), areaLightSamples_);
    layout->addWidget(shadowGroup);

    auto* sceneGroup = new QGroupBox(QStringLiteral("Scene"), this);
    auto* sceneLayout = new QVBoxLayout(sceneGroup);
    objectList_ = new QListWidget(sceneGroup);
    objectList_->setMinimumHeight(130);
    sceneLayout->addWidget(objectList_);

    auto* buttonRow = new QHBoxLayout();
    addSphereButton_ = new QPushButton(QStringLiteral("Sphere"), sceneGroup);
    addBoxButton_ = new QPushButton(QStringLiteral("Box"), sceneGroup);
    addCylinderButton_ = new QPushButton(QStringLiteral("Cyl"), sceneGroup);
    addPlaneButton_ = new QPushButton(QStringLiteral("Plane"), sceneGroup);
    addLightButton_ = new QPushButton(QStringLiteral("Point"), sceneGroup);
    addAreaLightButton_ = new QPushButton(QStringLiteral("Area"), sceneGroup);
    deleteButton_ = new QPushButton(QStringLiteral("Delete"), sceneGroup);
    buttonRow->addWidget(addSphereButton_);
    buttonRow->addWidget(addBoxButton_);
    buttonRow->addWidget(addCylinderButton_);
    buttonRow->addWidget(addPlaneButton_);
    buttonRow->addWidget(addLightButton_);
    buttonRow->addWidget(addAreaLightButton_);
    buttonRow->addWidget(deleteButton_);
    sceneLayout->addLayout(buttonRow);
    layout->addWidget(sceneGroup, 1);

    auto* editorGroup = new QGroupBox(QStringLiteral("Inspector"), this);
    auto* editorLayout = new QVBoxLayout(editorGroup);
    editorStack_ = new QStackedWidget(editorGroup);
    editorStack_->addWidget(new QLabel(QStringLiteral("Select an object or light."), editorStack_));

    auto* spherePage = new QWidget(editorStack_);
    auto* sphereLayout = new QFormLayout(spherePage);
    sphereCenterX_ = createDoubleSpinBox(-100.0, 100.0, 0.1);
    sphereCenterY_ = createDoubleSpinBox(-100.0, 100.0, 0.1);
    sphereCenterZ_ = createDoubleSpinBox(-100.0, 100.0, 0.1);
    sphereRadius_ = createDoubleSpinBox(0.01, 100.0, 0.05);
    sphereMaterialType_ = new QComboBox(spherePage);
    sphereMaterialType_->addItems({QStringLiteral("Diffuse"), QStringLiteral("Metal"), QStringLiteral("Glass"), QStringLiteral("Emissive")});
    sphereAlbedoButton_ = new QPushButton(spherePage);
    sphereUseTexture_ = new QCheckBox(QStringLiteral("Enable Texture"), spherePage);
    sphereTextureButton_ = new QPushButton(QStringLiteral("Load Texture"), spherePage);
    sphereTextureScale_ = createDoubleSpinBox(0.001, 100.0, 0.25);
    sphereTextureOffsetU_ = createDoubleSpinBox(-100.0, 100.0, 0.05);
    sphereTextureOffsetV_ = createDoubleSpinBox(-100.0, 100.0, 0.05);
    sphereTextureStrength_ = createDoubleSpinBox(0.0, 1.0, 0.05);
    sphereRoughness_ = createDoubleSpinBox(0.0, 1.0, 0.05);
    sphereRefractiveIndex_ = createDoubleSpinBox(1.0, 3.0, 0.05);
    sphereEmissionColorButton_ = new QPushButton(spherePage);
    sphereEmissionStrength_ = createDoubleSpinBox(0.0, 20.0, 0.25);
    sphereLayout->addRow(QStringLiteral("Center X"), sphereCenterX_);
    sphereLayout->addRow(QStringLiteral("Center Y"), sphereCenterY_);
    sphereLayout->addRow(QStringLiteral("Center Z"), sphereCenterZ_);
    sphereLayout->addRow(QStringLiteral("Radius"), sphereRadius_);
    sphereLayout->addRow(QStringLiteral("Material"), sphereMaterialType_);
    sphereLayout->addRow(QStringLiteral("Albedo"), sphereAlbedoButton_);
    sphereLayout->addRow(QStringLiteral("Texture"), sphereUseTexture_);
    sphereLayout->addRow(QStringLiteral("Texture Path"), sphereTextureButton_);
    sphereLayout->addRow(QStringLiteral("Texture Scale"), sphereTextureScale_);
    sphereLayout->addRow(QStringLiteral("Texture Offset U"), sphereTextureOffsetU_);
    sphereLayout->addRow(QStringLiteral("Texture Offset V"), sphereTextureOffsetV_);
    sphereLayout->addRow(QStringLiteral("Texture Strength"), sphereTextureStrength_);
    sphereLayout->addRow(QStringLiteral("Roughness"), sphereRoughness_);
    sphereLayout->addRow(QStringLiteral("Refractive Index"), sphereRefractiveIndex_);
    sphereLayout->addRow(QStringLiteral("Emission Color"), sphereEmissionColorButton_);
    sphereLayout->addRow(QStringLiteral("Emission Strength"), sphereEmissionStrength_);
    editorStack_->addWidget(spherePage);

    auto* boxPage = new QWidget(editorStack_);
    auto* boxLayout = new QFormLayout(boxPage);
    boxCenterX_ = createDoubleSpinBox(-100.0, 100.0, 0.1);
    boxCenterY_ = createDoubleSpinBox(-100.0, 100.0, 0.1);
    boxCenterZ_ = createDoubleSpinBox(-100.0, 100.0, 0.1);
    boxSizeX_ = createDoubleSpinBox(0.01, 100.0, 0.05);
    boxSizeY_ = createDoubleSpinBox(0.01, 100.0, 0.05);
    boxSizeZ_ = createDoubleSpinBox(0.01, 100.0, 0.05);
    boxMaterialType_ = new QComboBox(boxPage);
    boxMaterialType_->addItems({QStringLiteral("Diffuse"), QStringLiteral("Metal"), QStringLiteral("Glass"), QStringLiteral("Emissive")});
    boxAlbedoButton_ = new QPushButton(boxPage);
    boxUseTexture_ = new QCheckBox(QStringLiteral("Enable Texture"), boxPage);
    boxTextureButton_ = new QPushButton(QStringLiteral("Load Texture"), boxPage);
    boxTextureScale_ = createDoubleSpinBox(0.001, 100.0, 0.25);
    boxTextureOffsetU_ = createDoubleSpinBox(-100.0, 100.0, 0.05);
    boxTextureOffsetV_ = createDoubleSpinBox(-100.0, 100.0, 0.05);
    boxTextureStrength_ = createDoubleSpinBox(0.0, 1.0, 0.05);
    boxRoughness_ = createDoubleSpinBox(0.0, 1.0, 0.05);
    boxRefractiveIndex_ = createDoubleSpinBox(1.0, 3.0, 0.05);
    boxEmissionColorButton_ = new QPushButton(boxPage);
    boxEmissionStrength_ = createDoubleSpinBox(0.0, 20.0, 0.25);
    boxLayout->addRow(QStringLiteral("Center X"), boxCenterX_);
    boxLayout->addRow(QStringLiteral("Center Y"), boxCenterY_);
    boxLayout->addRow(QStringLiteral("Center Z"), boxCenterZ_);
    boxLayout->addRow(QStringLiteral("Size X"), boxSizeX_);
    boxLayout->addRow(QStringLiteral("Size Y"), boxSizeY_);
    boxLayout->addRow(QStringLiteral("Size Z"), boxSizeZ_);
    boxLayout->addRow(QStringLiteral("Material"), boxMaterialType_);
    boxLayout->addRow(QStringLiteral("Albedo"), boxAlbedoButton_);
    boxLayout->addRow(QStringLiteral("Texture"), boxUseTexture_);
    boxLayout->addRow(QStringLiteral("Texture Path"), boxTextureButton_);
    boxLayout->addRow(QStringLiteral("Texture Scale"), boxTextureScale_);
    boxLayout->addRow(QStringLiteral("Texture Offset U"), boxTextureOffsetU_);
    boxLayout->addRow(QStringLiteral("Texture Offset V"), boxTextureOffsetV_);
    boxLayout->addRow(QStringLiteral("Texture Strength"), boxTextureStrength_);
    boxLayout->addRow(QStringLiteral("Roughness"), boxRoughness_);
    boxLayout->addRow(QStringLiteral("Refractive Index"), boxRefractiveIndex_);
    boxLayout->addRow(QStringLiteral("Emission Color"), boxEmissionColorButton_);
    boxLayout->addRow(QStringLiteral("Emission Strength"), boxEmissionStrength_);
    editorStack_->addWidget(boxPage);

    auto* cylinderPage = new QWidget(editorStack_);
    auto* cylinderLayout = new QFormLayout(cylinderPage);
    cylinderCenterX_ = createDoubleSpinBox(-100.0, 100.0, 0.1);
    cylinderCenterY_ = createDoubleSpinBox(-100.0, 100.0, 0.1);
    cylinderCenterZ_ = createDoubleSpinBox(-100.0, 100.0, 0.1);
    cylinderRadius_ = createDoubleSpinBox(0.01, 100.0, 0.05);
    cylinderHeight_ = createDoubleSpinBox(0.01, 100.0, 0.05);
    cylinderMaterialType_ = new QComboBox(cylinderPage);
    cylinderMaterialType_->addItems({QStringLiteral("Diffuse"), QStringLiteral("Metal"), QStringLiteral("Glass"), QStringLiteral("Emissive")});
    cylinderAlbedoButton_ = new QPushButton(cylinderPage);
    cylinderUseTexture_ = new QCheckBox(QStringLiteral("Enable Texture"), cylinderPage);
    cylinderTextureButton_ = new QPushButton(QStringLiteral("Load Texture"), cylinderPage);
    cylinderTextureScale_ = createDoubleSpinBox(0.001, 100.0, 0.25);
    cylinderTextureOffsetU_ = createDoubleSpinBox(-100.0, 100.0, 0.05);
    cylinderTextureOffsetV_ = createDoubleSpinBox(-100.0, 100.0, 0.05);
    cylinderTextureStrength_ = createDoubleSpinBox(0.0, 1.0, 0.05);
    cylinderRoughness_ = createDoubleSpinBox(0.0, 1.0, 0.05);
    cylinderRefractiveIndex_ = createDoubleSpinBox(1.0, 3.0, 0.05);
    cylinderEmissionColorButton_ = new QPushButton(cylinderPage);
    cylinderEmissionStrength_ = createDoubleSpinBox(0.0, 20.0, 0.25);
    cylinderLayout->addRow(QStringLiteral("Center X"), cylinderCenterX_);
    cylinderLayout->addRow(QStringLiteral("Center Y"), cylinderCenterY_);
    cylinderLayout->addRow(QStringLiteral("Center Z"), cylinderCenterZ_);
    cylinderLayout->addRow(QStringLiteral("Radius"), cylinderRadius_);
    cylinderLayout->addRow(QStringLiteral("Height"), cylinderHeight_);
    cylinderLayout->addRow(QStringLiteral("Material"), cylinderMaterialType_);
    cylinderLayout->addRow(QStringLiteral("Albedo"), cylinderAlbedoButton_);
    cylinderLayout->addRow(QStringLiteral("Texture"), cylinderUseTexture_);
    cylinderLayout->addRow(QStringLiteral("Texture Path"), cylinderTextureButton_);
    cylinderLayout->addRow(QStringLiteral("Texture Scale"), cylinderTextureScale_);
    cylinderLayout->addRow(QStringLiteral("Texture Offset U"), cylinderTextureOffsetU_);
    cylinderLayout->addRow(QStringLiteral("Texture Offset V"), cylinderTextureOffsetV_);
    cylinderLayout->addRow(QStringLiteral("Texture Strength"), cylinderTextureStrength_);
    cylinderLayout->addRow(QStringLiteral("Roughness"), cylinderRoughness_);
    cylinderLayout->addRow(QStringLiteral("Refractive Index"), cylinderRefractiveIndex_);
    cylinderLayout->addRow(QStringLiteral("Emission Color"), cylinderEmissionColorButton_);
    cylinderLayout->addRow(QStringLiteral("Emission Strength"), cylinderEmissionStrength_);
    editorStack_->addWidget(cylinderPage);

    auto* planePage = new QWidget(editorStack_);
    auto* planeLayout = new QFormLayout(planePage);
    planePointX_ = createDoubleSpinBox(-100.0, 100.0, 0.1);
    planePointY_ = createDoubleSpinBox(-100.0, 100.0, 0.1);
    planePointZ_ = createDoubleSpinBox(-100.0, 100.0, 0.1);
    planeNormalX_ = createDoubleSpinBox(-1.0, 1.0, 0.05);
    planeNormalY_ = createDoubleSpinBox(-1.0, 1.0, 0.05);
    planeNormalZ_ = createDoubleSpinBox(-1.0, 1.0, 0.05);
    planeMaterialType_ = new QComboBox(planePage);
    planeMaterialType_->addItems({QStringLiteral("Diffuse"), QStringLiteral("Metal"), QStringLiteral("Glass"), QStringLiteral("Emissive")});
    planeAlbedoButton_ = new QPushButton(planePage);
    planeUseTexture_ = new QCheckBox(QStringLiteral("Enable Texture"), planePage);
    planeTextureButton_ = new QPushButton(QStringLiteral("Load Texture"), planePage);
    planeTextureScale_ = createDoubleSpinBox(0.001, 100.0, 0.25);
    planeTextureOffsetU_ = createDoubleSpinBox(-100.0, 100.0, 0.05);
    planeTextureOffsetV_ = createDoubleSpinBox(-100.0, 100.0, 0.05);
    planeTextureStrength_ = createDoubleSpinBox(0.0, 1.0, 0.05);
    planeRoughness_ = createDoubleSpinBox(0.0, 1.0, 0.05);
    planeRefractiveIndex_ = createDoubleSpinBox(1.0, 3.0, 0.05);
    planeEmissionColorButton_ = new QPushButton(planePage);
    planeEmissionStrength_ = createDoubleSpinBox(0.0, 20.0, 0.25);
    planeLayout->addRow(QStringLiteral("Point X"), planePointX_);
    planeLayout->addRow(QStringLiteral("Point Y"), planePointY_);
    planeLayout->addRow(QStringLiteral("Point Z"), planePointZ_);
    planeLayout->addRow(QStringLiteral("Normal X"), planeNormalX_);
    planeLayout->addRow(QStringLiteral("Normal Y"), planeNormalY_);
    planeLayout->addRow(QStringLiteral("Normal Z"), planeNormalZ_);
    planeLayout->addRow(QStringLiteral("Material"), planeMaterialType_);
    planeLayout->addRow(QStringLiteral("Albedo"), planeAlbedoButton_);
    planeLayout->addRow(QStringLiteral("Texture"), planeUseTexture_);
    planeLayout->addRow(QStringLiteral("Texture Path"), planeTextureButton_);
    planeLayout->addRow(QStringLiteral("Texture Scale"), planeTextureScale_);
    planeLayout->addRow(QStringLiteral("Texture Offset U"), planeTextureOffsetU_);
    planeLayout->addRow(QStringLiteral("Texture Offset V"), planeTextureOffsetV_);
    planeLayout->addRow(QStringLiteral("Texture Strength"), planeTextureStrength_);
    planeLayout->addRow(QStringLiteral("Roughness"), planeRoughness_);
    planeLayout->addRow(QStringLiteral("Refractive Index"), planeRefractiveIndex_);
    planeLayout->addRow(QStringLiteral("Emission Color"), planeEmissionColorButton_);
    planeLayout->addRow(QStringLiteral("Emission Strength"), planeEmissionStrength_);
    editorStack_->addWidget(planePage);

    auto* lightPage = new QWidget(editorStack_);
    auto* lightLayout = new QFormLayout(lightPage);
    lightPositionX_ = createDoubleSpinBox(-100.0, 100.0, 0.1);
    lightPositionY_ = createDoubleSpinBox(-100.0, 100.0, 0.1);
    lightPositionZ_ = createDoubleSpinBox(-100.0, 100.0, 0.1);
    lightNormalX_ = createDoubleSpinBox(-1.0, 1.0, 0.05);
    lightNormalY_ = createDoubleSpinBox(-1.0, 1.0, 0.05);
    lightNormalZ_ = createDoubleSpinBox(-1.0, 1.0, 0.05);
    lightWidth_ = createDoubleSpinBox(0.01, 100.0, 0.05);
    lightHeight_ = createDoubleSpinBox(0.01, 100.0, 0.05);
    lightColorButton_ = new QPushButton(lightPage);
    lightIntensity_ = createDoubleSpinBox(0.0, 1000.0, 1.0);
    lightLayout->addRow(QStringLiteral("Position X"), lightPositionX_);
    lightLayout->addRow(QStringLiteral("Position Y"), lightPositionY_);
    lightLayout->addRow(QStringLiteral("Position Z"), lightPositionZ_);
    lightLayout->addRow(QStringLiteral("Normal X"), lightNormalX_);
    lightLayout->addRow(QStringLiteral("Normal Y"), lightNormalY_);
    lightLayout->addRow(QStringLiteral("Normal Z"), lightNormalZ_);
    lightLayout->addRow(QStringLiteral("Width"), lightWidth_);
    lightLayout->addRow(QStringLiteral("Height"), lightHeight_);
    lightLayout->addRow(QStringLiteral("Color"), lightColorButton_);
    lightLayout->addRow(QStringLiteral("Intensity"), lightIntensity_);
    editorStack_->addWidget(lightPage);

    editorLayout->addWidget(editorStack_);
    layout->addWidget(editorGroup, 1);

    connect(presetComboBox_, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &ScenePanel::handlePresetChanged);
    connect(environmentMode_, qOverload<int>(&QComboBox::currentIndexChanged), this, &ScenePanel::handleEnvironmentChanged);
    connect(environmentExposure_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ScenePanel::handleEnvironmentChanged);
    connect(environmentIntensity_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ScenePanel::handleEnvironmentChanged);
    connect(environmentRotationY_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ScenePanel::handleEnvironmentChanged);
    connect(loadEnvironmentImageButton_, &QPushButton::clicked, this, &ScenePanel::handleLoadEnvironmentImage);
    connect(resetEnvironmentButton_, &QPushButton::clicked, this, &ScenePanel::handleResetEnvironment);
    connect(overlayLabelsEnabled_, &QCheckBox::toggled, this, &ScenePanel::handleOverlayChanged);
    connect(overlayShowPosition_, &QCheckBox::toggled, this, &ScenePanel::handleOverlayChanged);
    connect(overlayShowMaterialInfo_, &QCheckBox::toggled, this, &ScenePanel::handleOverlayChanged);
    connect(bloomEnabled_, &QCheckBox::toggled, this, &ScenePanel::handleBloomChanged);
    connect(bloomExposure_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ScenePanel::handleBloomChanged);
    connect(bloomThreshold_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ScenePanel::handleBloomChanged);
    connect(bloomStrength_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ScenePanel::handleBloomChanged);
    connect(bloomBlurPassCount_, qOverload<int>(&QSpinBox::valueChanged), this, &ScenePanel::handleBloomChanged);
    connect(softShadowsEnabled_, &QCheckBox::toggled, this, &ScenePanel::handleSoftShadowChanged);
    connect(areaLightSamples_, qOverload<int>(&QSpinBox::valueChanged), this, &ScenePanel::handleSoftShadowChanged);
    connect(objectList_, &QListWidget::currentRowChanged, this, &ScenePanel::handleSelectionChanged);
    connect(addSphereButton_, &QPushButton::clicked, this, &ScenePanel::handleAddSphere);
    connect(addBoxButton_, &QPushButton::clicked, this, &ScenePanel::handleAddBox);
    connect(addCylinderButton_, &QPushButton::clicked, this, &ScenePanel::handleAddCylinder);
    connect(addPlaneButton_, &QPushButton::clicked, this, &ScenePanel::handleAddPlane);
    connect(addLightButton_, &QPushButton::clicked, this, &ScenePanel::handleAddLight);
    connect(addAreaLightButton_, &QPushButton::clicked, this, &ScenePanel::handleAddAreaLight);
    connect(deleteButton_, &QPushButton::clicked, this, &ScenePanel::handleDeleteSelected);

    for (QDoubleSpinBox* spinBox : {sphereCenterX_, sphereCenterY_, sphereCenterZ_, sphereRadius_, sphereRoughness_, sphereRefractiveIndex_, sphereEmissionStrength_, sphereTextureScale_, sphereTextureOffsetU_, sphereTextureOffsetV_, sphereTextureStrength_}) {
        connect(spinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ScenePanel::handleSphereChanged);
    }
    connect(sphereUseTexture_, &QCheckBox::toggled, this, &ScenePanel::handleSphereChanged);
    connect(sphereMaterialType_, qOverload<int>(&QComboBox::currentIndexChanged), this, &ScenePanel::handleSphereChanged);
    connect(sphereAlbedoButton_, &QPushButton::clicked, this, &ScenePanel::chooseSphereAlbedo);
    connect(sphereEmissionColorButton_, &QPushButton::clicked, this, &ScenePanel::chooseSphereEmissionColor);
    connect(sphereTextureButton_, &QPushButton::clicked, this, &ScenePanel::chooseSphereTexture);

    for (QDoubleSpinBox* spinBox : {boxCenterX_, boxCenterY_, boxCenterZ_, boxSizeX_, boxSizeY_, boxSizeZ_, boxRoughness_, boxRefractiveIndex_, boxEmissionStrength_, boxTextureScale_, boxTextureOffsetU_, boxTextureOffsetV_, boxTextureStrength_}) {
        connect(spinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ScenePanel::handleBoxChanged);
    }
    connect(boxUseTexture_, &QCheckBox::toggled, this, &ScenePanel::handleBoxChanged);
    connect(boxMaterialType_, qOverload<int>(&QComboBox::currentIndexChanged), this, &ScenePanel::handleBoxChanged);
    connect(boxAlbedoButton_, &QPushButton::clicked, this, &ScenePanel::chooseBoxAlbedo);
    connect(boxEmissionColorButton_, &QPushButton::clicked, this, &ScenePanel::chooseBoxEmissionColor);
    connect(boxTextureButton_, &QPushButton::clicked, this, &ScenePanel::chooseBoxTexture);

    for (QDoubleSpinBox* spinBox : {cylinderCenterX_, cylinderCenterY_, cylinderCenterZ_, cylinderRadius_, cylinderHeight_, cylinderRoughness_, cylinderRefractiveIndex_, cylinderEmissionStrength_, cylinderTextureScale_, cylinderTextureOffsetU_, cylinderTextureOffsetV_, cylinderTextureStrength_}) {
        connect(spinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ScenePanel::handleCylinderChanged);
    }
    connect(cylinderUseTexture_, &QCheckBox::toggled, this, &ScenePanel::handleCylinderChanged);
    connect(cylinderMaterialType_, qOverload<int>(&QComboBox::currentIndexChanged), this, &ScenePanel::handleCylinderChanged);
    connect(cylinderAlbedoButton_, &QPushButton::clicked, this, &ScenePanel::chooseCylinderAlbedo);
    connect(cylinderEmissionColorButton_, &QPushButton::clicked, this, &ScenePanel::chooseCylinderEmissionColor);
    connect(cylinderTextureButton_, &QPushButton::clicked, this, &ScenePanel::chooseCylinderTexture);

    for (QDoubleSpinBox* spinBox : {planePointX_, planePointY_, planePointZ_, planeNormalX_, planeNormalY_, planeNormalZ_, planeRoughness_, planeRefractiveIndex_, planeEmissionStrength_, planeTextureScale_, planeTextureOffsetU_, planeTextureOffsetV_, planeTextureStrength_}) {
        connect(spinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ScenePanel::handlePlaneChanged);
    }
    connect(planeUseTexture_, &QCheckBox::toggled, this, &ScenePanel::handlePlaneChanged);
    connect(planeMaterialType_, qOverload<int>(&QComboBox::currentIndexChanged), this, &ScenePanel::handlePlaneChanged);
    connect(planeAlbedoButton_, &QPushButton::clicked, this, &ScenePanel::choosePlaneAlbedo);
    connect(planeEmissionColorButton_, &QPushButton::clicked, this, &ScenePanel::choosePlaneEmissionColor);
    connect(planeTextureButton_, &QPushButton::clicked, this, &ScenePanel::choosePlaneTexture);

    for (QDoubleSpinBox* spinBox : {lightPositionX_, lightPositionY_, lightPositionZ_, lightNormalX_, lightNormalY_, lightNormalZ_, lightWidth_, lightHeight_, lightIntensity_}) {
        connect(spinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ScenePanel::handleLightChanged);
    }
    connect(lightColorButton_, &QPushButton::clicked, this, &ScenePanel::chooseLightColor);
}

void ScenePanel::handlePresetChanged(int index)
{
    if (updating_) {
        return;
    }

    const QString key = presetComboBox_->itemData(index).toString();
    if (key == "reflection") {
        setScene(tinyray::Scene::createReflectionDemo());
    } else if (key == "glass") {
        setScene(tinyray::Scene::createGlassDemo());
    } else {
        setScene(tinyray::Scene::createColoredLightsDemo());
    }
    emitSceneChanged();
}

void ScenePanel::handleSelectionChanged()
{
    if (!updating_) {
        loadSelectedEditor();
        emitSceneChanged();
    }
}

void ScenePanel::handleAddSphere()
{
    const tinyray::Material material = makeDiffuseMaterial(tinyray::Vec3(0.80, 0.35, 0.25));
    scene_.addSphere(tinyray::Sphere(tinyray::Vec3(0.0, 0.0, 0.0), 0.5, material));
    rebuildObjectList(1, static_cast<int>(scene_.objects.size()) - 1);
    emitSceneChanged();
}

void ScenePanel::handleAddPlane()
{
    tinyray::Material material;
    material.albedo = tinyray::Vec3(0.55, 0.55, 0.58);
    scene_.addPlane(tinyray::Plane(tinyray::Vec3(0.0, -0.5, 0.0), tinyray::Vec3(0.0, 1.0, 0.0), material));
    rebuildObjectList(1, static_cast<int>(scene_.objects.size()) - 1);
    emitSceneChanged();
}

void ScenePanel::handleAddBox()
{
    const tinyray::Material material = makeDiffuseMaterial(tinyray::Vec3(0.55, 0.30, 0.85));
    scene_.addBox(tinyray::Box(tinyray::Vec3(-0.8, -0.05, -0.8),
                               tinyray::Vec3(0.7, 0.9, 0.7),
                               material));
    rebuildObjectList(1, static_cast<int>(scene_.objects.size()) - 1);
    emitSceneChanged();
}

void ScenePanel::handleAddCylinder()
{
    const tinyray::Material material = makeDiffuseMaterial(tinyray::Vec3(0.22, 0.62, 0.88));
    scene_.addCylinder(tinyray::Cylinder(tinyray::Vec3(0.9, -0.05, -0.8),
                                         0.35,
                                         0.9,
                                         material));
    rebuildObjectList(1, static_cast<int>(scene_.objects.size()) - 1);
    emitSceneChanged();
}

void ScenePanel::handleAddLight()
{
    scene_.addLight(tinyray::Light(tinyray::Vec3(2.0, 3.0, 2.0), tinyray::Vec3(1.0, 0.95, 0.85), 16.0));
    rebuildObjectList(2, static_cast<int>(scene_.lights.size()) - 1);
    emitSceneChanged();
}

void ScenePanel::handleAddAreaLight()
{
    scene_.addLight(tinyray::Light::area(tinyray::Vec3(0.0, 3.0, -0.5),
                                         tinyray::Vec3(0.0, -1.0, 0.0),
                                         2.6,
                                         1.2,
                                         tinyray::Vec3(1.0, 0.84, 0.62),
                                         18.0));
    rebuildObjectList(2, static_cast<int>(scene_.lights.size()) - 1);
    emitSceneChanged();
}

void ScenePanel::handleDeleteSelected()
{
    const QListWidgetItem* item = objectList_->currentItem();
    if (!item) {
        return;
    }

    const QString kind = item->data(Qt::UserRole).toString();
    const int index = item->data(Qt::UserRole + 1).toInt();
    if (kind == "object" && index >= 0 && index < static_cast<int>(scene_.objects.size())) {
        scene_.objects.erase(scene_.objects.begin() + index);
    } else if (kind == "light" && index >= 0 && index < static_cast<int>(scene_.lights.size())) {
        scene_.lights.erase(scene_.lights.begin() + index);
    }

    rebuildObjectList();
    emitSceneChanged();
}

void ScenePanel::handleSphereChanged()
{
    if (updating_) {
        return;
    }

    tinyray::Sphere* sphere = selectedSphere();
    if (!sphere) {
        return;
    }

    sphere->center = tinyray::Vec3(sphereCenterX_->value(), sphereCenterY_->value(), sphereCenterZ_->value());
    sphere->radius = sphereRadius_->value();
    sphere->material = readSphereMaterial();
    rebuildObjectList(1, objectList_->currentItem()->data(Qt::UserRole + 1).toInt());
    emitSceneChanged();
}

void ScenePanel::handleBoxChanged()
{
    if (updating_) {
        return;
    }

    tinyray::Box* box = selectedBox();
    if (!box) {
        return;
    }

    box->center = tinyray::Vec3(boxCenterX_->value(), boxCenterY_->value(), boxCenterZ_->value());
    box->size = tinyray::Vec3(boxSizeX_->value(), boxSizeY_->value(), boxSizeZ_->value());
    box->material = readBoxMaterial();
    rebuildObjectList(1, objectList_->currentItem()->data(Qt::UserRole + 1).toInt());
    emitSceneChanged();
}

void ScenePanel::handleCylinderChanged()
{
    if (updating_) {
        return;
    }

    tinyray::Cylinder* cylinder = selectedCylinder();
    if (!cylinder) {
        return;
    }

    cylinder->center = tinyray::Vec3(cylinderCenterX_->value(), cylinderCenterY_->value(), cylinderCenterZ_->value());
    cylinder->radius = cylinderRadius_->value();
    cylinder->height = cylinderHeight_->value();
    cylinder->material = readCylinderMaterial();
    rebuildObjectList(1, objectList_->currentItem()->data(Qt::UserRole + 1).toInt());
    emitSceneChanged();
}

void ScenePanel::handlePlaneChanged()
{
    if (updating_) {
        return;
    }

    tinyray::Plane* plane = selectedPlane();
    if (!plane) {
        return;
    }

    tinyray::Vec3 normal(planeNormalX_->value(), planeNormalY_->value(), planeNormalZ_->value());
    if (normal.nearZero()) {
        normal = tinyray::Vec3(0.0, 1.0, 0.0);
    }

    plane->point = tinyray::Vec3(planePointX_->value(), planePointY_->value(), planePointZ_->value());
    plane->normal = normal.normalized();
    plane->material = readPlaneMaterial();
    rebuildObjectList(1, objectList_->currentItem()->data(Qt::UserRole + 1).toInt());
    emitSceneChanged();
}

void ScenePanel::handleLightChanged()
{
    if (updating_) {
        return;
    }

    tinyray::Light* light = selectedLight();
    if (!light) {
        return;
    }

    light->position = tinyray::Vec3(lightPositionX_->value(), lightPositionY_->value(), lightPositionZ_->value());
    tinyray::Vec3 normal(lightNormalX_->value(), lightNormalY_->value(), lightNormalZ_->value());
    if (normal.nearZero()) {
        normal = tinyray::Vec3(0.0, -1.0, 0.0);
    }
    light->normal = normal.normalized();
    light->width = lightWidth_->value();
    light->height = lightHeight_->value();
    light->color = colorToVec3(lightColor_);
    light->intensity = lightIntensity_->value();
    rebuildObjectList(2, objectList_->currentItem()->data(Qt::UserRole + 1).toInt());
    emitSceneChanged();
}

void ScenePanel::handleBloomChanged()
{
    if (updating_) {
        return;
    }

    scene_.bloomSettings.enabled = bloomEnabled_->isChecked();
    scene_.bloomSettings.exposure = bloomExposure_->value();
    scene_.bloomSettings.threshold = bloomThreshold_->value();
    scene_.bloomSettings.strength = bloomStrength_->value();
    scene_.bloomSettings.blurPassCount = bloomBlurPassCount_->value();
    emitSceneChanged();
}

void ScenePanel::handleSoftShadowChanged()
{
    if (updating_) {
        return;
    }

    scene_.softShadowsEnabled = softShadowsEnabled_->isChecked();
    scene_.areaLightSamples = areaLightSamples_->value();
    emitSceneChanged();
}

void ScenePanel::handleEnvironmentChanged()
{
    if (updating_) {
        return;
    }

    const QVariant modeData = environmentMode_->currentData();
    const int modeValue = modeData.isValid()
        ? modeData.toInt()
        : static_cast<int>(tinyray::EnvironmentMode::Gradient);
    scene_.environment.mode = static_cast<tinyray::EnvironmentMode>(modeValue);
    scene_.environment.exposure = environmentExposure_->value();
    scene_.environment.intensity = environmentIntensity_->value();
    scene_.environment.rotationY = environmentRotationY_->value();
    emitSceneChanged();
}

void ScenePanel::handleLoadEnvironmentImage()
{
    const QString fileName = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Load Environment Image"),
        QString(),
        QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp *.tga);;All Files (*)"));
    if (fileName.isEmpty()) {
        return;
    }

    tinyray::Environment environment = scene_.environment;
    if (!environment.loadImage(fileName)) {
        QMessageBox::warning(this,
                             QStringLiteral("Environment Image"),
                             QStringLiteral("Failed to load the selected environment image."));
        return;
    }

    scene_.environment = environment;
    syncEnvironmentControls();
    emitSceneChanged();
}

void ScenePanel::handleResetEnvironment()
{
    if (updating_) {
        return;
    }

    scene_.environment = tinyray::Environment();
    syncEnvironmentControls();
    emitSceneChanged();
}

void ScenePanel::handleOverlayChanged()
{
    if (updating_) {
        return;
    }

    scene_.overlayLabelsEnabled = overlayLabelsEnabled_->isChecked();
    scene_.overlayShowPosition = overlayShowPosition_->isChecked();
    scene_.overlayShowMaterialInfo = overlayShowMaterialInfo_->isChecked();
    emitSceneChanged();
}

void ScenePanel::chooseSphereAlbedo()
{
    const QColor color = QColorDialog::getColor(sphereAlbedo_, this, QStringLiteral("Choose Albedo"));
    if (color.isValid()) {
        sphereAlbedo_ = color;
        updateColorButton(sphereAlbedoButton_, sphereAlbedo_);
        handleSphereChanged();
    }
}

void ScenePanel::chooseBoxAlbedo()
{
    const QColor color = QColorDialog::getColor(boxAlbedo_, this, QStringLiteral("Choose Albedo"));
    if (color.isValid()) {
        boxAlbedo_ = color;
        updateColorButton(boxAlbedoButton_, boxAlbedo_);
        handleBoxChanged();
    }
}

void ScenePanel::chooseCylinderAlbedo()
{
    const QColor color = QColorDialog::getColor(cylinderAlbedo_, this, QStringLiteral("Choose Albedo"));
    if (color.isValid()) {
        cylinderAlbedo_ = color;
        updateColorButton(cylinderAlbedoButton_, cylinderAlbedo_);
        handleCylinderChanged();
    }
}

void ScenePanel::choosePlaneAlbedo()
{
    const QColor color = QColorDialog::getColor(planeAlbedo_, this, QStringLiteral("Choose Albedo"));
    if (color.isValid()) {
        planeAlbedo_ = color;
        updateColorButton(planeAlbedoButton_, planeAlbedo_);
        handlePlaneChanged();
    }
}

namespace {

bool chooseTexturePath(QWidget* parent, QPushButton* button)
{
    const QString fileName = QFileDialog::getOpenFileName(
        parent,
        QStringLiteral("Load Texture"),
        QString(),
        QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp *.tga);;All Files (*)"));
    if (fileName.isEmpty()) {
        return false;
    }

    if (QImage(fileName).isNull()) {
        QMessageBox::warning(parent,
                             QStringLiteral("Texture"),
                             QStringLiteral("Failed to load the selected texture image."));
        return false;
    }

    button->setProperty("texturePath", fileName);
    button->setText(QStringLiteral("Change Texture"));
    button->setToolTip(fileName);
    return true;
}

} // namespace

void ScenePanel::chooseSphereTexture()
{
    if (chooseTexturePath(this, sphereTextureButton_)) {
        sphereUseTexture_->setChecked(true);
        handleSphereChanged();
    }
}

void ScenePanel::chooseBoxTexture()
{
    if (chooseTexturePath(this, boxTextureButton_)) {
        boxUseTexture_->setChecked(true);
        handleBoxChanged();
    }
}

void ScenePanel::chooseCylinderTexture()
{
    if (chooseTexturePath(this, cylinderTextureButton_)) {
        cylinderUseTexture_->setChecked(true);
        handleCylinderChanged();
    }
}

void ScenePanel::choosePlaneTexture()
{
    if (chooseTexturePath(this, planeTextureButton_)) {
        planeUseTexture_->setChecked(true);
        handlePlaneChanged();
    }
}

void ScenePanel::chooseLightColor()
{
    const QColor color = QColorDialog::getColor(lightColor_, this, QStringLiteral("Choose Light Color"));
    if (color.isValid()) {
        lightColor_ = color;
        updateColorButton(lightColorButton_, lightColor_);
        handleLightChanged();
    }
}

void ScenePanel::rebuildObjectList(int preferredKind, int preferredIndex)
{
    updating_ = true;
    objectList_->clear();

    for (int index = 0; index < static_cast<int>(scene_.objects.size()); ++index) {
        const auto& object = scene_.objects[static_cast<std::size_t>(index)];
        QString label;
        if (const auto* sphere = dynamic_cast<const tinyray::Sphere*>(object.get())) {
            label = QStringLiteral("Sphere %1 (%2)").arg(index + 1).arg(materialName(sphere->material.type));
        } else if (const auto* box = dynamic_cast<const tinyray::Box*>(object.get())) {
            label = QStringLiteral("Box %1 (%2)").arg(index + 1).arg(materialName(box->material.type));
        } else if (const auto* cylinder = dynamic_cast<const tinyray::Cylinder*>(object.get())) {
            label = QStringLiteral("Cylinder %1 (%2)").arg(index + 1).arg(materialName(cylinder->material.type));
        } else if (const auto* plane = dynamic_cast<const tinyray::Plane*>(object.get())) {
            label = QStringLiteral("Plane %1 (%2)").arg(index + 1).arg(materialName(plane->material.type));
        } else {
            label = QStringLiteral("Object %1").arg(index + 1);
        }

        auto* item = new QListWidgetItem(label, objectList_);
        item->setData(Qt::UserRole, QStringLiteral("object"));
        item->setData(Qt::UserRole + 1, index);
        if (preferredKind == 1 && preferredIndex == index) {
            objectList_->setCurrentItem(item);
        }
    }

    for (int index = 0; index < static_cast<int>(scene_.lights.size()); ++index) {
        const tinyray::Light& light = scene_.lights[static_cast<std::size_t>(index)];
        const QString lightName = light.type == tinyray::LightType::Area
            ? QStringLiteral("Area Light %1")
            : QStringLiteral("Point Light %1");
        auto* item = new QListWidgetItem(lightName.arg(index + 1), objectList_);
        item->setData(Qt::UserRole, QStringLiteral("light"));
        item->setData(Qt::UserRole + 1, index);
        if (preferredKind == 2 && preferredIndex == index) {
            objectList_->setCurrentItem(item);
        }
    }

    if (!objectList_->currentItem() && objectList_->count() > 0) {
        objectList_->setCurrentRow(0);
    }

    updating_ = false;
    loadSelectedEditor();
}

void ScenePanel::loadSelectedEditor()
{
    const QListWidgetItem* item = objectList_->currentItem();
    if (!item) {
        scene_.selectedObjectId = -1;
        editorStack_->setCurrentIndex(0);
        return;
    }

    const QString kind = item->data(Qt::UserRole).toString();
    const int index = item->data(Qt::UserRole + 1).toInt();
    updating_ = true;
    if (kind == "object" && index >= 0 && index < static_cast<int>(scene_.objects.size())) {
        const auto& object = scene_.objects[static_cast<std::size_t>(index)];
        scene_.selectedObjectId = object ? object->id() : -1;
        if (const auto* sphere = dynamic_cast<const tinyray::Sphere*>(object.get())) {
            setSphereEditor(*sphere);
            editorStack_->setCurrentIndex(1);
        } else if (const auto* box = dynamic_cast<const tinyray::Box*>(object.get())) {
            setBoxEditor(*box);
            editorStack_->setCurrentIndex(2);
        } else if (const auto* cylinder = dynamic_cast<const tinyray::Cylinder*>(object.get())) {
            setCylinderEditor(*cylinder);
            editorStack_->setCurrentIndex(3);
        } else if (const auto* plane = dynamic_cast<const tinyray::Plane*>(object.get())) {
            setPlaneEditor(*plane);
            editorStack_->setCurrentIndex(4);
        } else {
            editorStack_->setCurrentIndex(0);
        }
    } else if (kind == "light" && index >= 0 && index < static_cast<int>(scene_.lights.size())) {
        scene_.selectedObjectId = -1;
        setLightEditor(scene_.lights[static_cast<std::size_t>(index)]);
        editorStack_->setCurrentIndex(5);
    } else {
        scene_.selectedObjectId = -1;
        editorStack_->setCurrentIndex(0);
    }
    updating_ = false;
}

void ScenePanel::emitSceneChanged()
{
    if (!updating_) {
        emit sceneChanged(scene_);
    }
}

void ScenePanel::syncBloomControls()
{
    if (!bloomEnabled_ || !bloomExposure_ || !bloomThreshold_ || !bloomStrength_ || !bloomBlurPassCount_) {
        return;
    }

    bloomEnabled_->setChecked(scene_.bloomSettings.enabled);
    bloomExposure_->setValue(scene_.bloomSettings.safeExposure());
    bloomThreshold_->setValue(scene_.bloomSettings.safeThreshold());
    bloomStrength_->setValue(scene_.bloomSettings.safeStrength());
    bloomBlurPassCount_->setValue(scene_.bloomSettings.safeBlurPassCount());
    if (softShadowsEnabled_ && areaLightSamples_) {
        softShadowsEnabled_->setChecked(scene_.softShadowsEnabled);
        areaLightSamples_->setValue(std::clamp(scene_.areaLightSamples, 1, 128));
    }
}

void ScenePanel::syncEnvironmentControls()
{
    if (!environmentMode_ || !environmentExposure_ || !environmentIntensity_ || !environmentRotationY_) {
        return;
    }

    const int modeValue = static_cast<int>(scene_.environment.mode);
    const int modeIndex = environmentMode_->findData(modeValue);
    environmentMode_->setCurrentIndex(modeIndex >= 0 ? modeIndex : 0);
    environmentExposure_->setValue(scene_.environment.exposure);
    environmentIntensity_->setValue(scene_.environment.intensity);
    environmentRotationY_->setValue(scene_.environment.rotationY);

    if (loadEnvironmentImageButton_) {
        loadEnvironmentImageButton_->setText(scene_.environment.imagePath.isEmpty()
            ? QStringLiteral("Load Image")
            : QStringLiteral("Change Image"));
        loadEnvironmentImageButton_->setToolTip(scene_.environment.imagePath);
    }
    if (overlayLabelsEnabled_ && overlayShowPosition_ && overlayShowMaterialInfo_) {
        overlayLabelsEnabled_->setChecked(scene_.overlayLabelsEnabled);
        overlayShowPosition_->setChecked(scene_.overlayShowPosition);
        overlayShowMaterialInfo_->setChecked(scene_.overlayShowMaterialInfo);
    }
}

void ScenePanel::setSphereEditor(const tinyray::Sphere& sphere)
{
    sphereCenterX_->setValue(sphere.center.x);
    sphereCenterY_->setValue(sphere.center.y);
    sphereCenterZ_->setValue(sphere.center.z);
    sphereRadius_->setValue(sphere.radius);
    writeMaterialToControls(sphere.material, sphereMaterialType_, sphereRoughness_,
                            sphereRefractiveIndex_, sphereAlbedo_, sphereAlbedoButton_);
    sphereUseTexture_->setChecked(sphere.material.useTexture);
    sphereTextureButton_->setProperty("texturePath", sphere.material.texturePath);
    sphereTextureButton_->setText(sphere.material.texturePath.isEmpty() ? QStringLiteral("Checkerboard") : QStringLiteral("Change Texture"));
    sphereTextureButton_->setToolTip(sphere.material.texturePath);
    sphereTextureScale_->setValue(sphere.material.textureScale);
    sphereTextureOffsetU_->setValue(sphere.material.textureOffsetU);
    sphereTextureOffsetV_->setValue(sphere.material.textureOffsetV);
    sphereTextureStrength_->setValue(sphere.material.textureStrength);
}

void ScenePanel::setBoxEditor(const tinyray::Box& box)
{
    boxCenterX_->setValue(box.center.x);
    boxCenterY_->setValue(box.center.y);
    boxCenterZ_->setValue(box.center.z);
    boxSizeX_->setValue(box.size.x);
    boxSizeY_->setValue(box.size.y);
    boxSizeZ_->setValue(box.size.z);
    writeMaterialToControls(box.material, boxMaterialType_, boxRoughness_,
                            boxRefractiveIndex_, boxAlbedo_, boxAlbedoButton_);
    boxUseTexture_->setChecked(box.material.useTexture);
    boxTextureButton_->setProperty("texturePath", box.material.texturePath);
    boxTextureButton_->setText(box.material.texturePath.isEmpty() ? QStringLiteral("Checkerboard") : QStringLiteral("Change Texture"));
    boxTextureButton_->setToolTip(box.material.texturePath);
    boxTextureScale_->setValue(box.material.textureScale);
    boxTextureOffsetU_->setValue(box.material.textureOffsetU);
    boxTextureOffsetV_->setValue(box.material.textureOffsetV);
    boxTextureStrength_->setValue(box.material.textureStrength);
}

void ScenePanel::setCylinderEditor(const tinyray::Cylinder& cylinder)
{
    cylinderCenterX_->setValue(cylinder.center.x);
    cylinderCenterY_->setValue(cylinder.center.y);
    cylinderCenterZ_->setValue(cylinder.center.z);
    cylinderRadius_->setValue(cylinder.radius);
    cylinderHeight_->setValue(cylinder.height);
    writeMaterialToControls(cylinder.material, cylinderMaterialType_, cylinderRoughness_,
                            cylinderRefractiveIndex_, cylinderAlbedo_, cylinderAlbedoButton_);
    cylinderUseTexture_->setChecked(cylinder.material.useTexture);
    cylinderTextureButton_->setProperty("texturePath", cylinder.material.texturePath);
    cylinderTextureButton_->setText(cylinder.material.texturePath.isEmpty() ? QStringLiteral("Checkerboard") : QStringLiteral("Change Texture"));
    cylinderTextureButton_->setToolTip(cylinder.material.texturePath);
    cylinderTextureScale_->setValue(cylinder.material.textureScale);
    cylinderTextureOffsetU_->setValue(cylinder.material.textureOffsetU);
    cylinderTextureOffsetV_->setValue(cylinder.material.textureOffsetV);
    cylinderTextureStrength_->setValue(cylinder.material.textureStrength);
}

void ScenePanel::setPlaneEditor(const tinyray::Plane& plane)
{
    planePointX_->setValue(plane.point.x);
    planePointY_->setValue(plane.point.y);
    planePointZ_->setValue(plane.point.z);
    planeNormalX_->setValue(plane.normal.x);
    planeNormalY_->setValue(plane.normal.y);
    planeNormalZ_->setValue(plane.normal.z);
    writeMaterialToControls(plane.material, planeMaterialType_, planeRoughness_,
                            planeRefractiveIndex_, planeAlbedo_, planeAlbedoButton_);
    planeUseTexture_->setChecked(plane.material.useTexture);
    planeTextureButton_->setProperty("texturePath", plane.material.texturePath);
    planeTextureButton_->setText(plane.material.texturePath.isEmpty() ? QStringLiteral("Checkerboard") : QStringLiteral("Change Texture"));
    planeTextureButton_->setToolTip(plane.material.texturePath);
    planeTextureScale_->setValue(plane.material.textureScale);
    planeTextureOffsetU_->setValue(plane.material.textureOffsetU);
    planeTextureOffsetV_->setValue(plane.material.textureOffsetV);
    planeTextureStrength_->setValue(plane.material.textureStrength);
}

void ScenePanel::setLightEditor(const tinyray::Light& light)
{
    lightPositionX_->setValue(light.position.x);
    lightPositionY_->setValue(light.position.y);
    lightPositionZ_->setValue(light.position.z);
    lightNormalX_->setValue(light.safeNormal().x);
    lightNormalY_->setValue(light.safeNormal().y);
    lightNormalZ_->setValue(light.safeNormal().z);
    lightWidth_->setValue(std::max(light.width, 0.01));
    lightHeight_->setValue(std::max(light.height, 0.01));
    lightIntensity_->setValue(light.intensity);
    lightColor_ = vec3ToColor(light.color);
    updateColorButton(lightColorButton_, lightColor_);
}

tinyray::Sphere* ScenePanel::selectedSphere()
{
    const QListWidgetItem* item = objectList_->currentItem();
    if (!item || item->data(Qt::UserRole).toString() != "object") {
        return nullptr;
    }

    const int index = item->data(Qt::UserRole + 1).toInt();
    if (index < 0 || index >= static_cast<int>(scene_.objects.size())) {
        return nullptr;
    }

    return dynamic_cast<tinyray::Sphere*>(scene_.objects[static_cast<std::size_t>(index)].get());
}

tinyray::Box* ScenePanel::selectedBox()
{
    const QListWidgetItem* item = objectList_->currentItem();
    if (!item || item->data(Qt::UserRole).toString() != "object") {
        return nullptr;
    }

    const int index = item->data(Qt::UserRole + 1).toInt();
    if (index < 0 || index >= static_cast<int>(scene_.objects.size())) {
        return nullptr;
    }

    return dynamic_cast<tinyray::Box*>(scene_.objects[static_cast<std::size_t>(index)].get());
}

tinyray::Cylinder* ScenePanel::selectedCylinder()
{
    const QListWidgetItem* item = objectList_->currentItem();
    if (!item || item->data(Qt::UserRole).toString() != "object") {
        return nullptr;
    }

    const int index = item->data(Qt::UserRole + 1).toInt();
    if (index < 0 || index >= static_cast<int>(scene_.objects.size())) {
        return nullptr;
    }

    return dynamic_cast<tinyray::Cylinder*>(scene_.objects[static_cast<std::size_t>(index)].get());
}

tinyray::Plane* ScenePanel::selectedPlane()
{
    const QListWidgetItem* item = objectList_->currentItem();
    if (!item || item->data(Qt::UserRole).toString() != "object") {
        return nullptr;
    }

    const int index = item->data(Qt::UserRole + 1).toInt();
    if (index < 0 || index >= static_cast<int>(scene_.objects.size())) {
        return nullptr;
    }

    return dynamic_cast<tinyray::Plane*>(scene_.objects[static_cast<std::size_t>(index)].get());
}

tinyray::Light* ScenePanel::selectedLight()
{
    const QListWidgetItem* item = objectList_->currentItem();
    if (!item || item->data(Qt::UserRole).toString() != "light") {
        return nullptr;
    }

    const int index = item->data(Qt::UserRole + 1).toInt();
    if (index < 0 || index >= static_cast<int>(scene_.lights.size())) {
        return nullptr;
    }

    return &scene_.lights[static_cast<std::size_t>(index)];
}

QDoubleSpinBox* ScenePanel::createDoubleSpinBox(double minimum, double maximum, double step)
{
    auto* spinBox = new QDoubleSpinBox(this);
    spinBox->setRange(minimum, maximum);
    spinBox->setDecimals(3);
    spinBox->setSingleStep(step);
    return spinBox;
}

void ScenePanel::updateColorButton(QPushButton* button, const QColor& color)
{
    button->setText(QStringLiteral("RGB %1 %2 %3").arg(color.red()).arg(color.green()).arg(color.blue()));
    button->setStyleSheet(QStringLiteral("background-color: rgb(%1, %2, %3); color: %4;")
                              .arg(color.red())
                              .arg(color.green())
                              .arg(color.blue())
                              .arg(color.lightness() < 128 ? "white" : "black"));
}

tinyray::Material ScenePanel::readSphereMaterial() const
{
    tinyray::Material material;
    material.type = materialTypeFromIndex(sphereMaterialType_->currentIndex());
    material.albedo = colorToVec3(sphereAlbedo_);
    material.roughness = sphereRoughness_->value();
    material.refractiveIndex = sphereRefractiveIndex_->value();
    material.emissionColor = colorToVec3(sphereEmissionColor_);
    material.emissionStrength = sphereEmissionStrength_->value();
    material.useTexture = sphereUseTexture_->isChecked();
    material.textureScale = sphereTextureScale_->value();
    material.textureOffsetU = sphereTextureOffsetU_->value();
    material.textureOffsetV = sphereTextureOffsetV_->value();
    material.textureStrength = sphereTextureStrength_->value();
    material.texturePath = sphereTextureButton_->property("texturePath").toString();
    material.textureType = material.texturePath.isEmpty() ? tinyray::TextureType::Checkerboard : tinyray::TextureType::Image;
    material.fallbackColor = material.albedo;
    return material;
}

tinyray::Material ScenePanel::readBoxMaterial() const
{
    tinyray::Material material;
    material.type = materialTypeFromIndex(boxMaterialType_->currentIndex());
    material.albedo = colorToVec3(boxAlbedo_);
    material.roughness = boxRoughness_->value();
    material.refractiveIndex = boxRefractiveIndex_->value();
    material.emissionColor = colorToVec3(boxEmissionColor_);
    material.emissionStrength = boxEmissionStrength_->value();
    material.useTexture = boxUseTexture_->isChecked();
    material.textureScale = boxTextureScale_->value();
    material.textureOffsetU = boxTextureOffsetU_->value();
    material.textureOffsetV = boxTextureOffsetV_->value();
    material.textureStrength = boxTextureStrength_->value();
    material.texturePath = boxTextureButton_->property("texturePath").toString();
    material.textureType = material.texturePath.isEmpty() ? tinyray::TextureType::Checkerboard : tinyray::TextureType::Image;
    material.fallbackColor = material.albedo;
    return material;
}

tinyray::Material ScenePanel::readCylinderMaterial() const
{
    tinyray::Material material;
    material.type = materialTypeFromIndex(cylinderMaterialType_->currentIndex());
    material.albedo = colorToVec3(cylinderAlbedo_);
    material.roughness = cylinderRoughness_->value();
    material.refractiveIndex = cylinderRefractiveIndex_->value();
    material.emissionColor = colorToVec3(cylinderEmissionColor_);
    material.emissionStrength = cylinderEmissionStrength_->value();
    material.useTexture = cylinderUseTexture_->isChecked();
    material.textureScale = cylinderTextureScale_->value();
    material.textureOffsetU = cylinderTextureOffsetU_->value();
    material.textureOffsetV = cylinderTextureOffsetV_->value();
    material.textureStrength = cylinderTextureStrength_->value();
    material.texturePath = cylinderTextureButton_->property("texturePath").toString();
    material.textureType = material.texturePath.isEmpty() ? tinyray::TextureType::Checkerboard : tinyray::TextureType::Image;
    material.fallbackColor = material.albedo;
    return material;
}

tinyray::Material ScenePanel::readPlaneMaterial() const
{
    tinyray::Material material;
    material.type = materialTypeFromIndex(planeMaterialType_->currentIndex());
    material.albedo = colorToVec3(planeAlbedo_);
    material.roughness = planeRoughness_->value();
    material.refractiveIndex = planeRefractiveIndex_->value();
    material.emissionColor = colorToVec3(planeEmissionColor_);
    material.emissionStrength = planeEmissionStrength_->value();
    material.useTexture = planeUseTexture_->isChecked();
    material.textureScale = planeTextureScale_->value();
    material.textureOffsetU = planeTextureOffsetU_->value();
    material.textureOffsetV = planeTextureOffsetV_->value();
    material.textureStrength = planeTextureStrength_->value();
    material.texturePath = planeTextureButton_->property("texturePath").toString();
    material.textureType = material.texturePath.isEmpty() ? tinyray::TextureType::Checkerboard : tinyray::TextureType::Image;
    material.fallbackColor = material.albedo;
    return material;
}

void ScenePanel::writeMaterialToControls(const tinyray::Material& material,
                                         QComboBox* typeCombo,
                                         QDoubleSpinBox* roughnessSpin,
                                         QDoubleSpinBox* refractiveSpin,
                                         QColor& emissionColorCache,
                                         QPushButton* emissionColorButton,
                                         QDoubleSpinBox* emissionStrengthSpin,
                                         QColor& colorCache,
                                         QPushButton* colorButton)
{
    typeCombo->setCurrentIndex(materialTypeToIndex(material.type));
    roughnessSpin->setValue(material.roughness);
    refractiveSpin->setValue(material.refractiveIndex);
    emissionColorCache = vec3ToColor(material.emissionColor);
    updateColorButton(emissionColorButton, emissionColorCache);
    emissionStrengthSpin->setValue(material.emissionStrength);
    colorCache = vec3ToColor(material.albedo);
    updateColorButton(colorButton, colorCache);
}
