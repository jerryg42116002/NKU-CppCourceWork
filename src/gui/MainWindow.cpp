#include "gui/MainWindow.h"

#include <algorithm>
#include <cstddef>
#include <system_error>

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QVariant>
#include <QVBoxLayout>
#include <QWidget>

#include "core/SceneIO.h"
#include "core/Box.h"
#include "core/Cylinder.h"
#include "core/Sphere.h"
#include "gui/RealTimeRenderWidget.h"
#include "gui/ScenePanel.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("TinyRay Studio"));
    resize(1024, 640);
    setMinimumSize(860, 520);
    scene_ = tinyray::Scene::createDefaultScene();

    realTimeRenderWidget_ = new RealTimeRenderWidget(this);
    realTimeRenderWidget_->setScene(scene_);
    connect(realTimeRenderWidget_, &RealTimeRenderWidget::objectSelected,
            this, &MainWindow::handleRealTimeObjectSelected);
    connect(realTimeRenderWidget_, &RealTimeRenderWidget::lightSelected,
            this, &MainWindow::handleRealTimeLightSelected);
    connect(realTimeRenderWidget_, &RealTimeRenderWidget::objectMoved,
            this, &MainWindow::handleRealTimeObjectMoved);
    connect(realTimeRenderWidget_, &RealTimeRenderWidget::lightMoved,
            this, &MainWindow::handleRealTimeLightMoved);
    setCentralWidget(realTimeRenderWidget_);

    createMenus();
    createControlPanel();
    createStatusBar();
    connect(realTimeRenderWidget_, &RealTimeRenderWidget::interactionStatusChanged,
            this, [this](const QString& status) {
                if (statusLabel_ != nullptr) {
                    statusLabel_->setText(status);
                }
            });
    connect(realTimeRenderWidget_, &RealTimeRenderWidget::turntablePausedByUserInput,
            this, [this]() {
                setTurntableControlsRunning(false);
                if (statusLabel_ != nullptr) {
                    statusLabel_->setText(QStringLiteral("Turntable paused by user input"));
                }
            });

    connect(this, &MainWindow::renderProgressChanged,
            this, &MainWindow::updateRenderProgress,
            Qt::QueuedConnection);
    connect(this, &MainWindow::renderPreviewUpdated,
            this, &MainWindow::updateRenderPreview,
            Qt::QueuedConnection);
    connect(this, &MainWindow::renderFinished,
            this, &MainWindow::handleRenderFinished,
            Qt::QueuedConnection);
}

MainWindow::~MainWindow()
{
    renderer_.requestStop();
    if (renderThread_.joinable()) {
        renderThread_.join();
    }
}

void MainWindow::createMenus()
{
    auto* fileMenu = menuBar()->addMenu(QStringLiteral("File"));
    QAction* loadSceneAction = fileMenu->addAction(QStringLiteral("Load Scene"));
    QAction* saveSceneAction = fileMenu->addAction(QStringLiteral("Save Scene"));
    QAction* saveImageAction = fileMenu->addAction(QStringLiteral("Save Image"));
    fileMenu->addSeparator();
    QAction* exitAction = fileMenu->addAction(QStringLiteral("Exit"));

    auto* renderMenu = menuBar()->addMenu(QStringLiteral("Render"));
    QAction* startRenderAction = renderMenu->addAction(QStringLiteral("High Quality Render"));
    QAction* stopRenderAction = renderMenu->addAction(QStringLiteral("Stop Render"));
    QAction* clearImageAction = renderMenu->addAction(QStringLiteral("Clear Image"));

    auto* helpMenu = menuBar()->addMenu(QStringLiteral("Help"));
    QAction* aboutAction = helpMenu->addAction(QStringLiteral("About"));

    connect(loadSceneAction, &QAction::triggered, this, &MainWindow::handleLoadScene);
    connect(saveSceneAction, &QAction::triggered, this, &MainWindow::handleSaveScene);
    connect(saveImageAction, &QAction::triggered, this, &MainWindow::handleSaveImage);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    connect(startRenderAction, &QAction::triggered, this, &MainWindow::handleRender);
    connect(stopRenderAction, &QAction::triggered, this, &MainWindow::handleStop);
    connect(clearImageAction, &QAction::triggered, this, &MainWindow::handleClearImage);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::handleAbout);
}

