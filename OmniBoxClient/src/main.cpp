#include "mainWindow.h"
#include <QtWidgets/QApplication>
#include <QFile>
#include <QDebug>
#include <QDir>
#include <QThread>
#include <QNetworkProxy>
#include "Global.h"
#include "Macro.h"
#include "GameItem.h"
#include "LogRecord.h"
#include "Enum.h"
#include "ThreadPool.h"


void LoadStyle(QApplication* app)
{
    QFile file("./StyleSheet/stylesheet.qss");
    if (file.open(QFile::ReadOnly))
    {
        QString style = QString::fromUtf8(file.readAll());
        app->setStyleSheet(style);
        file.close();
        qDebug() << "Load Style Success";
    }
    else
    {
        qDebug() << "Load Style Failed";
    }
}

void RegisterMetaTypes()
{
    qRegisterMetaType<ReqID_TCP>("ReqID_TCP");
}

int main(int argc, char *argv[])
{
    RegisterMetaTypes();
    QApplication app(argc, argv);
    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);

    // 启动线程池
    ThreadPool::Instance()->Start(4);

    mainWindow window;

    app.setWindowIcon(QIcon(":/MiNi/Images/MiNiWorld/Login.jpg"));

    // 加载样式表
    LoadStyle(&app);

    app.exec();
    
    ThreadPool::Instance()->Stop();

    return 0;
}