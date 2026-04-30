#include "gui/ScenePanel.h"

#include <algorithm>
#include <cstddef>

#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

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
    return tinyray::MaterialType::Diffuse;
}

QString materialName(tinyray::MaterialType type)
{
    switch (type) {
    case tinyray::MaterialType::Metal:
        return QStringLiteral("Metal");
    case tinyray::MaterialType::Glass:
        return QStringLiteral("Glass");
    case tinyray::MaterialType::Diffuse:
    default:
        return QStringLiteral("Diffuse");
    }
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

    auto* sceneGroup = new QGroupBox(QStringLiteral("Scene"), this);
    auto* sceneLayout = new QVBoxLayout(sceneGroup);
    objectList_ = new QListWidget(sceneGroup);
    objectList_->setMinimumHeight(130);
    sceneLayout->addWidget(objectList_);

    auto* buttonRow = new QHBoxLayout();
    addSphereButton_ = new QPushButton(QStringLiteral("Sphere"), sceneGroup);
    addPlaneButton_ = new QPushButton(QStringLiteral("Plane"), sceneGroup);
    addLightButton_ = new QPushButton(QStringLiteral("Light"), sceneGroup);
    deleteButton_ = new QPushButton(QStringLiteral("Delete"), sceneGroup);
    buttonRow->addWidget(addSphereButton_);
    buttonRow->addWidget(addPlaneButton_);
    buttonRow->addWidget(addLightButton_);
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
    sphereMaterialType_->addItems({QStringLiteral("Diffuse"), QStringLiteral("Metal"), QStringLiteral("Glass")});
    sphereAlbedoButton_ = new QPushButton(spherePage);
    sphereRoughness_ = createDoubleSpinBox(0.0, 1.0, 0.05);
    sphereRefractiveIndex_ = createDoubleSpinBox(1.0, 3.0, 0.05);
    sphereLayout->addRow(QStringLiteral("Center X"), sphereCenterX_);
    sphereLayout->addRow(QStringLiteral("Center Y"), sphereCenterY_);
    sphereLayout->addRow(QStringLiteral("Center Z"), sphereCenterZ_);
    sphereLayout->addRow(QStringLiteral("Radius"), sphereRadius_);
    sphereLayout->addRow(QStringLiteral("Material"), sphereMaterialType_);
    sphereLayout->addRow(QStringLiteral("Albedo"), sphereAlbedoButton_);
    sphereLayout->addRow(QStringLiteral("Roughness"), sphereRoughness_);
    sphereLayout->addRow(QStringLiteral("Refractive Index"), sphereRefractiveIndex_);
    editorStack_->addWidget(spherePage);

    auto* planePage = new QWidget(editorStack_);
    auto* planeLayout = new QFormLayout(planePage);
    planePointX_ = createDoubleSpinBox(-100.0, 100.0, 0.1);
    planePointY_ = createDoubleSpinBox(-100.0, 100.0, 0.1);
    planePointZ_ = createDoubleSpinBox(-100.0, 100.0, 0.1);
    planeNormalX_ = createDoubleSpinBox(-1.0, 1.0, 0.05);
    planeNormalY_ = createDoubleSpinBox(-1.0, 1.0, 0.05);
    planeNormalZ_ = createDoubleSpinBox(-1.0, 1.0, 0.05);
    planeMaterialType_ = new QComboBox(planePage);
    planeMaterialType_->addItems({QStringLiteral("Diffuse"), QStringLiteral("Metal"), QStringLiteral("Glass")});
    planeAlbedoButton_ = new QPushButton(planePage);
    planeRoughness_ = createDoubleSpinBox(0.0, 1.0, 0.05);
    planeRefractiveIndex_ = createDoubleSpinBox(1.0, 3.0, 0.05);
    planeLayout->addRow(QStringLiteral("Point X"), planePointX_);
    planeLayout->addRow(QStringLiteral("Point Y"), planePointY_);
    planeLayout->addRow(QStringLiteral("Point Z"), planePointZ_);
    planeLayout->addRow(QStringLiteral("Normal X"), planeNormalX_);
    planeLayout->addRow(QStringLiteral("Normal Y"), planeNormalY_);
    planeLayout->addRow(QStringLiteral("Normal Z"), planeNormalZ_);
    planeLayout->addRow(QStringLiteral("Material"), planeMaterialType_);
    planeLayout->addRow(QStringLiteral("Albedo"), planeAlbedoButton_);
    planeLayout->addRow(QStringLiteral("Roughness"), planeRoughness_);
    planeLayout->addRow(QStringLiteral("Refractive Index"), planeRefractiveIndex_);
    editorStack_->addWidget(planePage);

    auto* lightPage = new QWidget(editorStack_);
    auto* lightLayout = new QFormLayout(lightPage);
    lightPositionX_ = createDoubleSpinBox(-100.0, 100.0, 0.1);
    lightPositionY_ = createDoubleSpinBox(-100.0, 100.0, 0.1);
    lightPositionZ_ = createDoubleSpinBox(-100.0, 100.0, 0.1);
    lightColorButton_ = new QPushButton(lightPage);
    lightIntensity_ = createDoubleSpinBox(0.0, 1000.0, 1.0);
    lightLayout->addRow(QStringLiteral("Position X"), lightPositionX_);
    lightLayout->addRow(QStringLiteral("Position Y"), lightPositionY_);
    lightLayout->addRow(QStringLiteral("Position Z"), lightPositionZ_);
    lightLayout->addRow(QStringLiteral("Color"), lightColorButton_);
    lightLayout->addRow(QStringLiteral("Intensity"), lightIntensity_);
    editorStack_->addWidget(lightPage);

    editorLayout->addWidget(editorStack_);
    layout->addWidget(editorGroup, 1);

    connect(presetComboBox_, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &ScenePanel::handlePresetChanged);
    connect(objectList_, &QListWidget::currentRowChanged, this, &ScenePanel::handleSelectionChanged);
    connect(addSphereButton_, &QPushButton::clicked, this, &ScenePanel::handleAddSphere);
    connect(addPlaneButton_, &QPushButton::clicked, this, &ScenePanel::handleAddPlane);
    connect(addLightButton_, &QPushButton::clicked, this, &ScenePanel::handleAddLight);
    connect(deleteButton_, &QPushButton::clicked, this, &ScenePanel::handleDeleteSelected);

    for (QDoubleSpinBox* spinBox : {sphereCenterX_, sphereCenterY_, sphereCenterZ_, sphereRadius_, sphereRoughness_, sphereRefractiveIndex_}) {
        connect(spinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ScenePanel::handleSphereChanged);
    }
    connect(sphereMaterialType_, qOverload<int>(&QComboBox::currentIndexChanged), this, &ScenePanel::handleSphereChanged);
    connect(sphereAlbedoButton_, &QPushButton::clicked, this, &ScenePanel::chooseSphereAlbedo);

    for (QDoubleSpinBox* spinBox : {planePointX_, planePointY_, planePointZ_, planeNormalX_, planeNormalY_, planeNormalZ_, planeRoughness_, planeRefractiveIndex_}) {
        connect(spinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ScenePanel::handlePlaneChanged);
    }
    connect(planeMaterialType_, qOverload<int>(&QComboBox::currentIndexChanged), this, &ScenePanel::handlePlaneChanged);
    connect(planeAlbedoButton_, &QPushButton::clicked, this, &ScenePanel::choosePlaneAlbedo);

    for (QDoubleSpinBox* spinBox : {lightPositionX_, lightPositionY_, lightPositionZ_, lightIntensity_}) {
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
    }
}