void MainWindow::createControlPanel()
{
    auto* dock = new QDockWidget(QStringLiteral("Render Settings"), this);
    dock->setAllowedAreas(Qt::RightDockWidgetArea);
    dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    auto* scrollArea = new QScrollArea(dock);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto* panel = new QWidget(scrollArea);
    auto* panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(8, 8, 8, 8);
    panelLayout->setSpacing(8);

    auto* settingsGroup = new QGroupBox(QStringLiteral("Render Settings"), panel);
    auto* settingsLayout = new QFormLayout(settingsGroup);
    settingsLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    settingsLayout->setLabelAlignment(Qt::AlignLeft);

    const tinyray::RenderSettings defaults;

    widthSpinBox_ = new QSpinBox(settingsGroup);
    widthSpinBox_->setRange(64, 4096);
    widthSpinBox_->setSingleStep(64);
    widthSpinBox_->setSuffix(QStringLiteral(" px"));
    widthSpinBox_->setValue(defaults.width);

    heightSpinBox_ = new QSpinBox(settingsGroup);
    heightSpinBox_->setRange(64, 4096);
    heightSpinBox_->setSingleStep(64);
    heightSpinBox_->setSuffix(QStringLiteral(" px"));
    heightSpinBox_->setValue(defaults.height);

    samplesComboBox_ = new QComboBox(settingsGroup);
    const int sampleOptions[] = {1, 4, 8, 16, 32, 64};
    for (const int sampleCount : sampleOptions) {
        samplesComboBox_->addItem(QString::number(sampleCount));
    }
    const int defaultSamplesIndex = samplesComboBox_->findText(QString::number(defaults.samplesPerPixel));
    samplesComboBox_->setCurrentIndex(defaultSamplesIndex >= 0 ? defaultSamplesIndex : 3);

    maxDepthSpinBox_ = new QSpinBox(settingsGroup);
    maxDepthSpinBox_->setRange(1, 64);
    maxDepthSpinBox_->setValue(defaults.maxDepth);

    threadsSpinBox_ = new QSpinBox(settingsGroup);
    threadsSpinBox_->setRange(1, 128);
    threadsSpinBox_->setValue(std::clamp(defaults.threadCount, 1, 128));

    dragModeComboBox_ = new QComboBox(settingsGroup);
    dragModeComboBox_->addItem(QStringLiteral("Move XZ Plane"), static_cast<int>(tinyray::ObjectDragMode::PlaneXZ));
    dragModeComboBox_->addItem(QStringLiteral("Move XY Plane"), static_cast<int>(tinyray::ObjectDragMode::PlaneXY));
    dragModeComboBox_->addItem(QStringLiteral("Move YZ Plane"), static_cast<int>(tinyray::ObjectDragMode::PlaneYZ));
    dragModeComboBox_->addItem(QStringLiteral("Move X Axis"), static_cast<int>(tinyray::ObjectDragMode::AxisX));
    dragModeComboBox_->addItem(QStringLiteral("Move Y Axis"), static_cast<int>(tinyray::ObjectDragMode::AxisY));
    dragModeComboBox_->addItem(QStringLiteral("Move Z Axis"), static_cast<int>(tinyray::ObjectDragMode::AxisZ));

    settingsLayout->addRow(QStringLiteral("Width"), widthSpinBox_);
    settingsLayout->addRow(QStringLiteral("Height"), heightSpinBox_);
    settingsLayout->addRow(QStringLiteral("Samples"), samplesComboBox_);
    settingsLayout->addRow(QStringLiteral("Max Depth"), maxDepthSpinBox_);
    settingsLayout->addRow(QStringLiteral("Threads"), threadsSpinBox_);
    settingsLayout->addRow(QStringLiteral("Drag Mode"), dragModeComboBox_);

    auto* cameraGroup = new QGroupBox(QStringLiteral("Camera"), panel);
    auto* cameraLayout = new QFormLayout(cameraGroup);
    cameraLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    startTurntableButton_ = new QPushButton(QStringLiteral("Start Turntable"), cameraGroup);
    stopTurntableButton_ = new QPushButton(QStringLiteral("Stop Turntable"), cameraGroup);
    auto* turntableButtonRow = new QWidget(cameraGroup);
    auto* turntableButtonLayout = new QHBoxLayout(turntableButtonRow);
    turntableButtonLayout->setContentsMargins(0, 0, 0, 0);
    turntableButtonLayout->setSpacing(6);
    turntableButtonLayout->addWidget(startTurntableButton_);
    turntableButtonLayout->addWidget(stopTurntableButton_);

    turntableSpeedSlider_ = new QSlider(Qt::Horizontal, cameraGroup);
    turntableSpeedSlider_->setRange(0, 120);
    turntableSpeedSlider_->setSingleStep(1);
    turntableSpeedSlider_->setPageStep(5);
    turntableSpeedSlider_->setValue(24);
    turntableSpeedValueLabel_ = new QLabel(QStringLiteral("24 deg/s"), cameraGroup);
    turntableSpeedValueLabel_->setMinimumWidth(72);
    auto* turntableSpeedRow = new QWidget(cameraGroup);
    auto* turntableSpeedLayout = new QHBoxLayout(turntableSpeedRow);
    turntableSpeedLayout->setContentsMargins(0, 0, 0, 0);
    turntableSpeedLayout->setSpacing(6);
    turntableSpeedLayout->addWidget(turntableSpeedSlider_, 1);
    turntableSpeedLayout->addWidget(turntableSpeedValueLabel_);

    turntableDirectionComboBox_ = new QComboBox(cameraGroup);
    turntableDirectionComboBox_->addItem(QStringLiteral("Clockwise"),
                                         static_cast<int>(tinyray::TurntableDirection::Clockwise));
    turntableDirectionComboBox_->addItem(QStringLiteral("Counterclockwise"),
                                         static_cast<int>(tinyray::TurntableDirection::Counterclockwise));
    turntableDirectionComboBox_->setCurrentIndex(1);

    turntableTargetModeComboBox_ = new QComboBox(cameraGroup);
    turntableTargetModeComboBox_->addItem(QStringLiteral("Scene Center"),
                                          static_cast<int>(tinyray::TurntableTargetMode::SceneCenter));
    turntableTargetModeComboBox_->addItem(QStringLiteral("Selected Object"),
                                          static_cast<int>(tinyray::TurntableTargetMode::SelectedObject));
    turntableTargetModeComboBox_->addItem(QStringLiteral("Custom Target"),
                                          static_cast<int>(tinyray::TurntableTargetMode::CustomTarget));

    resetViewButton_ = new QPushButton(QStringLiteral("Reset View"), cameraGroup);
    focusSelectedButton_ = new QPushButton(QStringLiteral("Focus Selected Object"), cameraGroup);
    auto* viewButtonRow = new QWidget(cameraGroup);
    auto* viewButtonLayout = new QHBoxLayout(viewButtonRow);
    viewButtonLayout->setContentsMargins(0, 0, 0, 0);
    viewButtonLayout->setSpacing(6);
    viewButtonLayout->addWidget(resetViewButton_);
    viewButtonLayout->addWidget(focusSelectedButton_);

    cameraLayout->addRow(QStringLiteral("Turntable"), turntableButtonRow);
    cameraLayout->addRow(QStringLiteral("Speed"), turntableSpeedRow);
    cameraLayout->addRow(QStringLiteral("Direction"), turntableDirectionComboBox_);
    cameraLayout->addRow(QStringLiteral("Target Mode"), turntableTargetModeComboBox_);
    cameraLayout->addRow(QStringLiteral("View"), viewButtonRow);

    scenePanel_ = new ScenePanel(panel);
    scenePanel_->setScene(scene_);

    auto* outputGroup = new QGroupBox(QStringLiteral("Output"), panel);
    auto* outputLayout = new QVBoxLayout(outputGroup);
    outputLayout->setSpacing(8);

    renderButton_ = new QPushButton(QStringLiteral("High Quality Render"), outputGroup);
    stopButton_ = new QPushButton(QStringLiteral("Stop"), outputGroup);
    saveImageButton_ = new QPushButton(QStringLiteral("Save Image"), outputGroup);
    saveSceneButton_ = new QPushButton(QStringLiteral("Save Scene"), outputGroup);
    loadSceneButton_ = new QPushButton(QStringLiteral("Load Scene"), outputGroup);
    clearButton_ = new QPushButton(QStringLiteral("Clear"), outputGroup);

    outputLayout->addWidget(renderButton_);
    outputLayout->addWidget(stopButton_);
    outputLayout->addWidget(saveImageButton_);
    outputLayout->addWidget(saveSceneButton_);
    outputLayout->addWidget(loadSceneButton_);
    outputLayout->addWidget(clearButton_);

    panelLayout->addWidget(settingsGroup);
    panelLayout->addWidget(cameraGroup);
    panelLayout->addWidget(scenePanel_, 1);
    panelLayout->addWidget(outputGroup);

    scrollArea->setWidget(panel);
    dock->setWidget(scrollArea);
    dock->setMinimumWidth(240);
    dock->resize(300, height());
    addDockWidget(Qt::RightDockWidgetArea, dock);

    connect(renderButton_, &QPushButton::clicked, this, &MainWindow::handleRender);
    connect(stopButton_, &QPushButton::clicked, this, &MainWindow::handleStop);
    connect(saveImageButton_, &QPushButton::clicked, this, &MainWindow::handleSaveImage);
    connect(saveSceneButton_, &QPushButton::clicked, this, &MainWindow::handleSaveScene);
    connect(loadSceneButton_, &QPushButton::clicked, this, &MainWindow::handleLoadScene);
    connect(clearButton_, &QPushButton::clicked, this, &MainWindow::handleClearImage);
    connect(scenePanel_, &ScenePanel::sceneChanged, this, &MainWindow::handleSceneChanged);
    connect(dragModeComboBox_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (realTimeRenderWidget_ == nullptr) {
            return;
        }
        const auto mode = static_cast<tinyray::ObjectDragMode>(dragModeComboBox_->itemData(index).toInt());
        realTimeRenderWidget_->setObjectDragMode(mode);
        statusLabel_->setText(QStringLiteral("Drag mode updated"));
    });
    connect(startTurntableButton_, &QPushButton::clicked, this, &MainWindow::handleStartTurntable);
    connect(stopTurntableButton_, &QPushButton::clicked, this, &MainWindow::handleStopTurntable);
    connect(turntableSpeedSlider_, &QSlider::valueChanged,
            this, &MainWindow::handleTurntableSpeedChanged);
    connect(turntableDirectionComboBox_, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MainWindow::handleTurntableDirectionChanged);
    connect(turntableTargetModeComboBox_, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MainWindow::handleTurntableTargetModeChanged);
    connect(resetViewButton_, &QPushButton::clicked, this, &MainWindow::handleResetView);
    connect(focusSelectedButton_, &QPushButton::clicked, this, &MainWindow::handleFocusSelectedObject);

    setRenderControlsEnabled(true);
    setTurntableControlsRunning(false);
    handleTurntableSpeedChanged(turntableSpeedSlider_->value());
    handleTurntableDirectionChanged(turntableDirectionComboBox_->currentIndex());
    handleTurntableTargetModeChanged(turntableTargetModeComboBox_->currentIndex());
}

