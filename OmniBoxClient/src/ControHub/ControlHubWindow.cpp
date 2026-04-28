#include "ControlHubWindow.h"
#include "TitleBar.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedLayout>
#include <QDebug>
#include <QPainter>
#include <QListWidget>
#include <QButtonGroup>
#include "GameItem.h"
#include "GameWidget.h"
#include "SettingWidget.h"
#include "UserMgr.h"


// 假设你有一个定义侧边栏宽度的宏，如果没有可以在这里写死比如 200
#ifndef HZ_LIST_WIDTH
#define HZ_LIST_WIDTH 200
#endif

ControlHubWindow::ControlHubWindow(QWidget* parent) : QWidget(parent)
{
    resize(1100, 800);

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent); // 我来负责整窗绘制，更高效

    // 1.根布局(垂直布局)
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // 2.顶部栏
    m_title = new TitleBar(this);
    m_title->SetMode(TitleMode::Hub);
    m_title->SetUserName(UserMgr::Instance()->GetUserName());
    root->addWidget(m_title);

    // 3.主窗口(水平布局)
    auto* center = new QWidget(this);
    QHBoxLayout* hbox = new QHBoxLayout(center);
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(0);

    center->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // ==========================================================
    // 4. 左侧边栏 (剥离了 MiniWorld 概念，替换为中控模块名)
    // ==========================================================
    // 配置名称与图标的映射 (图标路径保留你原来的)
    m_gameItems.insert(u8"宣传视频", ":/MiNi/Images/MiNiWorld/GameCenter.png");
    m_gameItems.insert(u8"对战控制", ":/MiNi/Images/MiNiWorld/Icon.png");
    m_gameItems.insert(u8"系统设置", ":/MiNi/Images/MiNiWorld/Setting.png");

    m_leftList_c = new QListWidget(center);
    m_leftList_c->setObjectName("LeftSidebar");                                         // 为左侧栏赋予唯一 ID
    QVBoxLayout* listLayout = new QVBoxLayout(m_leftList_c);
    listLayout->setContentsMargins(0, 0, 0, 0);
    listLayout->setSpacing(0);
    m_leftList_c->setFixedWidth(HZ_LIST_WIDTH);

    // 按钮组
    m_leftList_btns = new QButtonGroup(m_leftList_c);
    m_leftList_btns->setExclusive(true);                                                // 互斥选中

    // 添加核心控制模块
    AddGameItem(u8"宣传视频");
    AddGameItem(u8"对战控制");

    // 添加弹簧把上面两个标签顶上去
    listLayout->addStretch(1);

    // 添加底部的设置按钮
    AddGameItem(u8"系统设置");

    // 默认选中第一个“宣传视频”
    m_leftList_btns->button(0)->setChecked(true);

    hbox->addWidget(m_leftList_c, 0);

    // ==========================================================
    // 5. 右边堆栈窗口 (页面管理器)
    // ==========================================================
    m_stack = new QStackedLayout();
    m_stack->setContentsMargins(0, 0, 0, 0);
    m_stack->setSpacing(0);

    // 绑定侧边栏切换信号
    connect(m_leftList_btns, QOverload<int>::of(&QButtonGroup::buttonClicked),
        m_stack, &QStackedLayout::setCurrentIndex);

    hbox->addLayout(m_stack, 1);
    root->addWidget(center, 1);
}

void ControlHubWindow::AddGameItem(QString name)
{
    if (m_leftList_btns->buttons().count() < 3)
    {
        // 这里的 GameItem 类大概率是你自己封装的侧边栏按钮类，不用改它的底层
        GameItem* item = new GameItem(name, m_gameItems.value(name), m_leftList_c);
        item->setCheckable(true);
        item->setChecked(true);
        m_leftList_btns->addButton(item, m_leftList_btns->buttons().count());
        m_leftList_c->layout()->addWidget(item);
    }
}