void ScenePanel::handleAddSphere()
{
    tinyray::Material material;
    material.albedo = tinyray::Vec3(0.80, 0.35, 0.25);
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

void ScenePanel::handleAddLight()
{
    scene_.addLight(tinyray::Light(tinyray::Vec3(2.0, 3.0, 2.0), tinyray::Vec3(1.0, 0.95, 0.85), 16.0));
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
    light->color = colorToVec3(lightColor_);
    light->intensity = lightIntensity_->value();
    rebuildObjectList(2, objectList_->currentItem()->data(Qt::UserRole + 1).toInt());
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

void ScenePanel::choosePlaneAlbedo()
{
    const QColor color = QColorDialog::getColor(planeAlbedo_, this, QStringLiteral("Choose Albedo"));
    if (color.isValid()) {
        planeAlbedo_ = color;
        updateColorButton(planeAlbedoButton_, planeAlbedo_);
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
        auto* item = new QListWidgetItem(QStringLiteral("Point Light %1").arg(index + 1), objectList_);
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
        editorStack_->setCurrentIndex(0);
        return;
    }

    const QString kind = item->data(Qt::UserRole).toString();
    const int index = item->data(Qt::UserRole + 1).toInt();
    updating_ = true;
    if (kind == "object" && index >= 0 && index < static_cast<int>(scene_.objects.size())) {
        const auto& object = scene_.objects[static_cast<std::size_t>(index)];
        if (const auto* sphere = dynamic_cast<const tinyray::Sphere*>(object.get())) {
            setSphereEditor(*sphere);
            editorStack_->setCurrentIndex(1);
        } else if (const auto* plane = dynamic_cast<const tinyray::Plane*>(object.get())) {
            setPlaneEditor(*plane);
            editorStack_->setCurrentIndex(2);
        } else {
            editorStack_->setCurrentIndex(0);
        }
    } else if (kind == "light" && index >= 0 && index < static_cast<int>(scene_.lights.size())) {
        setLightEditor(scene_.lights[static_cast<std::size_t>(index)]);
        editorStack_->setCurrentIndex(3);
    } else {
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

void ScenePanel::setSphereEditor(const tinyray::Sphere& sphere)
{
    sphereCenterX_->setValue(sphere.center.x);
    sphereCenterY_->setValue(sphere.center.y);
    sphereCenterZ_->setValue(sphere.center.z);
    sphereRadius_->setValue(sphere.radius);
    writeMaterialToControls(sphere.material, sphereMaterialType_, sphereRoughness_,
                            sphereRefractiveIndex_, sphereAlbedo_, sphereAlbedoButton_);
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
}

void ScenePanel::setLightEditor(const tinyray::Light& light)
{
    lightPositionX_->setValue(light.position.x);
    lightPositionY_->setValue(light.position.y);
    lightPositionZ_->setValue(light.position.z);
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
    return material;
}

tinyray::Material ScenePanel::readPlaneMaterial() const
{
    tinyray::Material material;
    material.type = materialTypeFromIndex(planeMaterialType_->currentIndex());
    material.albedo = colorToVec3(planeAlbedo_);
    material.roughness = planeRoughness_->value();
    material.refractiveIndex = planeRefractiveIndex_->value();
    return material;
}

void ScenePanel::writeMaterialToControls(const tinyray::Material& material,
                                         QComboBox* typeCombo,
                                         QDoubleSpinBox* roughnessSpin,
                                         QDoubleSpinBox* refractiveSpin,
                                         QColor& colorCache,
                                         QPushButton* colorButton)
{
    typeCombo->setCurrentIndex(materialTypeToIndex(material.type));
    roughnessSpin->setValue(material.roughness);
    refractiveSpin->setValue(material.refractiveIndex);
    colorCache = vec3ToColor(material.albedo);
    updateColorButton(colorButton, colorCache);
}