void MainWindow::createStatusBar()
{
    statusLabel_ = new QLabel(QStringLiteral("Real-time rendering"), this);
    statusLabel_->setMinimumWidth(220);

    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    progressBar_->setTextVisible(true);
    progressBar_->setFixedWidth(220);

    statusBar()->addWidget(statusLabel_, 1);
    statusBar()->addPermanentWidget(progressBar_);
}

void MainWindow::setRenderControlsEnabled(bool enabled)
{
    renderButton_->setEnabled(enabled);
    saveImageButton_->setEnabled(true);
    saveSceneButton_->setEnabled(enabled);
    loadSceneButton_->setEnabled(enabled);
    clearButton_->setEnabled(enabled);
    stopButton_->setEnabled(!enabled);
}

void MainWindow::setTurntableControlsRunning(bool running)
{
    if (startTurntableButton_ != nullptr) {
        startTurntableButton_->setEnabled(!running);
    }
    if (stopTurntableButton_ != nullptr) {
        stopTurntableButton_->setEnabled(running);
    }
}

void MainWindow::applyRenderSettings(const tinyray::RenderSettings& settings)
{
    widthSpinBox_->setValue(std::clamp(settings.width, widthSpinBox_->minimum(), widthSpinBox_->maximum()));
    heightSpinBox_->setValue(std::clamp(settings.height, heightSpinBox_->minimum(), heightSpinBox_->maximum()));

    const int sampleIndex = samplesComboBox_->findText(QString::number(settings.samplesPerPixel));
    samplesComboBox_->setCurrentIndex(sampleIndex >= 0 ? sampleIndex : samplesComboBox_->findText(QStringLiteral("16")));

    maxDepthSpinBox_->setValue(std::clamp(settings.maxDepth, maxDepthSpinBox_->minimum(), maxDepthSpinBox_->maximum()));
    threadsSpinBox_->setValue(std::clamp(settings.threadCount, threadsSpinBox_->minimum(), threadsSpinBox_->maximum()));
}

