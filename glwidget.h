#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QTimer>

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    GLWidget(QWidget *parent = nullptr);
    ~GLWidget();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void updateColor();  // 定时更新颜色

private:
    QOpenGLShaderProgram *shaderProgram;
    QTimer *colorTimer;  // 用于动态改变颜色
    unsigned int VBO, VAO;
    float greenValue;    // 当前的绿色分量值
};

#endif // GLWIDGET_H
