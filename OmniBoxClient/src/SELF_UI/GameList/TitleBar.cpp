#include "TitleBar.h"
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include "UserMgr.h"
#include "AccountWidget.h"

// ==============================================================================
// 内部类：自绘系统控制按钮 (抛弃图片，全向量渲染)
// ==============================================================================
enum class SysBtnType { Minimize, Close };

class SysButton : public QPushButton
{
public:
    SysButton(SysBtnType type, QWidget* parent = nullptr) : QPushButton(parent), m_type(type) {}

protected:
    void paintEvent(QPaintEvent* e) override
    {
        // 1. 让 QPushButton 先画底色 (这样就能完美响应外部 QSS 的 hover/pressed 背景)
        QPushButton::paintEvent(e);

        // 2. 启动高精度自绘
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);                                  // 开启抗锯齿

        // 动态计算画笔颜色：如果鼠标悬浮，关闭按钮画白色，否则画暗灰蓝色
        QColor penColor = "#8A95A5";
        if (underMouse() && m_type == SysBtnType::Close) {
            penColor = "#FFFFFF";                                                       // 悬浮关闭按钮时，线段变白
        }
        else if (underMouse()) {
            penColor = "#00C3FF";                                                       // 悬浮最小化按钮时，线段变科技蓝
        }

        QPen pen(penColor);
        pen.setWidth(2);                                                                // 线条粗细
        pen.setCapStyle(Qt::RoundCap);                                                  // 圆滑线头
        painter.setPen(pen);

        QRect r = rect();
        int cx = r.width() / 2;
        int cy = r.height() / 2;
        int w = 10;                                                                     // 图标的跨度大小

        // 3. 核心向量渲染逻辑
        if (m_type == SysBtnType::Minimize)
        {
            // 画一条底部的横线 [-]
            painter.drawLine(cx - w / 2, cy, cx + w / 2, cy);
        }
        else if (m_type == SysBtnType::Close)
        {
            // 画一个叉叉 [X]
            painter.drawLine(cx - w / 2, cy - w / 2, cx + w / 2, cy + w / 2);
            painter.drawLine(cx - w / 2, cy + w / 2, cx + w / 2, cy - w / 2);
        }
    }

private:
    SysBtnType m_type;
};
// ==============================================================================

TitleBar::TitleBar(QWidget* parent) : QWidget(parent)
{
    setFixedHeight(56);
    setAttribute(Qt::WA_StyledBackground, true);                                        // 启用 QSS 渲染
    setObjectName("MainTitleBar");                                                      // 赋予全局唯一 ID

    QHBoxLayout* hLayout = new QHBoxLayout(this);
    hLayout->setContentsMargins(0, 0, 16, 0);
    hLayout->setSpacing(8);

    InitBtns();

    hLayout->addWidget(m_btnName);
    hLayout->addStretch();
    hLayout->addSpacing(12);
    hLayout->addWidget(m_btnMin);
    hLayout->addWidget(m_btnClose);

    connect(m_btnMin, &QPushButton::clicked, this, &TitleBar::minimizeRequested);
    connect(m_btnClose, &QPushButton::clicked, this, &TitleBar::closeRequested);
}

void TitleBar::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        m_dragOffset = e->globalPos() - window()->frameGeometry().topLeft();            // 计算相对顶层窗口左上的偏移
        e->accept();
    }
    else
    {
        QWidget::mousePressEvent(e);
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent* e)
{
    if (e->buttons() & Qt::LeftButton)
    {
        window()->move(e->globalPos() - m_dragOffset);                                  // 拖动顶层主窗口
        e->accept();
    }
    else
    {
        QWidget::mouseMoveEvent(e);
    }
}

void TitleBar::InitBtns()
{
    // 实例化咱们的自绘按钮
    m_btnMin = new SysButton(SysBtnType::Minimize, this);
    m_btnClose = new SysButton(SysBtnType::Close, this);
    m_btnName = new QPushButton(this);

    // 绑定 QSS ID，彻底解耦
    m_btnMin->setObjectName("TitleMinBtn");
    m_btnClose->setObjectName("TitleCloseBtn");
    m_btnName->setObjectName("TitleAccountBtn");

    // 配置基础属性
    for (QPushButton* btn : { m_btnMin, m_btnClose })
    {
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedSize(36, 36);                                                      // 加大一点判定区域，提升手感
        btn->setToolTip(btn == m_btnMin ? u8"最小化" : u8"关闭");
    }

    // 账号按钮配置
    UserInfo userinfo = UserMgr::Instance()->getUserInfo();
    m_btnName->setText(userinfo.UserName);
    m_btnName->setCursor(Qt::PointingHandCursor);
    m_btnName->setFixedSize(200, 36);                                                   // 宽度与左侧栏对齐
    m_btnName->setToolTip(u8"账号信息");

    // 账号弹层逻辑
    auto accountWidget = new AccountWidget(m_btnName);                                  // 以按钮为父对象，随之销毁
    accountWidget->setUserName(userinfo.UserName);

    connect(m_btnName, &QPushButton::clicked, this, [=]()
        {
            accountWidget->adjustSize();
            QPoint pt = m_btnName->mapToGlobal(QPoint(m_btnName->width(), m_btnName->height()));
            accountWidget->move(pt);
            accountWidget->show();                                                      // 点击外部自动关闭(由弹窗自身属性控制)
        });
}