#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QLibraryInfo>
#include <QDir>

#include "MainWindow.hpp"

#include <LAppAllocator.hpp>
#include <LAppPal.hpp>
#include <CubismFramework.hpp>

#ifdef _WIN32
#include <Windows.h>
#endif

int main(int argc, char *argv[])
{

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    LAppAllocator allocator;
    Csm::CubismFramework::Option option;

    option.LogFunction = LAppPal::PrintLn;
    option.LoggingLevel = Csm::CubismFramework::Option::LogLevel_Verbose;
    option.LoadFileFunction = LAppPal::LoadFileAsBytes;
    option.ReleaseBytesFunction = LAppPal::ReleaseBytes;

    Csm::CubismFramework::StartUp(&allocator, &option);
    Csm::CubismFramework::Initialize();

    QApplication app(argc, argv);

    LAppPal::InitShaderDir(QDir::current().absolutePath().toStdString());

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

    QObject::connect(&app, &QApplication::aboutToQuit, []()
                     { Csm::CubismFramework::Dispose(); });

    MainWindow w;
    w.show();
    return app.exec();
}
