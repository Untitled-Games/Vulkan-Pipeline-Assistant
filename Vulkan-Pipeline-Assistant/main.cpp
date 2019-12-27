/*
 * Author: Ralph Ridley
 * Date: 19/12/19
 */

#include "mainwindow.h"

#include <QApplication>
#include <QPushButton>

using namespace vpa;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qDebug() << "App path: " << a.applicationDirPath();
    MainWindow w;
    w.show();
    return a.exec();
}
