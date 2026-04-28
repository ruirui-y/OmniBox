#include "mainWindow.h"
#include <QStackedWidget>
#include "LoginWidget.h"
#include "ControlHubWindow.h"
#include "Titlebar.h"
#include "JsonTool.h"
#include "Global.h"
#include "ThreadPool.h"
#include "Macro.h"
#include "UserMgr.h"

// 定义固定窗口大小常量
constexpr int HUB_WIDTH = 1100;
constexpr int HUB_HEIGHT = 800;

mainWindow::mainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    // 隐藏系统标题栏，开启无边框模式
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);

    m_pages = new QStackedWidget(this);
    setCentralWidget(m_pages);

    BindSlots();

    _LoginWidget = new LoginWidget(this);
    // 绑定登录页标题栏的信号
    TitleBar* loginTitle = _LoginWidget->GetTitle();
    if (loginTitle) {
        connect(loginTitle, &TitleBar::minimizeRequested, this, &QWidget::showMinimized);
        connect(loginTitle, &TitleBar::closeRequested, this, &mainWindow::close);
    }
    
    m_pages->addWidget(_LoginWidget);
    m_pages->setCurrentWidget(_LoginWidget);

    setFixedSize(_LoginWidget->size());

    setWindowTitle("Demand Station");
}

mainWindow::~mainWindow()
{
}

void mainWindow::BindSlots()
{
    connect(ThreadPool::Instance()->GetTCPMgr(), &TCPMgr::SigLoginSuccess, this, &mainWindow::SlotSwitchToControlHubWidget);
    connect(ThreadPool::Instance()->GetTCPMgr(), &TCPMgr::SigConnectClose, this, &mainWindow::SlotSwitchToLoginWidget);
}

void mainWindow::SetFrameless(bool on)
{
    // 只切换是否无边框，其它保持默认
    setWindowFlag(Qt::FramelessWindowHint, on);

    // 登录页通常需要系统菜单/最小化/关闭按钮，游戏页则不需要
    setWindowFlag(Qt::WindowSystemMenuHint, !on);
    setWindowFlag(Qt::WindowMinMaxButtonsHint, !on);
    setWindowFlag(Qt::WindowCloseButtonHint, true);

    // 重新显示以应用新 flags（很重要）
    if (isVisible()) showNormal(); else show();
}

void mainWindow::SlotSwitchToLoginWidget()
{
    if (m_pages->currentWidget() == _LoginWidget)
    {
        emit _LoginWidget->sig_connect_tcp();
        return;
    }

    // 清除配置文件
    JsonTool::Instance()->clearJsonFile(LoginConfigPath);

    // 切换界面
    if (_ControlHub != nullptr)
    {
        m_pages->removeWidget(_ControlHub);
        _ControlHub->deleteLater();
        _ControlHub = nullptr;
    }

    // 清空用户数据
    UserMgr::Instance()->ClearUser();

    setFixedSize(_LoginWidget->size());

    m_pages->setCurrentWidget(_LoginWidget);
    emit _LoginWidget->sig_connect_tcp();

    _LoginWidget->EnableBtn(true);

    qDebug() << "[mainWindow] Switch to LoginWidget";
}

void mainWindow::SlotSwitchToControlHubWidget()
{
    if (_ControlHub == nullptr)
    {
        _ControlHub = new ControlHubWindow(this);
        m_pages->addWidget(_ControlHub);

        // 获取标题栏
        TitleBar* title = _ControlHub->GetTitle();
        connect(title, &TitleBar::minimizeRequested, this, &QWidget::showMinimized);                                // 最小化
        connect(title, &TitleBar::closeRequested, this, &mainWindow::CloseWidget);                                  // 关闭
    }

    // 无边框 + 自绘背景
    SetFrameless(true);
    setAutoFillBackground(false);

    m_pages->setCurrentWidget(_ControlHub);

    setFixedSize(QSize(HUB_WIDTH, HUB_HEIGHT));
    show();
    qDebug() << "[mainWindow] Switch to ControlHubWindow";
}

void mainWindow::CloseWidget()
{
    ThreadPool::Instance()->GetTCPMgr()->SetInitiateDisCon(true);                                                                // 主动断开连接
    close();
}