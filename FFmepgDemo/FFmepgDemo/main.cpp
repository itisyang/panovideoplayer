#include "ffmepgdemo.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    FFmepgDemo w;
    w.show();
    return a.exec();
}
