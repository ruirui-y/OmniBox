#include "GameWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFile>
#include <QApplication>
#include <QMessageBox>
#include <QDebug>

GameWidget::GameWidget(QWidget* parent)
    : QWidget(parent)
{
    this->setObjectName("GameWidget");                                  // 绑定主窗口QSS
    this->resize(1000, 650);                                            // 设置舒适的默认尺寸

    m_gameProcess = new QProcess(this);                                 // 实例化进程管理器

    BuildUI();                                                          // 搭建界面

    // 绑定游戏进程结束信号 (简化版，省去exitCode等参数)
    connect(m_gameProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        this, &GameWidget::onGameProcessFinished);                     // 游戏结束触发重新加密
}

GameWidget::~GameWidget()
{
    if (m_gameProcess->state() == QProcess::Running) {                  // 如果游戏还在跑
        m_gameProcess->terminate();                                     // 尝试优雅关闭
        m_gameProcess->waitForFinished(2000);                           // 等待2秒
    }
}

void GameWidget::BuildUI()
{
    QVBoxLayout* rootLayout = new QVBoxLayout(this);                    // 根布局
    rootLayout->setContentsMargins(30, 30, 30, 30);                     // 四周留白呼吸感
    rootLayout->setSpacing(20);                                         // 元素纵向间距

    // ================== 1. 顶部栏 ==================
    QHBoxLayout* topLayout = new QHBoxLayout();                         // 顶部横向布局
    QLabel* logoLabel = new QLabel(u8"GAME LIST");                      // 标题文本
    logoLabel->setObjectName("logoLabel");                              // 绑定QSS
    topLayout->addWidget(logoLabel);                                    // 加入标题
    topLayout->addStretch();                                            // 中间撑开

    // ================== 2. 核心列表 ==================
    m_listWidget = new QListWidget(this);                               // 创建列表控件
    m_listWidget->setViewMode(QListView::IconMode);                     // 👑开启图标(流式)模式
    m_listWidget->setResizeMode(QListView::Adjust);                     // 👑窗口拉伸时自动重排
    m_listWidget->setSpacing(15);                                       // 卡片之间的网格间距
    m_listWidget->setMovement(QListView::Static);                       // 禁止拖拽图标

    // 批量塞入测试数据
    AddGame(u8"赛博朋克城市", u8"最后游玩时间: 昨天");
    AddGame(u8"奇幻森林", u8"最后游玩时间: 3天前");
    AddGame(u8"太空战舰", u8"最后游玩时间: 1小时前");
    AddGame(u8"元素地牢", u8"未游玩");
    AddGame(u8"极速狂飙", u8"最后游玩时间: 刚刚");

    // ================== 3. 右下角全局按钮 ==================
    QHBoxLayout* bottomLayout = new QHBoxLayout();                      // 底部按钮层
    bottomLayout->addStretch();                                         // 把按钮全挤到右边去

    m_btnPlay = new QPushButton(u8"▶");                                 // 播放按钮 (图里的绿钮)
    m_btnPlay->setObjectName("btnPlay");                                // 绑定QSS
    m_btnPlay->setFixedSize(70, 70);                                    // 固定圆形尺寸
    m_btnPlay->setEnabled(false);                                       // 初始没选中游戏时禁用

    m_btnStop = new QPushButton(u8"⏹");                                // 停止按钮 (图里的红钮)
    m_btnStop->setObjectName("btnStop");                                // 绑定QSS
    m_btnStop->setFixedSize(70, 70);                                    // 固定圆形尺寸
    m_btnStop->setEnabled(false);                                       // 初始禁用

    bottomLayout->addWidget(m_btnPlay);                                 // 放入绿钮
    bottomLayout->addSpacing(5);                                        // 俩按钮拉开点距离
    bottomLayout->addWidget(m_btnStop);                                 // 放入红钮

    // ================== 4. 组装并绑定 ==================
    rootLayout->addLayout(topLayout);                                   // 放入顶栏
    rootLayout->addWidget(m_listWidget, 1);                             // 放入列表(占比最大)
    rootLayout->addLayout(bottomLayout);                                // 放入底栏

    // 绑定列表项切换信号，实现高亮卡片和更新按钮状态
    connect(m_listWidget, &QListWidget::currentItemChanged,
        this, &GameWidget::onGameSelected);                             // 选中卡片槽

    connect(m_btnPlay, &QPushButton::clicked,
        this, &GameWidget::onPlayClicked);                              // 播放点击槽

    connect(m_btnStop, &QPushButton::clicked,
        this, &GameWidget::onStopClicked);                              // 停止点击槽
}

