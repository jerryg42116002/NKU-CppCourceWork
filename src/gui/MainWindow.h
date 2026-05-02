#pragma once

#include <thread>

#include <QImage>
#include <QMainWindow>

#include "core/Renderer.h"
#include "core/RenderSettings.h"
#include "core/Scene.h"
#include "interaction/DragMode.h"

class QComboBox;
class QPushButton;
class QLabel;
class QProgressBar;
class QSlider;
class QSpinBox;

class RealTimeRenderWidget;
class ScenePanel;

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
    void handleRealTimeObjectSelected(int objectId);
    void handleRealTimeLightSelected(int lightIndex);
    void handleRealTimeObjectMoved(int objectId, double x, double y, double z);
    void handleRealTimeLightMoved(int lightIndex, double x, double y, double z);

    void createMenus();
    void createControlPanel();
    void createStatusBar();
    void setRenderControlsEnabled(bool enabled);
    void setTurntableControlsRunning(bool running);
    void applyRenderSettings(const tinyray::RenderSettings& settings);
    tinyray::RenderSettings currentRenderSettings() const;
    void syncCurrentViewportCameraToScene();

private slots:
    void handleRender();
    void handleStop();
    void handleSaveImage();
    void handleClearImage();
    void handleSaveScene();
    void handleLoadScene();
    void handleAbout();
    void handleSceneChanged(const tinyray::Scene& scene);
    void updateRenderProgress(int progress);
    void updateRenderPreview(const QImage& image,
                             int currentSample,
                             int totalSamples,
                             int progress,
                             double elapsedSeconds);
    void handleRenderFinished(const QImage& image, bool stopped);
    void handleStartTurntable();
    void handleStopTurntable();
    void handleTurntableSpeedChanged(int value);
    void handleTurntableDirectionChanged(int index);
    void handleTurntableTargetModeChanged(int index);
    void handleResetView();
    void handleFocusSelectedObject();

private:
    RealTimeRenderWidget* realTimeRenderWidget_ = nullptr;
    ScenePanel* scenePanel_ = nullptr;

    QSpinBox* widthSpinBox_ = nullptr;
    QSpinBox* heightSpinBox_ = nullptr;
    QComboBox* samplesComboBox_ = nullptr;
    QSpinBox* maxDepthSpinBox_ = nullptr;
    QSpinBox* threadsSpinBox_ = nullptr;
    QComboBox* dragModeComboBox_ = nullptr;
    QSlider* turntableSpeedSlider_ = nullptr;
    QLabel* turntableSpeedValueLabel_ = nullptr;
    QComboBox* turntableDirectionComboBox_ = nullptr;
    QComboBox* turntableTargetModeComboBox_ = nullptr;

    QPushButton* renderButton_ = nullptr;
    QPushButton* stopButton_ = nullptr;
    QPushButton* saveImageButton_ = nullptr;
    QPushButton* saveSceneButton_ = nullptr;
    QPushButton* loadSceneButton_ = nullptr;
    QPushButton* clearButton_ = nullptr;
    QPushButton* startTurntableButton_ = nullptr;
    QPushButton* stopTurntableButton_ = nullptr;
    QPushButton* resetViewButton_ = nullptr;
    QPushButton* focusSelectedButton_ = nullptr;

    QLabel* statusLabel_ = nullptr;
    QProgressBar* progressBar_ = nullptr;

    tinyray::Scene scene_;
    tinyray::Renderer renderer_;
    QImage highQualityImage_;
    std::thread renderThread_;
};
