#include <iostream>
#include <QApplication>

#include "curecraft_math.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    // Initialize Qt Application
    QApplication app(argc, argv);

    // Create and show the Patient Monitor GUI
    MainWindow window;
    window.show();

    std::cout << "CureCraft Patient Monitor started" << std::endl;

    return app.exec();
}