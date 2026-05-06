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
#include "server_msg.pb.h"


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

    // =========================================================================
    // 👑 注册 Protobuf 自定义类型，允许其在多线程 (跨线程信号槽) 中进行深拷贝传递
    // =========================================================================
    qRegisterMetaType<omnibox::ListDirectoryResponse>("omnibox::ListDirectoryResponse");
    qRegisterMetaType<omnibox::CreateFolderResponse>("omnibox::CreateFolderResponse");
    qRegisterMetaType<omnibox::DeleteNodeResponse>("omnibox::DeleteNodeResponse");
    qRegisterMetaType<omnibox::RenameNodeResponse>("omnibox::RenameNodeResponse");
    qRegisterMetaType<omnibox::MoveNodeResponse>("omnibox::MoveNodeResponse");
}

int main(int argc, char *argv[])
{
    RegisterMetaTypes();
    QApplication app(argc, argv);
    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);

    // 关闭全局的系统菜单动画和淡入淡出特效
    QApplication::setEffectEnabled(Qt::UI_AnimateMenu, false);
    QApplication::setEffectEnabled(Qt::UI_FadeMenu, false);

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