tinyray::RenderSettings MainWindow::currentRenderSettings() const
{
    tinyray::RenderSettings settings;
    settings.width = widthSpinBox_->value();
    settings.height = heightSpinBox_->value();
    settings.samplesPerPixel = samplesComboBox_->currentText().toInt();
    settings.maxDepth = maxDepthSpinBox_->value();
    settings.threadCount = threadsSpinBox_->value();
    return settings;
}

void MainWindow::syncCurrentViewportCameraToScene()
{
    if (realTimeRenderWidget_ != nullptr) {
        scene_.camera = realTimeRenderWidget_->currentCameraSnapshot();
    }
}

void MainWindow::handleRender()
{
    if (renderThread_.joinable()) {
        statusLabel_->setText(QStringLiteral("Render already running"));
        return;
    }

    if (realTimeRenderWidget_ != nullptr) {
        realTimeRenderWidget_->setTurntableEnabled(false);
        setTurntableControlsRunning(false);
    }
    syncCurrentViewportCameraToScene();
    const tinyray::RenderSettings settings = currentRenderSettings();
    const tinyray::Scene scene = scene_;

    setRenderControlsEnabled(false);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    statusLabel_->setText(QStringLiteral("High quality rendering | Sample 0/%1 | 0% | 0.0s").arg(settings.samplesPerPixel));
    progressBar_->setValue(0);

    renderer_.resetStopFlag();
    try {
        renderThread_ = std::thread([this, scene, settings]() {
            const QImage image = renderer_.render(scene, settings, [this](int progress) {
                emit renderProgressChanged(progress);
            }, [this](const QImage& image,
                      int currentSample,
                      int totalSamples,
                      int progress,
                      double elapsedSeconds) {
                emit renderPreviewUpdated(image, currentSample, totalSamples, progress, elapsedSeconds);
            });

            emit renderFinished(image, renderer_.stopRequested());
        });
    } catch (const std::system_error& error) {
        QApplication::restoreOverrideCursor();
        setRenderControlsEnabled(true);
        statusLabel_->setText(QStringLiteral("Failed to start render thread"));
        QMessageBox::warning(this,
                             QStringLiteral("Render"),
                             QStringLiteral("Failed to start render thread: %1").arg(error.what()));
    }
}

