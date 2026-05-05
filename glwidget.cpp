#include "glwidget.h"
#include <QKeyEvent>
#include <QDebug>
#include <cmath>
#include <QDateTime>

// 顶点着色器源码
static const char *vertexShaderSource =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos, 1.0);\n"
    "}\n";

// 片段着色器源码（使用 uniform 变量）
static const char *fragmentShaderSource =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec4 ourColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = ourColor;\n"
    "}\n";

GLWidget::GLWidget(QWidget *parent)
    : QOpenGLWidget(parent), shaderProgram(nullptr), VBO(0), VAO(0), greenValue(0.5f)
{
    setWindowTitle("Qt OpenGL Dynamic Color Triangle");
    resize(800, 600);

    // 创建定时器，每 16ms（约 60fps）更新颜色
    colorTimer = new QTimer(this);
    connect(colorTimer, &QTimer::timeout, this, &GLWidget::updateColor);
    colorTimer->start(16);  // 约 60fps
}

GLWidget::~GLWidget()
{
    makeCurrent();
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    delete shaderProgram;
    doneCurrent();
}

void GLWidget::initializeGL()
{
    // 初始化 OpenGL 函数
    initializeOpenGLFunctions();

    // 编译并链接着色器程序
    shaderProgram = new QOpenGLShaderProgram(this);
    shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    if (!shaderProgram->link()) {
        qDebug() << "Shader program linking failed:" << shaderProgram->log();
        return;
    }

    // 设置顶点数据
    float vertices[] = {
         0.5f, -0.5f, 0.0f,  // 右下
        -0.5f, -0.5f, 0.0f,  // 左下
         0.0f,  0.5f, 0.0f   // 顶部
    };

    // 创建 VAO 和 VBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);  // 解绑 VAO
}

void GLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void GLWidget::paintGL()
{
    // 清屏
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // 激活着色器程序
    shaderProgram->bind();

    // 获取 uniform 位置并设置颜色
    int vertexColorLocation = shaderProgram->uniformLocation("ourColor");
    shaderProgram->setUniformValue(vertexColorLocation, 0.0f, greenValue, 0.0f, 1.0f);

    // 绑定 VAO 并绘制三角形
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    shaderProgram->release();
}

void GLWidget::updateColor()
{
    // 使用正弦波动态改变绿色分量（产生呼吸灯效果）
    static double startTime = QDateTime::currentMSecsSinceEpoch() / 1000.0;
    double currentTime = QDateTime::currentMSecsSinceEpoch() / 1000.0;
    greenValue = static_cast<float>(sin(currentTime) / 2.0 + 0.5);

    // 触发重绘
    update();  // 调用 update() 会触发 paintGL()
}

void GLWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
    } else {
        QOpenGLWidget::keyPressEvent(event);
    }
}
