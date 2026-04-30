#pragma once

#include <thread>

#include <QImage>
#include <QMainWindow>

#include "core/Renderer.h"
#include "core/RenderSettings.h"
#include "core/Scene.h"

class QComboBox;
class QPushButton;
class QLabel;
class QProgressBar;
class QSpinBox;

class RenderWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

signals:
    void renderProgressChanged(int progress);
    void renderPreviewUpdated(const QImage& image,
                              int currentSample,
                              int totalSamples,
                              int progress,
                              double elapsedSeconds);
    void renderFinished(const QImage& image, bool stopped);

private:
    void createControlPanel();
    void createStatusBar();
    void setRenderControlsEnabled(bool enabled);
    tinyray::RenderSettings currentRenderSettings() const;

private slots:
    void handleRender();
    void handleStop();
    void handleSaveImage();
    void handleClearImage();
    void updateRenderProgress(int progress);
    void updateRenderPreview(const QImage& image,
                             int currentSample,
                             int totalSamples,
                             int progress,
                             double elapsedSeconds);
    void handleRenderFinished(const QImage& image, bool stopped);

private:
    RenderWidget* renderWidget_ = nullptr;

    QSpinBox* widthSpinBox_ = nullptr;
    QSpinBox* heightSpinBox_ = nullptr;
    QComboBox* samplesComboBox_ = nullptr;
    QSpinBox* maxDepthSpinBox_ = nullptr;
    QSpinBox* threadsSpinBox_ = nullptr;

    QPushButton* renderButton_ = nullptr;
    QPushButton* stopButton_ = nullptr;
    QPushButton* saveImageButton_ = nullptr;
    QPushButton* clearButton_ = nullptr;

    QLabel* statusLabel_ = nullptr;
    QProgressBar* progressBar_ = nullptr;

    tinyray::Scene scene_;
    tinyray::Renderer renderer_;
    std::thread renderThread_;
};
