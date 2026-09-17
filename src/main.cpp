/**
 * @file main.cpp
 * @brief Application entry point configuring surface formats and launching event loop.
 */

#include <QApplication>
#include <QSurfaceFormat>
#include <QFileInfo>
#include "ui/MainWindow.h"
#include "core/I18n.h"

int main(int argc, char* argv[]) {
    // Configure hardware surface format: OpenGL 3.3 Core Profile, 4x MSAA, 24-bit depth, VSync
    QSurfaceFormat fmt;
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setSamples(4);
    fmt.setDepthBufferSize(24);
    fmt.setAlphaBufferSize(0);
    fmt.setSwapInterval(1);
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);

    // Initialize Localization
    StudioViewer::I18n::init();

    StudioViewer::MainWindow viewer;
    viewer.show();

    // Check command line arguments ("Open with..." passes file path in argv)
    const QStringList args = QCoreApplication::arguments();
    for (int i = 1; i < args.size(); ++i) {
        QFileInfo fi(args.at(i));
        if (fi.exists() && fi.isFile()) {
            QString ext = fi.suffix().toLower();
            if (ext == "obj" || ext == "glb" || ext == "gltf") {
                viewer.loadModel(fi.absoluteFilePath());
                break;
            }
        }
    }

    return app.exec();
}
