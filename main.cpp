#include <QApplication>
#include <QSurfaceFormat>
#include "glwidget.h"

int main(int argc, char *argv[])
{
    // 设置 OpenGL 版本和核心模式（必须在 QApplication 构造之前调用）
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setOption(QSurfaceFormat::DeprecatedFunctions, false);
    QSurfaceFormat::setDefaultFormat(format);

    // 强制使用桌面 OpenGL（而非 ANGLE），确保 3.3 Core 上下文正确创建
    QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);

    QApplication app(argc, argv);

    GLWidget widget;
    widget.show();

    return app.exec();
}
