#include "preview/BloomPass.h"

#include <algorithm>

#include <QtGui/qopengl.h>
#include <QVector2D>

namespace {

constexpr const char* kFullscreenVertexShader = R"(
    #version 120
    attribute vec2 aPosition;
    varying vec2 vUv;

    void main()
    {
        vUv = aPosition * 0.5 + vec2(0.5);
        gl_Position = vec4(aPosition, 0.0, 1.0);
    }
)";

constexpr const char* kBrightFragmentShader = R"(
    #version 120
    uniform sampler2D uScene;
    uniform float uThreshold;
    varying vec2 vUv;

    void main()
    {
        vec3 color = texture2D(uScene, vUv).rgb;
        float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
        float bright = smoothstep(uThreshold, uThreshold + 0.55, luminance);
        gl_FragColor = vec4(color * bright, 1.0);
    }
)";

constexpr const char* kBlurFragmentShader = R"(
    #version 120
    uniform sampler2D uImage;
    uniform vec2 uTexelSize;
    uniform bool uHorizontal;
    varying vec2 vUv;

    void main()
    {
        vec2 direction = uHorizontal ? vec2(uTexelSize.x, 0.0) : vec2(0.0, uTexelSize.y);
        vec3 color = texture2D(uImage, vUv).rgb * 0.227027;
        color += texture2D(uImage, vUv + direction * 1.384615).rgb * 0.316216;
        color += texture2D(uImage, vUv - direction * 1.384615).rgb * 0.316216;
        color += texture2D(uImage, vUv + direction * 3.230769).rgb * 0.070270;
        color += texture2D(uImage, vUv - direction * 3.230769).rgb * 0.070270;
        gl_FragColor = vec4(color, 1.0);
    }
)";

constexpr const char* kCompositeFragmentShader = R"(
    #version 120
    uniform sampler2D uScene;
    uniform sampler2D uBloom;
    uniform float uBloomStrength;
    uniform float uExposure;
    varying vec2 vUv;

    void main()
    {
        vec3 sceneColor = texture2D(uScene, vUv).rgb;
        vec3 bloomColor = texture2D(uBloom, vUv).rgb * uBloomStrength;
        vec3 color = sceneColor + bloomColor;
        color = vec3(1.0) - exp(-max(color, vec3(0.0)) * uExposure);
        color = pow(max(color, vec3(0.0)), vec3(1.0 / 2.2));
        gl_FragColor = vec4(color, 1.0);
    }
)";

std::unique_ptr<QOpenGLShaderProgram> makeProgram(const char* fragmentSource)
{
    auto program = std::make_unique<QOpenGLShaderProgram>();
    if (!program->addShaderFromSourceCode(QOpenGLShader::Vertex, kFullscreenVertexShader)
        || !program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentSource)
        || !program->link()) {
        return nullptr;
    }
    return program;
}

QOpenGLFramebufferObjectFormat hdrFormat()
{
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::NoAttachment);
    format.setTextureTarget(GL_TEXTURE_2D);
    format.setInternalTextureFormat(GL_RGBA16F);
    return format;
}

} // namespace

bool BloomPass::initialize()
{
    if (initialized_) {
        return true;
    }

    initializeOpenGLFunctions();
    brightProgram_ = makeProgram(kBrightFragmentShader);
    blurProgram_ = makeProgram(kBlurFragmentShader);
    compositeProgram_ = makeProgram(kCompositeFragmentShader);
    initialized_ = brightProgram_ && blurProgram_ && compositeProgram_;
    return initialized_;
}

bool BloomPass::render(GLuint sourceTexture,
                       GLuint defaultFramebuffer,
                       const QSize& size,
                       const tinyray::BloomSettings& settings)
{
    if (!initialize() || size.isEmpty()) {
        return false;
    }

    ensureBuffers(size);
    if (!brightBuffer_ || !pingPongBuffers_[0] || !pingPongBuffers_[1]) {
        return false;
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);

    brightBuffer_->bind();
    glViewport(0, 0, brightBuffer_->width(), brightBuffer_->height());
    glClear(GL_COLOR_BUFFER_BIT);
    brightProgram_->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sourceTexture);
    brightProgram_->setUniformValue("uScene", 0);
    brightProgram_->setUniformValue("uThreshold", static_cast<float>(settings.safeThreshold()));
    drawFullScreenTriangle(*brightProgram_);
    brightProgram_->release();
    brightBuffer_->release();

    GLuint inputTexture = brightBuffer_->texture();
    const int blurPasses = settings.safeBlurPassCount();
    for (int pass = 0; pass < blurPasses; ++pass) {
        const int targetIndex = pass % 2;
        QOpenGLFramebufferObject* target = pingPongBuffers_[targetIndex].get();
        target->bind();
        glViewport(0, 0, target->width(), target->height());
        glClear(GL_COLOR_BUFFER_BIT);

        blurProgram_->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTexture);
        blurProgram_->setUniformValue("uImage", 0);
        blurProgram_->setUniformValue("uTexelSize",
                                      QVector2D(1.0f / static_cast<float>(std::max(target->width(), 1)),
                                                1.0f / static_cast<float>(std::max(target->height(), 1))));
        blurProgram_->setUniformValue("uHorizontal", pass % 2 == 0 ? 1 : 0);
        drawFullScreenTriangle(*blurProgram_);
        blurProgram_->release();
        target->release();
        inputTexture = target->texture();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);
    glViewport(0, 0, size.width(), size.height());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    compositeProgram_->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sourceTexture);
    compositeProgram_->setUniformValue("uScene", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    compositeProgram_->setUniformValue("uBloom", 1);
    compositeProgram_->setUniformValue("uBloomStrength", static_cast<float>(settings.safeStrength()));
    compositeProgram_->setUniformValue("uExposure", static_cast<float>(settings.safeExposure()));
    drawFullScreenTriangle(*compositeProgram_);
    compositeProgram_->release();

    return true;
}

void BloomPass::reset()
{
    brightBuffer_.reset();
    pingPongBuffers_[0].reset();
    pingPongBuffers_[1].reset();
}

void BloomPass::ensureBuffers(const QSize& size)
{
    if (brightBuffer_
        && pingPongBuffers_[0]
        && pingPongBuffers_[1]
        && brightBuffer_->size() == size
        && pingPongBuffers_[0]->size() == size
        && pingPongBuffers_[1]->size() == size) {
        return;
    }

    const QOpenGLFramebufferObjectFormat format = hdrFormat();
    brightBuffer_ = std::make_unique<QOpenGLFramebufferObject>(size, format);
    pingPongBuffers_[0] = std::make_unique<QOpenGLFramebufferObject>(size, format);
    pingPongBuffers_[1] = std::make_unique<QOpenGLFramebufferObject>(size, format);
}

void BloomPass::drawFullScreenTriangle(QOpenGLShaderProgram& program)
{
    static const GLfloat vertices[] = {
        -1.0f, -1.0f,
         3.0f, -1.0f,
        -1.0f,  3.0f
    };

    const int positionLocation = program.attributeLocation("aPosition");
    if (positionLocation < 0) {
        return;
    }

    program.enableAttributeArray(positionLocation);
    program.setAttributeArray(positionLocation, GL_FLOAT, vertices, 2);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    program.disableAttributeArray(positionLocation);
}
