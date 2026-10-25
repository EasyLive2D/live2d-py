#include <GL/glew.h>
#include <QApplication>
#include <QDir>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>
#include <GraphicsBackend.hpp>

#include "MainWindow.hpp"

#include <Init.hpp>

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef DEBUG_ENABLE_CALLSTACK
#include <Debug.hpp>
#endif

int main(int argc, char* argv[])
{
#ifdef DEBUG_ENABLE_CALLSTACK
    Live2D::Common::Debug::InstallCrashHandler();
#endif

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    // LogLevel_Verbose = 0
    Live2D::Init(QDir::current().absolutePath().toStdString().c_str(), 0);

    Live2D::Common::Backend::SetBackend(Live2D::Common::Backend::BACKEND_OPENGL);

    QApplication app(argc, argv);

    // Set up translations
    QTranslator translator;
    QTranslator qtTranslator;

    // Get system locale
    QString locale = QLocale::system().name();

    // Load Qt's built-in translations
    qtTranslator.load("qt_" + locale, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);

    // Load application translations
    QString translationPath = QApplication::applicationDirPath();
    if (translator.load(":/i18n/moe_" + locale, translationPath)) {
        app.installTranslator(&translator);
    } else {
        // Fallback to English if system locale translation not found
        if (translator.load(":/i18n/moe_en", translationPath)) {
            app.installTranslator(&translator);
        }
    }

    QObject::connect(&app, &QApplication::aboutToQuit, []() { Live2D::Dispose(); });

    MainWindow w;
    w.show();
    return app.exec();
}