void MainWindow::handleStop()
{
    if (!renderThread_.joinable()) {
        statusLabel_->setText(QStringLiteral("No active render"));
        return;
    }

    renderer_.requestStop();
    statusLabel_->setText(QStringLiteral("Stop requested"));
}

void MainWindow::handleSaveImage()
{
    QImage imageToSave = highQualityImage_;
    if (imageToSave.isNull() && realTimeRenderWidget_ != nullptr) {
        imageToSave = realTimeRenderWidget_->grabFramebuffer();
    }

    if (imageToSave.isNull()) {
        QMessageBox::information(this,
                                 QStringLiteral("Save Image"),
                                 QStringLiteral("There is no image to save."));
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Save Rendered Image"),
        QStringLiteral("tinyray_render.png"),
        QStringLiteral("PNG Images (*.png)"));

    if (fileName.isEmpty()) {
        return;
    }

    if (QFileInfo(fileName).suffix().isEmpty()) {
        fileName += QStringLiteral(".png");
    }

    if (!imageToSave.save(fileName, "PNG")) {
        QMessageBox::warning(this, QStringLiteral("Save Image"),
                             QStringLiteral("Failed to save the PNG image."));
        return;
    }

    statusLabel_->setText(highQualityImage_.isNull()
                              ? QStringLiteral("Real-time viewport image saved")
                              : QStringLiteral("High quality image saved"));
}

