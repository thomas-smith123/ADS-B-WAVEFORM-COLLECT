#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowTitle("ADS-B Tool -- By BH8CPP");
    w.show();
    return a.exec();
}
