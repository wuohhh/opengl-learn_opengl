#include "glwidget.h"

#include <QDebug>
#include <QImage>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

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
      timer(new QTimer(this)),
      camera(glm::vec3(0.0f, 0.0f, 3.0f)),
      deltaTime(0.0f),
      lastFrame(0.0f),
      firstMouse(true),
      lastX(400.0f),
      lastY(300.0f),
      mousePressed(false)
{
    setWindowTitle("Qt OpenGL Camera Class");
    resize(800, 600);
    setMouseTracking(false);

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
    float currentFrame = elapsedTimer.elapsed() / 1000.0f;
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    if (pressedKeys.contains(Qt::Key_W))
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (pressedKeys.contains(Qt::Key_S))
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (pressedKeys.contains(Qt::Key_A))
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (pressedKeys.contains(Qt::Key_D))
        camera.ProcessKeyboard(RIGHT, deltaTime);

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

    int locProj = shaderProgram->uniformLocation("projection");
    int locView = shaderProgram->uniformLocation("view");
    int locModel = shaderProgram->uniformLocation("model");

    {
        float aspect = static_cast<float>(width()) / static_cast<float>(height());
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), aspect, 0.1f, 100.0f);
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
        checkGlError("projection");
    }

    {
        glm::mat4 view = camera.GetViewMatrix();
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
        pressedKeys.insert(event->key());
        QOpenGLWidget::keyPressEvent(event);
    }
}

void GLWidget::keyReleaseEvent(QKeyEvent *event)
{
    pressedKeys.remove(event->key());
    QOpenGLWidget::keyReleaseEvent(event);
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        mousePressed = true;
        firstMouse = true;
    }
    QOpenGLWidget::mousePressEvent(event);
}

void GLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        mousePressed = false;
    }
    QOpenGLWidget::mouseReleaseEvent(event);
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!mousePressed)
        return;

    float xpos = static_cast<float>(event->pos().x());
    float ypos = static_cast<float>(event->pos().y());

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
    QOpenGLWidget::mouseMoveEvent(event);
}

void GLWidget::wheelEvent(QWheelEvent *event)
{
    camera.ProcessMouseScroll(static_cast<float>(event->angleDelta().y()) / 120.0f);
    QOpenGLWidget::wheelEvent(event);
}
