#include "vlcdemo.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    VLCDemo w;
    w.show();
    return a.exec();
}
