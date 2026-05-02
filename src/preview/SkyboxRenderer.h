#pragma once

#include <algorithm>

#include <QImage>
#include <QOpenGLFunctions>
#include <QString>
#include <QtGui/qopengl.h>

#include "core/Environment.h"

class SkyboxRenderer
{
public:
    ~SkyboxRenderer() = default;

    GLuint texture(QOpenGLFunctions* functions, const tinyray::Environment& environment)
    {
        if (!functions
            || environment.imagePath.isEmpty()
            || environment.mode == tinyray::EnvironmentMode::Gradient
            || environment.mode == tinyray::EnvironmentMode::SolidColor) {
            return 0;
        }

        if (textureId_ != 0 && loadedPath_ == environment.imagePath) {
            return textureId_;
        }

        QImage image(environment.imagePath);
        if (image.isNull()) {
            return 0;
        }

        image = image.convertToFormat(QImage::Format_RGBA8888).mirrored(false, true);
        if (textureId_ == 0) {
            functions->glGenTextures(1, &textureId_);
        }

        functions->glBindTexture(GL_TEXTURE_2D, textureId_);
        functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        functions->glTexImage2D(GL_TEXTURE_2D,
                                0,
                                GL_RGBA,
                                image.width(),
                                image.height(),
                                0,
                                GL_RGBA,
                                GL_UNSIGNED_BYTE,
                                image.constBits());
        functions->glBindTexture(GL_TEXTURE_2D, 0);
        loadedPath_ = environment.imagePath;
        return textureId_;
    }

private:
    GLuint textureId_ = 0;
    QString loadedPath_;
};