void MainWindow::handleClearImage()
{
    highQualityImage_ = QImage();
    progressBar_->setValue(0);
    statusLabel_->setText(QStringLiteral("Real-time rendering"));
}

void MainWindow::handleSaveScene()
{
    syncCurrentViewportCameraToScene();

    const QString fileName = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Save Scene"),
        QStringLiteral("tinyray_scene.json"),
        QStringLiteral("TinyRay Scene (*.json)"));

    if (fileName.isEmpty()) {
        return;
    }

    QString errorMessage;
    if (!tinyray::SceneIO::saveToFile(scene_, currentRenderSettings(), fileName, &errorMessage)) {
        QMessageBox::warning(this, QStringLiteral("Save Scene"), errorMessage);
        return;
    }

    statusLabel_->setText(QStringLiteral("Scene saved"));
}

void MainWindow::handleLoadScene()
{
    if (renderThread_.joinable()) {
        QMessageBox::information(this,
                                 QStringLiteral("Load Scene"),
                                 QStringLiteral("Stop the current render before loading a scene."));
        return;
    }

    const QString fileName = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Load Scene"),
        QString(),
        QStringLiteral("TinyRay Scene (*.json)"));

    if (fileName.isEmpty()) {
        return;
    }

    tinyray::Scene loadedScene;
    tinyray::RenderSettings loadedSettings = currentRenderSettings();
    QString errorMessage;
    if (!tinyray::SceneIO::loadFromFile(fileName, loadedScene, loadedSettings, &errorMessage)) {
        QMessageBox::warning(this, QStringLiteral("Load Scene"), errorMessage);
        return;
    }

    scene_ = loadedScene;
    scenePanel_->setScene(scene_);
    scenePanel_->setSelectedObjectId(scene_.selectedObjectId);
    realTimeRenderWidget_->setTurntableEnabled(false);
    setTurntableControlsRunning(false);
    realTimeRenderWidget_->setScene(scene_);
    realTimeRenderWidget_->setSelectedObjectId(scene_.selectedObjectId);
    applyRenderSettings(loadedSettings);
    highQualityImage_ = QImage();
    progressBar_->setValue(0);
    statusLabel_->setText(QStringLiteral("Scene loaded"));
}

void MainWindow::handleAbout()
{
    QMessageBox::about(
        this,
        QStringLiteral("About TinyRay Studio"),
        QStringLiteral("TinyRay Studio\n\n"
                       "C++17 + Qt Widgets + CPU Ray Tracing\n\n"
                       "Supports real-time OpenGL interaction, object selection, sphere dragging, "
                       "shadows, diffuse shading, metal reflection, glass refraction, "
                       "anti-aliasing, gamma correction, multi-threaded high quality rendering, "
                       "scene editing, and PNG export."));
}

