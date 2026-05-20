#include "glwidget.h"

#include <QDebug>
#include <QImage>
#include <QKeyEvent>
#include <QMatrix4x4>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GLWidget::GLWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      shaderProgram(nullptr),
      VBO(0),
      VAO(0),
      texture1(0),
      texture2(0),
      timer(new QTimer(this))
{
    setWindowTitle("Qt OpenGL Coordinate Systems Multiple");
    resize(800, 600);

    connect(timer, &QTimer::timeout, this, [this]() {
        update();
    });
    timer->start(16);
    elapsedTimer.start();
}

GLWidget::~GLWidget()
{
    makeCurrent();
    if (texture2) {
        glDeleteTextures(1, &texture2);
    }
    if (texture1) {
        glDeleteTextures(1, &texture1);
    }
    if (VBO) {
        glDeleteBuffers(1, &VBO);
    }
    if (VAO) {
        glDeleteVertexArrays(1, &VAO);
    }
    delete shaderProgram;
    doneCurrent();
}

void GLWidget::initializeGL()
{
    if (!initializeOpenGLFunctions()) {
        qWarning() << "ERROR: Failed to initialize OpenGL 3.3 Core functions.";
        qWarning() << "  The context likely does not support OpenGL 3.3 Core Profile.";
        return;
    }

    // 输出 OpenGL 上下文诊断信息
    const GLubyte *vendor   = glGetString(GL_VENDOR);
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *version  = glGetString(GL_VERSION);
    qDebug() << "OpenGL Vendor:  " << (vendor   ? (const char *)vendor   : "N/A");
    qDebug() << "OpenGL Renderer:" << (renderer ? (const char *)renderer : "N/A");
    qDebug() << "OpenGL Version: " << (version  ? (const char *)version  : "N/A");

    shaderProgram = new QOpenGLShaderProgram(this);
    if (!shaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/coordinate_systems.vs")) {
        qWarning() << "Vertex shader compilation failed:" << shaderProgram->log();
        return;
    }
    if (!shaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/coordinate_systems.fs")) {
        qWarning() << "Fragment shader compilation failed:" << shaderProgram->log();
        return;
    }
    if (!shaderProgram->link()) {
        qWarning() << "Shader program linking failed:" << shaderProgram->log();
        return;
    }

    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void *>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);

    const auto loadTexture = [this](unsigned int *textureId, const QString &resourcePath) {
        glGenTextures(1, textureId);
        glBindTexture(GL_TEXTURE_2D, *textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        QImage textureImage(resourcePath);
        if (textureImage.isNull()) {
            qWarning() << "Failed to load texture resource:" << resourcePath;
            textureImage = QImage(1, 1, QImage::Format_RGBA8888);
            textureImage.fill(Qt::magenta);
        }

        const QImage glImage = textureImage.mirrored(false, true).convertToFormat(QImage::Format_RGBA8888);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     glImage.width(),
                     glImage.height(),
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     glImage.constBits());
        glGenerateMipmap(GL_TEXTURE_2D);
    };

    loadTexture(&texture1, QStringLiteral(":/resource/container.jpg"));
    loadTexture(&texture2, QStringLiteral(":/resource/awesomeface.png"));

    // 刷新 OpenGL 错误状态
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        qWarning() << "OpenGL error during initialization:" << err;
    }

    shaderProgram->bind();
    shaderProgram->setUniformValue("texture1", 0);
    shaderProgram->setUniformValue("texture2", 1);
    shaderProgram->release();
}

void GLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void GLWidget::checkGlError(const char *location)
{
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        qWarning() << "[OpenGL Error @" << location << "]" << err;
    }
}

void GLWidget::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!shaderProgram || !shaderProgram->isLinked()) {
        return;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture2);

    shaderProgram->bind();
    checkGlError("bind");

    // 直接使用 glUniformMatrix4fv 传递 GLM 矩阵（避免 QMatrix4x4 转换）
    int locProj = shaderProgram->uniformLocation("projection");
    int locView = shaderProgram->uniformLocation("view");
    int locModel = shaderProgram->uniformLocation("model");
    qDebug() << "Uniform locations: proj=" << locProj << " view=" << locView << " model=" << locModel;

    {
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
        checkGlError("projection");
    }

    {
        glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
        checkGlError("view");
    }

    glm::vec3 cubePositions[] = {
        glm::vec3( 0.0f,  0.0f,  0.0f),
        glm::vec3( 2.0f,  5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3( 2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f,  3.0f, -7.5f),
        glm::vec3( 1.3f, -2.0f, -2.5f),
        glm::vec3( 1.5f,  2.0f, -2.5f),
        glm::vec3( 1.5f,  0.2f, -1.5f),
        glm::vec3(-1.3f,  1.0f, -1.5f)
    };

    glBindVertexArray(VAO);
    checkGlError("bind VAO");

    float time = elapsedTimer.elapsed() / 1000.0f;
    for (unsigned int i = 0; i < 10; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, cubePositions[i]);
        float angle = 20.0f * i + time * 50.0f;
        model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
        glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
        checkGlError("model");

        glDrawArrays(GL_TRIANGLES, 0, 36);
        checkGlError("draw");
    }
    glBindVertexArray(0);

    shaderProgram->release();
}

void GLWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
    } else {
        QOpenGLWidget::keyPressEvent(event);
    }
}
