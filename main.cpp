#include "mainwindow.h"

/**
* Author: Saugata Kundu
* Application Description: To Encode & Decode Images
*/

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("ImgEncoder");
    MainWindow w;
    w.show();
    return a.exec();
}
