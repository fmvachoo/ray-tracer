#include <QApplication>
#include <QSurfaceFormat>
#include "src/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QSurfaceFormat fmt;
    fmt.setVersion(4, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setDepthBufferSize(24);
    fmt.setSamples(4);
    QSurfaceFormat::setDefaultFormat(fmt);

    MainWindow w;
    w.resize(1280, 720);
    w.setWindowTitle("RayTracer");
    w.show();

    return app.exec();
}
