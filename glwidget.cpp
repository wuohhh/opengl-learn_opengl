#include "glwidget.h"

#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GLWidget::GLWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      lightingShader(nullptr),
      lightCubeShader(nullptr),
      VBO(0),
      cubeVAO(0),
      lightCubeVAO(0),
      timer(new QTimer(this)),
      camera(glm::vec3(0.0f, 0.0f, 3.0f)),
      deltaTime(0.0f),
      lastFrame(0.0f),
      firstMouse(true),
      lastX(400.0f),
      lastY(300.0f),
      mousePressed(false),
      lightPos(1.2f, 1.0f, 2.0f)
{
    setWindowTitle("Qt OpenGL Colors");
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
    if (VBO) {
        glDeleteBuffers(1, &VBO);
    }
    if (cubeVAO) {
        glDeleteVertexArrays(1, &cubeVAO);
    }
    if (lightCubeVAO) {
        glDeleteVertexArrays(1, &lightCubeVAO);
    }
    delete lightingShader;
    delete lightCubeShader;
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

    lightingShader = new QOpenGLShaderProgram(this);
    if (!lightingShader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/colors.vs")) {
        qWarning() << "colors.vs compilation failed:" << lightingShader->log();
        return;
    }
    if (!lightingShader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/colors.fs")) {
        qWarning() << "colors.fs compilation failed:" << lightingShader->log();
        return;
    }
    if (!lightingShader->link()) {
        qWarning() << "lightingShader linking failed:" << lightingShader->log();
        return;
    }

    lightCubeShader = new QOpenGLShaderProgram(this);
    if (!lightCubeShader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/light_cube.vs")) {
        qWarning() << "light_cube.vs compilation failed:" << lightCubeShader->log();
        return;
    }
    if (!lightCubeShader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/light_cube.fs")) {
        qWarning() << "light_cube.fs compilation failed:" << lightCubeShader->log();
        return;
    }
    if (!lightCubeShader->link()) {
        qWarning() << "lightCubeShader linking failed:" << lightCubeShader->log();
        return;
    }

    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };

    glGenVertexArrays(1, &cubeVAO);
    glGenVertexArrays(1, &lightCubeVAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(cubeVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void *>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(lightCubeVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void *>(0));
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        qWarning() << "OpenGL error during initialization:" << err;
    }
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

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!lightingShader || !lightingShader->isLinked())
        return;
    if (!lightCubeShader || !lightCubeShader->isLinked())
        return;

    float aspect = static_cast<float>(width()) / static_cast<float>(height());
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), aspect, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();

    lightingShader->bind();
    lightingShader->setUniformValue("objectColor", 1.0f, 0.5f, 0.31f);
    lightingShader->setUniformValue("lightColor", 1.0f, 1.0f, 1.0f);
    lightingShader->setUniformValue("lightPos", lightPos.x, lightPos.y, lightPos.z);
    lightingShader->setUniformValue("viewPos", camera.Position.x, camera.Position.y, camera.Position.z);

    int locProj = lightingShader->uniformLocation("projection");
    int locView = lightingShader->uniformLocation("view");
    int locModel = lightingShader->uniformLocation("model");
    glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));

    glBindVertexArray(cubeVAO);
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 36);

    lightCubeShader->bind();
    locProj = lightCubeShader->uniformLocation("projection");
    locView = lightCubeShader->uniformLocation("view");
    locModel = lightCubeShader->uniformLocation("model");
    glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));

    glBindVertexArray(lightCubeVAO);
    model = glm::mat4(1.0f);
    model = glm::translate(model, lightPos);
    model = glm::scale(model, glm::vec3(0.2f));
    glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glBindVertexArray(0);
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