void MainWindow::handleSceneChanged(const tinyray::Scene& scene)
{
    const int incomingSelection = scene.selectedObjectId;
    scene_ = scene;
    if (incomingSelection >= 0 && scene_.containsObjectId(incomingSelection)) {
        scene_.selectedObjectId = incomingSelection;
    } else {
        scene_.selectedObjectId = -1;
    }

    realTimeRenderWidget_->setScene(scene_);
    scenePanel_->setSelectedObjectId(scene_.selectedObjectId);
    highQualityImage_ = QImage();
    statusLabel_->setText(QStringLiteral("Scene updated"));
}

void MainWindow::handleRealTimeObjectSelected(int objectId)
{
    scene_.selectedObjectId = objectId;
    scenePanel_->setSelectedObjectId(objectId);
    if (objectId < 0) {
        statusLabel_->setText(QStringLiteral("Real-time rendering"));
        return;
    }

    statusLabel_->setText(QStringLiteral("Real-time rendering | Selected %1").arg(scene_.objectLabel(objectId)));
}

void MainWindow::handleRealTimeLightSelected(int lightIndex)
{
    scene_.selectedObjectId = -1;
    scenePanel_->setSelectedLightIndex(lightIndex);
    if (lightIndex < 0 || lightIndex >= static_cast<int>(scene_.lights.size())) {
        statusLabel_->setText(QStringLiteral("Real-time rendering"));
        return;
    }

    statusLabel_->setText(QStringLiteral("Real-time rendering | Selected Point Light %1").arg(lightIndex + 1));
}

void MainWindow::handleRealTimeObjectMoved(int objectId, double x, double y, double z)
{
    tinyray::Object* object = scene_.objectById(objectId);
    if (object == nullptr) {
        return;
    }

    const tinyray::Vec3 newCenter(x, y, z);
    if (auto* sphere = dynamic_cast<tinyray::Sphere*>(object)) {
        sphere->center = newCenter;
    } else if (auto* box = dynamic_cast<tinyray::Box*>(object)) {
        box->center = newCenter;
    } else if (auto* cylinder = dynamic_cast<tinyray::Cylinder*>(object)) {
        cylinder->center = newCenter;
    } else {
        return;
    }

    scene_.selectedObjectId = objectId;
    highQualityImage_ = QImage();
    scenePanel_->setScene(scene_);
    scenePanel_->setSelectedObjectId(objectId);
    statusLabel_->setText(QStringLiteral("Object dragging | %1").arg(scene_.objectLabel(objectId)));
}

void MainWindow::handleRealTimeLightMoved(int lightIndex, double x, double y, double z)
{
    if (lightIndex < 0 || lightIndex >= static_cast<int>(scene_.lights.size())) {
        return;
    }

    scene_.lights[static_cast<std::size_t>(lightIndex)].position = tinyray::Vec3(x, y, z);
    scene_.selectedObjectId = -1;
    highQualityImage_ = QImage();
    scenePanel_->setScene(scene_);
    scenePanel_->setSelectedLightIndex(lightIndex);
    statusLabel_->setText(QStringLiteral("Light dragging | Point Light %1").arg(lightIndex + 1));
}

void MainWindow::handleStartTurntable()
{
    if (realTimeRenderWidget_ == nullptr) {
        return;
    }

    handleTurntableSpeedChanged(turntableSpeedSlider_ != nullptr ? turntableSpeedSlider_->value() : 24);
    handleTurntableDirectionChanged(turntableDirectionComboBox_ != nullptr ? turntableDirectionComboBox_->currentIndex() : 1);
    handleTurntableTargetModeChanged(turntableTargetModeComboBox_ != nullptr ? turntableTargetModeComboBox_->currentIndex() : 0);
    realTimeRenderWidget_->setTurntableEnabled(true);
    setTurntableControlsRunning(true);
    statusLabel_->setText(QStringLiteral("Turntable started"));
}

void MainWindow::handleStopTurntable()
{
    if (realTimeRenderWidget_ != nullptr) {
        realTimeRenderWidget_->setTurntableEnabled(false);
    }
    setTurntableControlsRunning(false);
    statusLabel_->setText(QStringLiteral("Turntable stopped"));
}

