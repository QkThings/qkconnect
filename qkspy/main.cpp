#include "mainwindow.h"
#include "qkgui.h"
#include <QApplication>
#include <QDate>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setApplicationName("QkSpy");
    QCoreApplication::setApplicationVersion(QDate::fromString(__DATE__,"MMM dd yyyy").toString("yyyyMMdd"));
    QkGUI::qt_fusionDark();

    MainWindow w;
    w.show();

    return a.exec();
}
