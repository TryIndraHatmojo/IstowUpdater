#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include "localrepository.h"

int main(int argc, char *argv[])
{
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

    QGuiApplication app(argc, argv);

    // Set style non-native sebelum engine dibuat
    // Bisa diganti: "Material", "Universal", "Fusion", "Basic", "Imagine"
    QQuickStyle::setStyle("Material");

    // Inisialisasi database lokal sebelum QML engine berjalan
    LocalRepository::getInstance();

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("IstowUpdater", "MainPage");

    return app.exec();
}
