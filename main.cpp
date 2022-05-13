#include "mainwindow.h"
#include "QLabel"
#include "QImage"
#include "cmath"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
