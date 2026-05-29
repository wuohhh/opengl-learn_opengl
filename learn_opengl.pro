QT += core gui widgets opengl
greaterThan(QT_MAJOR_VERSION, 5): QT += openglwidgets

CONFIG += c++11

INCLUDEPATH += $$PWD/glm

SOURCES += \
    glwidget.cpp \
    main.cpp

HEADERS += \
    glwidget.h \
    camera.h

RESOURCES += \
    resource.qrc
