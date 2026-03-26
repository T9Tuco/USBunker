#include "ui/main_window.h"
#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("USBunker");
    app.setApplicationVersion("1.0.0");

    bunker::ui::MainWindow window;
    window.show();

    return app.exec();
}
