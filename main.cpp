#include <QApplication>
#include <QDir>
#include <QLockFile>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName("RetroClock");
    QApplication::setApplicationName("RetroClock");
    QApplication::setApplicationVersion(APP_VERSION);
    QApplication::setQuitOnLastWindowClosed(false);   // 隐藏到托盘时不退出

    // 单实例：开机自启动后再手动打开不会出现两个时钟
    QLockFile lock(QDir::temp().absoluteFilePath("RetroClock.lock"));
    if (!lock.tryLock(100))
        return 0;

    MainWindow w;
    return app.exec();
}
