#include "gui/MainWindow.h"

#include <algorithm>
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
#include <QSpinBox>
#include <QStatusBar>
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
    connect(realTimeRenderWidget_, &RealTimeRenderWidget::objectMoved,
            this, &MainWindow::handleRealTimeObjectMoved);
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

    setRenderControlsEnabled(true);
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

void MainWindow::handleRender()
{
    if (renderThread_.joinable()) {
        statusLabel_->setText(QStringLiteral("Render already running"));
        return;
    }

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
    realTimeRenderWidget_->setScene(scene_);
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
    const int previousSelection = scene_.selectedObjectId;
    scene_ = scene;
    if (previousSelection >= 0 && scene_.containsObjectId(previousSelection)) {
        scene_.selectedObjectId = previousSelection;
    } else if (scene_.selectedObjectId >= 0 && !scene_.containsObjectId(scene_.selectedObjectId)) {
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