void MainWindow::handleTurntableSpeedChanged(int value)
{
    const double speed = static_cast<double>(std::max(value, 0));
    if (turntableSpeedValueLabel_ != nullptr) {
        turntableSpeedValueLabel_->setText(QStringLiteral("%1 deg/s").arg(value));
    }
    if (realTimeRenderWidget_ != nullptr) {
        realTimeRenderWidget_->setTurntableSpeed(speed);
    }
}

void MainWindow::handleTurntableDirectionChanged(int index)
{
    if (realTimeRenderWidget_ == nullptr || turntableDirectionComboBox_ == nullptr) {
        return;
    }

    const QVariant directionData = turntableDirectionComboBox_->itemData(index);
    const int directionValue = directionData.isValid()
        ? directionData.toInt()
        : static_cast<int>(tinyray::TurntableDirection::Counterclockwise);
    realTimeRenderWidget_->setTurntableDirection(
        directionValue < 0
            ? tinyray::TurntableDirection::Clockwise
            : tinyray::TurntableDirection::Counterclockwise);
}

void MainWindow::handleTurntableTargetModeChanged(int index)
{
    if (realTimeRenderWidget_ == nullptr || turntableTargetModeComboBox_ == nullptr) {
        return;
    }

    const QVariant modeData = turntableTargetModeComboBox_->itemData(index);
    const int modeValue = modeData.isValid()
        ? modeData.toInt()
        : static_cast<int>(tinyray::TurntableTargetMode::SceneCenter);
    realTimeRenderWidget_->setTurntableTargetMode(static_cast<tinyray::TurntableTargetMode>(modeValue));
}

void MainWindow::handleResetView()
{
    if (realTimeRenderWidget_ == nullptr) {
        return;
    }

    realTimeRenderWidget_->setTurntableEnabled(false);
    realTimeRenderWidget_->resetView();
    setTurntableControlsRunning(false);
    syncCurrentViewportCameraToScene();
    highQualityImage_ = QImage();
    statusLabel_->setText(QStringLiteral("Camera view reset"));
}

void MainWindow::handleFocusSelectedObject()
{
    if (realTimeRenderWidget_ == nullptr) {
        return;
    }

    if (turntableTargetModeComboBox_ != nullptr) {
        const int selectedObjectIndex = turntableTargetModeComboBox_->findData(
            static_cast<int>(tinyray::TurntableTargetMode::SelectedObject));
        if (selectedObjectIndex >= 0) {
            turntableTargetModeComboBox_->setCurrentIndex(selectedObjectIndex);
        }
    }

    if (!realTimeRenderWidget_->focusSelectedObject()) {
        statusLabel_->setText(QStringLiteral("No selected object to focus"));
        return;
    }

    syncCurrentViewportCameraToScene();
    highQualityImage_ = QImage();
    statusLabel_->setText(QStringLiteral("Focused selected object"));
}

void MainWindow::updateRenderProgress(int progress)
{
    const int clampedProgress = std::clamp(progress, 0, 100);
    progressBar_->setValue(clampedProgress);
}

void MainWindow::updateRenderPreview(const QImage& image,
                                     int currentSample,
                                     int totalSamples,
                                     int progress,
                                     double elapsedSeconds)
{
    highQualityImage_ = image;

    const int clampedProgress = std::clamp(progress, 0, 100);
    progressBar_->setValue(clampedProgress);
    statusLabel_->setText(QStringLiteral("High quality rendering | Sample %1/%2 | %3% | %4s")
                              .arg(currentSample)
                              .arg(totalSamples)
                              .arg(clampedProgress)
                              .arg(elapsedSeconds, 0, 'f', 1));
}

void MainWindow::handleRenderFinished(const QImage& image, bool stopped)
{
    if (renderThread_.joinable()) {
        renderThread_.join();
    }

    highQualityImage_ = image;
    QApplication::restoreOverrideCursor();
    setRenderControlsEnabled(true);

    if (stopped) {
        statusLabel_->setText(QStringLiteral("Render stopped"));
    } else {
        progressBar_->setValue(100);
        statusLabel_->setText(QStringLiteral("Render done"));
    }
}
