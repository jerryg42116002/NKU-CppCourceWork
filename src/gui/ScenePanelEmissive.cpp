#include "gui/ScenePanel.h"

#include <algorithm>

#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QString>

namespace {

int materialIndex(tinyray::MaterialType type)
{
    switch (type) {
    case tinyray::MaterialType::Diffuse:
        return 0;
    case tinyray::MaterialType::Metal:
        return 1;
    case tinyray::MaterialType::Glass:
        return 2;
    case tinyray::MaterialType::Emissive:
        return 3;
    }

    return 0;
}

QColor vecToColor(const tinyray::Vec3& color)
{
    const auto toChannel = [](double value) {
        return static_cast<int>(std::clamp(value, 0.0, 1.0) * 255.0 + 0.5);
    };

    return QColor(toChannel(color.x), toChannel(color.y), toChannel(color.z));
}

void setButtonColor(QPushButton* button, const QColor& color)
{
    if (!button) {
        return;
    }

    button->setText(color.name(QColor::HexRgb).toUpper());
    button->setStyleSheet(QString("background-color: %1; color: %2;")
                              .arg(color.name(QColor::HexRgb),
                                   color.lightness() < 128 ? "#ffffff" : "#111111"));
}

} // namespace

void ScenePanel::writeMaterialToControls(const tinyray::Material& material,
                                         QComboBox* typeCombo,
                                         QDoubleSpinBox* roughnessSpin,
                                         QDoubleSpinBox* refractiveSpin,
                                         QColor& colorCache,
                                         QPushButton* colorButton)
{
    if (typeCombo == sphereMaterialType_) {
        writeMaterialToControls(material,
                                typeCombo,
                                roughnessSpin,
                                refractiveSpin,
                                sphereEmissionColor_,
                                sphereEmissionColorButton_,
                                sphereEmissionStrength_,
                                colorCache,
                                colorButton);
        return;
    }

    if (typeCombo == boxMaterialType_) {
        writeMaterialToControls(material,
                                typeCombo,
                                roughnessSpin,
                                refractiveSpin,
                                boxEmissionColor_,
                                boxEmissionColorButton_,
                                boxEmissionStrength_,
                                colorCache,
                                colorButton);
        return;
    }

    if (typeCombo == cylinderMaterialType_) {
        writeMaterialToControls(material,
                                typeCombo,
                                roughnessSpin,
                                refractiveSpin,
                                cylinderEmissionColor_,
                                cylinderEmissionColorButton_,
                                cylinderEmissionStrength_,
                                colorCache,
                                colorButton);
        return;
    }

    if (typeCombo == planeMaterialType_) {
        writeMaterialToControls(material,
                                typeCombo,
                                roughnessSpin,
                                refractiveSpin,
                                planeEmissionColor_,
                                planeEmissionColorButton_,
                                planeEmissionStrength_,
                                colorCache,
                                colorButton);
        return;
    }

    typeCombo->setCurrentIndex(materialIndex(material.type));
    roughnessSpin->setValue(material.roughness);
    refractiveSpin->setValue(material.refractiveIndex);
    colorCache = vecToColor(material.albedo);
    setButtonColor(colorButton, colorCache);
}

void ScenePanel::chooseSphereEmissionColor()
{
    const QColor color = QColorDialog::getColor(sphereEmissionColor_, this, "Choose sphere emission color");
    if (!color.isValid()) {
        return;
    }

    sphereEmissionColor_ = color;
    updateColorButton(sphereEmissionColorButton_, sphereEmissionColor_);
    handleSphereChanged();
}

void ScenePanel::chooseBoxEmissionColor()
{
    const QColor color = QColorDialog::getColor(boxEmissionColor_, this, "Choose box emission color");
    if (!color.isValid()) {
        return;
    }

    boxEmissionColor_ = color;
    updateColorButton(boxEmissionColorButton_, boxEmissionColor_);
    handleBoxChanged();
}

void ScenePanel::chooseCylinderEmissionColor()
{
    const QColor color = QColorDialog::getColor(cylinderEmissionColor_, this, "Choose cylinder emission color");
    if (!color.isValid()) {
        return;
    }

    cylinderEmissionColor_ = color;
    updateColorButton(cylinderEmissionColorButton_, cylinderEmissionColor_);
    handleCylinderChanged();
}

void ScenePanel::choosePlaneEmissionColor()
{
    const QColor color = QColorDialog::getColor(planeEmissionColor_, this, "Choose plane emission color");
    if (!color.isValid()) {
        return;
    }

    planeEmissionColor_ = color;
    updateColorButton(planeEmissionColorButton_, planeEmissionColor_);
    handlePlaneChanged();
}
