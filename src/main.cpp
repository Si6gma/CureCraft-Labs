#include <iostream>
#include <QApplication>

#include "math.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    // Initialize Qt Application
    QApplication app(argc, argv);

    // Create and show the Patient Monitor GUI
    MainWindow window;
    window.show();

    // Uncomment the line below for fullscreen mode (e.g., for Raspberry Pi)
    // window.showFullScreen();

    std::cout << "CureCraft Patient Monitor started" << std::endl;

    return app.exec();
}