#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QElapsedTimer>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>
#include <QSet>
#include <QTimer>

#include "camera.h"

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    explicit GLWidget(QWidget *parent = nullptr);
    ~GLWidget();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void checkGlError(const char *location);

private:
    QOpenGLShaderProgram *lightingShader;
    QOpenGLShaderProgram *lightCubeShader;
    unsigned int VBO;
    unsigned int cubeVAO;
    unsigned int lightCubeVAO;
    unsigned int diffuseMap;
    unsigned int specularMap;
    QTimer *timer;
    QElapsedTimer elapsedTimer;

    Camera camera;
    float deltaTime;
    float lastFrame;
    bool firstMouse;
    float lastX;
    float lastY;
    bool mousePressed;
    QSet<int> pressedKeys;

    glm::vec3 lightPos;

    unsigned int loadTexture(QString path);
};

#endif // GLWIDGET_H
