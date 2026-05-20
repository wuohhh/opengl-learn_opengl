QT += core gui widgets opengl openglwidgets

CONFIG += c++11

INCLUDEPATH += $$PWD/glm

SOURCES += \
    glwidget.cpp \
    main.cpp

HEADERS += \
    glwidget.h

RESOURCES += \
    resource.qrc
