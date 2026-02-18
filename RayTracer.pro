QT += core gui widgets openglwidgets

CONFIG += c++17
TARGET = RayTracer
TEMPLATE = app

SOURCES += \
    main.cpp \
    src/MainWindow.cpp \
    src/Viewport.cpp \
    src/Scene.cpp \
    src/SceneObject.cpp \
    src/Material.cpp \
    src/Camera.cpp \
    src/ObjLoader.cpp \
    src/PathTracer.cpp \
    src/PropertiesPanel.cpp \
    src/RenderWindow.cpp \
    src/BVH.cpp

HEADERS += \
    src/MainWindow.h \
    src/Viewport.h \
    src/Scene.h \
    src/SceneObject.h \
    src/Material.h \
    src/Camera.h \
    src/ObjLoader.h \
    src/PathTracer.h \
    src/PropertiesPanel.h \
    src/RenderWindow.h \
    src/BVH.h \
    src/Light.h

RESOURCES += resources.qrc

win32: LIBS += -lopengl32
unix:  LIBS += -lGL
