#include <QApplication>
#include <QSurfaceFormat>
#include "glwidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 可选：设置 OpenGL 版本和核心模式（与原始 GLFW 配置一致）
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
#ifdef __APPLE__
    format.setOption(QSurfaceFormat::DeprecatedFunctions, false);
#endif
    QSurfaceFormat::setDefaultFormat(format);

    GLWidget widget;
    widget.show();

    return app.exec();
}
