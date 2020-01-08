#include "mainwindow.h"

#include <QApplication>
#include <QPushButton>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    qDebug() << "App path: " << a.applicationDirPath();
    vpa::MainWindow w;
    w.show();
    return a.exec();
}
