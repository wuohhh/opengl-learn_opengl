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
      lightingShader(nullptr),
      multipleLightsShader(nullptr),
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
      mousePressed(false)
{
    setWindowTitle("Qt OpenGL Multiple Lights");
    resize(800, 600);
    setMouseTracking(false);

    cubePositions[0] = glm::vec3(0.0f, 0.0f, 0.0f);
    cubePositions[1] = glm::vec3(2.0f, 5.0f, -15.0f);
    cubePositions[2] = glm::vec3(-1.5f, -2.2f, -2.5f);
    cubePositions[3] = glm::vec3(-3.8f, -2.0f, -12.3f);
    cubePositions[4] = glm::vec3(2.4f, -0.4f, -3.5f);
    cubePositions[5] = glm::vec3(-1.7f, 3.0f, -7.5f);
    cubePositions[6] = glm::vec3(1.3f, -2.0f, -2.5f);
    cubePositions[7] = glm::vec3(1.5f, 2.0f, -2.5f);
    cubePositions[8] = glm::vec3(1.5f, 0.2f, -1.5f);
    cubePositions[9] = glm::vec3(-1.3f, 1.0f, -1.5f);

    pointLightPositions[0] = glm::vec3(0.7f, 0.2f, 2.0f);
    pointLightPositions[1] = glm::vec3(2.3f, -3.3f, -4.0f);
    pointLightPositions[2] = glm::vec3(-4.0f, 2.0f, -12.0f);
    pointLightPositions[3] = glm::vec3(0.0f, 0.0f, -3.0f);

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
    if (diffuseMap) {
        glDeleteTextures(1, &diffuseMap);
    }
    if (specularMap) {
        glDeleteTextures(1, &specularMap);
    }
    delete lightingShader;
    delete multipleLightsShader;
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
    if (!lightingShader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/lighting_maps.vs")) {
        qWarning() << "lighting_maps.vs compilation failed:" << lightingShader->log();
        return;
    }
    if (!lightingShader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/lighting_maps.fs")) {
        qWarning() << "lighting_maps.fs compilation failed:" << lightingShader->log();
        return;
    }
    if (!lightingShader->link()) {
        qWarning() << "lightingShader linking failed:" << lightingShader->log();
        return;
    }

    multipleLightsShader = new QOpenGLShaderProgram(this);
    if (!multipleLightsShader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/lighting_maps.vs")) {
        qWarning() << "lighting_maps.vs compilation failed:" << multipleLightsShader->log();
        return;
    }
    if (!multipleLightsShader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/multiple_lights.fs")) {
        qWarning() << "multiple_lights.fs compilation failed:" << multipleLightsShader->log();
        return;
    }
    if (!multipleLightsShader->link()) {
        qWarning() << "multipleLightsShader linking failed:" << multipleLightsShader->log();
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
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };

    glGenVertexArrays(1, &cubeVAO);
    glGenVertexArrays(1, &lightCubeVAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(cubeVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(lightCubeVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(0));
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // load textures
    diffuseMap = loadTexture(":/resource/container2.png");
    specularMap = loadTexture(":/resource/container2_specular.png");

    // shader configuration
    lightingShader->bind();
    lightingShader->setUniformValue("material.diffuse", 0);
    lightingShader->setUniformValue("material.specular", 1);
    lightingShader->release();

    multipleLightsShader->bind();
    multipleLightsShader->setUniformValue("material.diffuse", 0);
    multipleLightsShader->setUniformValue("material.specular", 1);
    multipleLightsShader->release();

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

    if (!multipleLightsShader || !multipleLightsShader->isLinked())
        return;
    if (!lightCubeShader || !lightCubeShader->isLinked())
        return;

    float aspect = static_cast<float>(width()) / static_cast<float>(height());
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), aspect, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();

    // ===========================
    // Multiple Lights Shader Pass
    // ===========================
    multipleLightsShader->bind();
    multipleLightsShader->setUniformValue("viewPos", camera.Position.x, camera.Position.y, camera.Position.z);
    multipleLightsShader->setUniformValue("material.shininess", 32.0f);

    // directional light
    multipleLightsShader->setUniformValue("dirLight.direction", -0.2f, -1.0f, -0.3f);
    multipleLightsShader->setUniformValue("dirLight.ambient", 0.05f, 0.05f, 0.05f);
    multipleLightsShader->setUniformValue("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
    multipleLightsShader->setUniformValue("dirLight.specular", 0.5f, 0.5f, 0.5f);

    // point light 1
    multipleLightsShader->setUniformValue("pointLights[0].position", pointLightPositions[0].x, pointLightPositions[0].y, pointLightPositions[0].z);
    multipleLightsShader->setUniformValue("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
    multipleLightsShader->setUniformValue("pointLights[0].diffuse", 0.8f, 0.8f, 0.8f);
    multipleLightsShader->setUniformValue("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
    multipleLightsShader->setUniformValue("pointLights[0].constant", 1.0f);
    multipleLightsShader->setUniformValue("pointLights[0].linear", 0.09f);
    multipleLightsShader->setUniformValue("pointLights[0].quadratic", 0.032f);
    // point light 2
    multipleLightsShader->setUniformValue("pointLights[1].position", pointLightPositions[1].x, pointLightPositions[1].y, pointLightPositions[1].z);
    multipleLightsShader->setUniformValue("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
    multipleLightsShader->setUniformValue("pointLights[1].diffuse", 0.8f, 0.8f, 0.8f);
    multipleLightsShader->setUniformValue("pointLights[1].specular", 1.0f, 1.0f, 1.0f);
    multipleLightsShader->setUniformValue("pointLights[1].constant", 1.0f);
    multipleLightsShader->setUniformValue("pointLights[1].linear", 0.09f);
    multipleLightsShader->setUniformValue("pointLights[1].quadratic", 0.032f);
    // point light 3
    multipleLightsShader->setUniformValue("pointLights[2].position", pointLightPositions[2].x, pointLightPositions[2].y, pointLightPositions[2].z);
    multipleLightsShader->setUniformValue("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
    multipleLightsShader->setUniformValue("pointLights[2].diffuse", 0.8f, 0.8f, 0.8f);
    multipleLightsShader->setUniformValue("pointLights[2].specular", 1.0f, 1.0f, 1.0f);
    multipleLightsShader->setUniformValue("pointLights[2].constant", 1.0f);
    multipleLightsShader->setUniformValue("pointLights[2].linear", 0.09f);
    multipleLightsShader->setUniformValue("pointLights[2].quadratic", 0.032f);
    // point light 4
    multipleLightsShader->setUniformValue("pointLights[3].position", pointLightPositions[3].x, pointLightPositions[3].y, pointLightPositions[3].z);
    multipleLightsShader->setUniformValue("pointLights[3].ambient", 0.05f, 0.05f, 0.05f);
    multipleLightsShader->setUniformValue("pointLights[3].diffuse", 0.8f, 0.8f, 0.8f);
    multipleLightsShader->setUniformValue("pointLights[3].specular", 1.0f, 1.0f, 1.0f);
    multipleLightsShader->setUniformValue("pointLights[3].constant", 1.0f);
    multipleLightsShader->setUniformValue("pointLights[3].linear", 0.09f);
    multipleLightsShader->setUniformValue("pointLights[3].quadratic", 0.032f);

    // spot light
    multipleLightsShader->setUniformValue("spotLight.position", camera.Position.x, camera.Position.y, camera.Position.z);
    multipleLightsShader->setUniformValue("spotLight.direction", camera.Front.x, camera.Front.y, camera.Front.z);
    multipleLightsShader->setUniformValue("spotLight.ambient", 0.0f, 0.0f, 0.0f);
    multipleLightsShader->setUniformValue("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
    multipleLightsShader->setUniformValue("spotLight.specular", 1.0f, 1.0f, 1.0f);
    multipleLightsShader->setUniformValue("spotLight.constant", 1.0f);
    multipleLightsShader->setUniformValue("spotLight.linear", 0.09f);
    multipleLightsShader->setUniformValue("spotLight.quadratic", 0.032f);
    multipleLightsShader->setUniformValue("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
    multipleLightsShader->setUniformValue("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));

    int locProj = multipleLightsShader->uniformLocation("projection");
    int locView = multipleLightsShader->uniformLocation("view");
    int locModel = multipleLightsShader->uniformLocation("model");
    glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));

    // bind diffuse map
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseMap);
    // bind specular map
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, specularMap);

    // render 10 containers
    glBindVertexArray(cubeVAO);
    for (unsigned int i = 0; i < 10; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, cubePositions[i]);
        float angle = 20.0f * i;
        model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
        glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    // ===========================
    // Light Cube Shader Pass
    // ===========================
    lightCubeShader->bind();
    locProj = lightCubeShader->uniformLocation("projection");
    locView = lightCubeShader->uniformLocation("view");
    locModel = lightCubeShader->uniformLocation("model");
    glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));

    // draw 4 point light cubes
    glBindVertexArray(lightCubeVAO);
    for (unsigned int i = 0; i < 4; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, pointLightPositions[i]);
        model = glm::scale(model, glm::vec3(0.2f));
        glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

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

unsigned int GLWidget::loadTexture(QString path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    QImage img(path);
    if (img.isNull()) {
        qWarning() << "Texture failed to load at path:" << path;
        return 0;
    }
    QImage tex = img.mirrored().convertToFormat(QImage::Format_RGBA8888);

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.bits());
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return textureID;
}