// 👑 新技能：造卡机 (将 Widget 强塞入 ListItem)
void GameWidget::AddGame(const QString& name, const QString& timeStr)
{
    // 1. 创建底层卡片容器
    QFrame* card = new QFrame();                                        // 卡片背景框
    card->setObjectName("gameCard");                                    // 绑定QSS
    card->setProperty("selected", false);                               // 初始状态未选中

    QVBoxLayout* layout = new QVBoxLayout(card);                        // 卡片内垂直布局
    layout->setContentsMargins(10, 10, 10, 15);                         // 内边距
    layout->setSpacing(8);                                              // 图片和文字的间距

    // 2. 组装卡片内容
    QLabel* cover = new QLabel(card);                                   // 封面图
    cover->setObjectName("cardCover");                                  // 绑定QSS
    cover->setFixedSize(160, 160);                                      // 图里的正方形封面

    QLabel* title = new QLabel(name, card);                             // 游戏名
    title->setObjectName("cardTitle");                                  // 绑定QSS

    QLabel* subTitle = new QLabel(timeStr, card);                       // 游玩时间
    subTitle->setObjectName("cardSub");                                 // 绑定QSS

    layout->addWidget(cover);                                           // 封面居上
    layout->addWidget(title);                                           // 标题居中
    layout->addWidget(subTitle);                                        // 时间居底

    // 3. 将 Widget 放入 ListItem
    QListWidgetItem* item = new QListWidgetItem(m_listWidget);          // 生成空壳Item
    item->setSizeHint(QSize(180, 240));                                 // 给壳子设定固定宽高
    item->setData(Qt::UserRole, name);                                  // 偷偷在数据层存下游戏名

    m_listWidget->addItem(item);                                        // 把壳子塞进列表
    m_listWidget->setItemWidget(item, card);                            // 把漂亮的卡片套进壳子里
}

// 👑 处理选中特效与状态
void GameWidget::onGameSelected(QListWidgetItem* current, QListWidgetItem* previous)
{
    // 取消上一个卡片的高亮
    if (previous) {
        QWidget* preWidget = m_listWidget->itemWidget(previous);        // 捞出上一个UI
        preWidget->setProperty("selected", false);                      // 设为未选中
        preWidget->style()->unpolish(preWidget);                        // 刷新QSS引擎
        preWidget->style()->polish(preWidget);                          // 应用新QSS
    }

    // 点亮当前卡片
    if (current) {
        QWidget* curWidget = m_listWidget->itemWidget(current);         // 捞出当前UI
        curWidget->setProperty("selected", true);                       // 设为选中
        curWidget->style()->unpolish(curWidget);                        // 刷新QSS引擎
        curWidget->style()->polish(curWidget);                          // 👑蓝光边框出现！

        m_selectedGameName = current->data(Qt::UserRole).toString();    // 记下玩家选了哪个游戏

        // 只有没在运行游戏时，才允许点绿钮
        if (m_gameProcess->state() != QProcess::Running) {
            m_btnPlay->setEnabled(true);                                // 激活播放键
        }
    }
}

// ============== 业务逻辑区 ==============

void GameWidget::onPlayClicked()
{
    m_btnPlay->setEnabled(false);                                       // 禁用播放防连点
    m_listWidget->setEnabled(false);                                    // 游戏启动时锁定列表

    // 【此处调用你之前写的 CMDTools 进行解密...】

    m_gameProcess->setProgram("C:/Users/Mars/QTProject/RailCineControl/RailCineControl/Packs/Windows/MiNiWorld.exe");                           // 启动测试程序
    m_gameProcess->start();                                             // 发射！

    if (m_gameProcess->waitForStarted(2000)) {                          // 启动成功
        m_btnStop->setEnabled(true);                                    // 激活红钮
    }
    else {
        QMessageBox::critical(this, u8"错误", u8"启动失败！");
        m_btnPlay->setEnabled(true);                                    // 恢复绿钮
        m_listWidget->setEnabled(true);                                 // 解锁列表
    }
}

void GameWidget::onStopClicked()
{
    m_btnStop->setEnabled(false);                                       // 禁用红钮防连点

    if (m_gameProcess->state() == QProcess::Running) {
        qDebug() << u8"正在执行强制击杀...";

        // 1. 获取当前进程的真实 PID (Process ID)
        qint64 pid = m_gameProcess->processId();

        // 2. 先尝试 Qt 标准的体面击杀方式
        m_gameProcess->terminate();

        // 给它 1 秒钟自己体面退出，如果不体面，我们就帮它体面
        if (!m_gameProcess->waitForFinished(1000)) {
            qDebug() << u8"进程拒绝体面退出，启动 taskkill 进程树连根拔起机制, PID:" << pid;

            // 3. 终极必杀技：调用 Windows 系统命令强杀整个进程树
            // /F = 强制终止 (Force)
            // /T = 终止关联的所有子进程 (Tree)
            // /PID = 指定进程ID
            QStringList args;
            args << "/F" << "/T" << "/PID" << QString::number(pid);

            // 使用 QProcess::execute 阻塞执行这条系统命令
            QProcess::execute("taskkill", args);

            // 兜底补刀
            m_gameProcess->kill();
        }
    }
}

void GameWidget::onGameProcessFinished()
{
    // 【此处调用你之前写的 CMDTools 进行加密还原...】

    m_btnStop->setEnabled(false);                                       // 游戏退出了，红钮变暗
    m_btnPlay->setEnabled(true);                                        // 绿钮重新变亮
    m_listWidget->setEnabled(true);                                     // 列表解锁允许选别的游戏
}

void GameWidget::LoadStyleSheet()
{
    QFile qssFile(":/style.qss");                                       // 替换为你的真实路径
    if (qssFile.open(QFile::ReadOnly)) {
        this->setStyleSheet(QLatin1String(qssFile.readAll()));          // 一把梭哈应用样式
        qssFile.close();
    }
}