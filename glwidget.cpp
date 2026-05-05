#include "glwidget.h"
#include <QKeyEvent>
#include <QDebug>

// 顶点着色器源码
static const char *vertexShaderSource =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\n";

// 片段着色器源码
static const char *fragmentShaderSource =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
    "}\n";

GLWidget::GLWidget(QWidget *parent)
    : QOpenGLWidget(parent), shaderProgram(nullptr), VBO(0), VAO(0)
{
    // 设置窗口标题和大小（可选）
    setWindowTitle("Qt OpenGL Triangle");
    resize(800, 600);
}

GLWidget::~GLWidget()
{
    // 在 OpenGL 上下文销毁前释放资源
    makeCurrent();
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    delete shaderProgram;
    doneCurrent();
}

void GLWidget::initializeGL()
{
    // 初始化 OpenGL 函数（通过 QOpenGLFunctions_3_3_Core）
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
        -0.5f, -0.5f, 0.0f, // 左
         0.5f, -0.5f, 0.0f, // 右
         0.0f,  0.5f, 0.0f  // 上
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
    glBindVertexArray(0);
}

void GLWidget::resizeGL(int w, int h)
{
    // 设置视口
    glViewport(0, 0, w, h);
}

void GLWidget::paintGL()
{
    // 清屏颜色
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // 绘制三角形
    shaderProgram->bind();
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    shaderProgram->release();
}

void GLWidget::keyPressEvent(QKeyEvent *event)
{
    // 按 ESC 键退出程序
    if (event->key() == Qt::Key_Escape) {
        close();  // 关闭窗口，程序结束
    } else {
        QOpenGLWidget::keyPressEvent(event);
    }
}
