#include "window.hpp"

#include <QApplication>

#include <cstdlib>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    memsim::visual::Window window;
    const auto args = app.arguments();
    if (args.size() == 3 && args[1] == QStringLiteral("--smoke-test")) {
        return window.runSmokeTest(args[2]) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    if (args.size() == 3 && args[1] == QStringLiteral("--playback-test")) {
        return window.runPlaybackTest(args[2]) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    window.show();
    return app.exec();
}
