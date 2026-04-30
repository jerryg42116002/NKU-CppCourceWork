#include "gui/MainWindow.h"

#include <algorithm>

#include <QApplication>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

#include "gui/RenderWidget.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("TinyRay Studio"));
    resize(1180, 720);
    scene_ = tinyray::Scene::createDefaultScene();

    renderWidget_ = new RenderWidget(this);
    setCentralWidget(renderWidget_);

    createControlPanel();
    createStatusBar();
}

void MainWindow::createControlPanel()
{
    auto* dock = new QDockWidget(QStringLiteral("Render Settings"), this);
    dock->setAllowedAreas(Qt::RightDockWidgetArea);
    dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    auto* panel = new QWidget(dock);
    auto* panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(12, 12, 12, 12);
    panelLayout->setSpacing(12);

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

    samplesSpinBox_ = new QSpinBox(settingsGroup);
    samplesSpinBox_->setRange(1, 1024);
    samplesSpinBox_->setValue(defaults.samplesPerPixel);

    maxDepthSpinBox_ = new QSpinBox(settingsGroup);
    maxDepthSpinBox_->setRange(1, 64);
    maxDepthSpinBox_->setValue(defaults.maxDepth);

    threadsSpinBox_ = new QSpinBox(settingsGroup);
    threadsSpinBox_->setRange(1, 128);
    threadsSpinBox_->setValue(std::clamp(defaults.threadCount, 1, 128));

    settingsLayout->addRow(QStringLiteral("Width"), widthSpinBox_);
    settingsLayout->addRow(QStringLiteral("Height"), heightSpinBox_);
    settingsLayout->addRow(QStringLiteral("Samples"), samplesSpinBox_);
    settingsLayout->addRow(QStringLiteral("Max Depth"), maxDepthSpinBox_);
    settingsLayout->addRow(QStringLiteral("Threads"), threadsSpinBox_);

    auto* outputGroup = new QGroupBox(QStringLiteral("Output"), panel);
    auto* outputLayout = new QVBoxLayout(outputGroup);
    outputLayout->setSpacing(8);

    renderButton_ = new QPushButton(QStringLiteral("Render"), outputGroup);
    stopButton_ = new QPushButton(QStringLiteral("Stop"), outputGroup);
    saveImageButton_ = new QPushButton(QStringLiteral("Save Image"), outputGroup);
    clearButton_ = new QPushButton(QStringLiteral("Clear"), outputGroup);

    outputLayout->addWidget(renderButton_);
    outputLayout->addWidget(stopButton_);
    outputLayout->addWidget(saveImageButton_);
    outputLayout->addWidget(clearButton_);

    panelLayout->addWidget(settingsGroup);
    panelLayout->addWidget(outputGroup);
    panelLayout->addStretch(1);

    dock->setWidget(panel);
    dock->setMinimumWidth(280);
    addDockWidget(Qt::RightDockWidgetArea, dock);

    connect(renderButton_, &QPushButton::clicked, this, &MainWindow::handleRender);
    connect(stopButton_, &QPushButton::clicked, this, &MainWindow::handleStop);
    connect(saveImageButton_, &QPushButton::clicked, this, &MainWindow::handleSaveImage);
    connect(clearButton_, &QPushButton::clicked, this, &MainWindow::handleClearImage);
}

void MainWindow::createStatusBar()
{
    statusLabel_ = new QLabel(QStringLiteral("Ready"), this);
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
    saveImageButton_->setEnabled(enabled);
    clearButton_->setEnabled(enabled);
}

tinyray::RenderSettings MainWindow::currentRenderSettings() const
{
    tinyray::RenderSettings settings;
    settings.width = widthSpinBox_->value();
    settings.height = heightSpinBox_->value();
    settings.samplesPerPixel = samplesSpinBox_->value();
    settings.maxDepth = maxDepthSpinBox_->value();
    settings.threadCount = threadsSpinBox_->value();
    return settings;
}

void MainWindow::handleRender()
{
    const tinyray::RenderSettings settings = currentRenderSettings();

    setRenderControlsEnabled(false);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    statusLabel_->setText(QStringLiteral("Rendering %1 x %2").arg(settings.width).arg(settings.height));
    progressBar_->setValue(0);

    renderer_.resetStopFlag();
    const QImage image = renderer_.render(scene_, settings, [this](int progress) {
        progressBar_->setValue(progress);
        qApp->processEvents();
    });

    renderWidget_->setImage(image);

    QApplication::restoreOverrideCursor();
    setRenderControlsEnabled(true);

    if (renderer_.stopRequested()) {
        statusLabel_->setText(QStringLiteral("Render stopped"));
    } else {
        progressBar_->setValue(100);
        statusLabel_->setText(QStringLiteral("Render complete"));
    }
}

void MainWindow::handleStop()
{
    renderer_.requestStop();
    statusLabel_->setText(QStringLiteral("Stop requested"));
}

void MainWindow::handleSaveImage()
{
    if (renderWidget_->image().isNull()) {
        QMessageBox::information(this, QStringLiteral("Save Image"),
                                 QStringLiteral("There is no rendered image to save."));
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

    if (!renderWidget_->image().save(fileName, "PNG")) {
        QMessageBox::warning(this, QStringLiteral("Save Image"),
                             QStringLiteral("Failed to save the PNG image."));
        return;
    }

    statusLabel_->setText(QStringLiteral("Image saved"));
}

void MainWindow::handleClearImage()
{
    renderWidget_->clearImage();
    progressBar_->setValue(0);
    statusLabel_->setText(QStringLiteral("Preview cleared"));
}
