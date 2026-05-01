#pragma once

#include <memory>

#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QSize>

#include "core/BloomSettings.h"

class BloomPass : protected QOpenGLFunctions
{
public:
    BloomPass() = default;

    bool initialize();
    bool render(GLuint sourceTexture,
                GLuint defaultFramebuffer,
                const QSize& size,
                const tinyray::BloomSettings& settings);
    void reset();

private:
    void ensureBuffers(const QSize& size);
    void drawFullScreenTriangle(QOpenGLShaderProgram& program);

    bool initialized_ = false;
    std::unique_ptr<QOpenGLShaderProgram> brightProgram_;
    std::unique_ptr<QOpenGLShaderProgram> blurProgram_;
    std::unique_ptr<QOpenGLShaderProgram> compositeProgram_;
    std::unique_ptr<QOpenGLFramebufferObject> brightBuffer_;
    std::unique_ptr<QOpenGLFramebufferObject> pingPongBuffers_[2];
};
