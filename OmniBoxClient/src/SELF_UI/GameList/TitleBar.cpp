#include "TitleBar.h"
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include "AccountWidget.h"

// ==============================================================================
// 内部类：自绘系统控制按钮 (全向量渲染，支持 QSS 状态)
// ==============================================================================
enum class SysBtnType { Minimize, Close };

class SysButton : public QPushButton
{
public:
    SysButton(SysBtnType type, QWidget* parent = nullptr) : QPushButton(parent), m_type(type) {}

protected:
    void paintEvent(QPaintEvent* e) override
    {
        QPushButton::paintEvent(e);                                                 // 先由父类绘制背景和边框
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);                              // 开启抗锯齿

        // 颜色逻辑：优先匹配 QSS 悬浮态，若无则使用标准色
        QColor penColor = underMouse() ? (m_type == SysBtnType::Close ? "#FFFFFF" : "#00C3FF") : "#8A95A5";
        QPen pen(penColor);
        pen.setWidth(2);                                                            // 线条宽度
        pen.setCapStyle(Qt::RoundCap);                                              // 圆润线头
        painter.setPen(pen);

        int cx = rect().width() / 2;
        int cy = rect().height() / 2;
        int w = 10;                                                                 // 图标跨度

        if (m_type == SysBtnType::Minimize) {
            painter.drawLine(cx - w / 2, cy, cx + w / 2, cy);                       // 绘制最小化横线
        }
        else {
            painter.drawLine(cx - w / 2, cy - w / 2, cx + w / 2, cy + w / 2);       // 绘制关闭叉叉
            painter.drawLine(cx - w / 2, cy + w / 2, cx + w / 2, cy - w / 2);
        }
    }
private:
    SysBtnType m_type;
};

// ==============================================================================
// TitleBar 实现
// ==============================================================================

TitleBar::TitleBar(QWidget* parent) : QWidget(parent)
{
    setFixedHeight(56);                                                             // 设置固定高度
    setAttribute(Qt::WA_StyledBackground, true);                                    // 确保 QSS 生效

    QHBoxLayout* hLayout = new QHBoxLayout(this);
    hLayout->setContentsMargins(0, 0, 16, 0);                                       // 右侧留出边距
    hLayout->setSpacing(8);

    InitBtns();                                                                     // 初始化按钮组件

    hLayout->addWidget(m_btnLeft);                                                  // 添加左侧内容区
    hLayout->addStretch();                                                          // 添加弹簧
    hLayout->addWidget(m_btnMin);                                                   // 添加最小化
    hLayout->addWidget(m_btnClose);                                                 // 添加关闭

    connect(m_btnMin, &QPushButton::clicked, this, &TitleBar::minimizeRequested);   // 转发最小化信号
    connect(m_btnClose, &QPushButton::clicked, this, &TitleBar::closeRequested);     // 转发关闭信号
}

void TitleBar::SetMode(TitleMode mode)
{
    m_currentMode = mode;                                                           // 记录当前模式

    if (mode == TitleMode::Login) {
        m_btnLeft->setEnabled(false);                                               // 登录模式不可点击
        m_btnLeft->setText("");                                                     // 清空文字
        m_btnLeft->setToolTip("");                                                  // 移除提示
    }
    else {
        m_btnLeft->setEnabled(true);                                                // 中控模式允许点击
        m_btnLeft->setToolTip(u8"查看账户信息");                                      // 设置提示

        // 绑定中控特有的点击弹窗逻辑
        connect(m_btnLeft, &QPushButton::clicked, this, [=]() {
            auto accountWidget = new AccountWidget(this);                           // 动态创建账户浮窗
            accountWidget->setUserName(m_btnLeft->text());                          // 传递最新用户名
            accountWidget->adjustSize();                                            // 自动调整大小

            // 计算位置：出现在按钮正下方
            QPoint pt = m_btnLeft->mapToGlobal(QPoint(0, m_btnLeft->height()));
            accountWidget->move(pt);
            accountWidget->show();
            }, Qt::UniqueConnection);                                                   // 确保不重复绑定
    }
}

void TitleBar::SetUserName(const QString& name)
{
    if (m_currentMode == TitleMode::Hub) {
        m_btnLeft->setText(name);                                                   // 仅在 Hub 模式更新文字
        m_btnLeft->setIcon(QIcon());                                                // 移除 Logo
    }

    // 根据文字内容自动调整宽度
    m_btnLeft->setFixedWidth(m_btnLeft->sizeHint().width());
}

void TitleBar::SetLogo(const QString& iconPath)
{
    if (m_currentMode == TitleMode::Login) {
        m_btnLeft->setIcon(QIcon(iconPath));                                        // 仅在 Login 模式显示 Logo
        m_btnLeft->setIconSize(QSize(35, 35));                                      // 设定 Logo 大小
        m_btnLeft->setText("");
    }

    // 设置固定宽度
    m_btnLeft->setFixedWidth(60);
}

void TitleBar::InitBtns()
{
    // 实例化左侧复用按钮
    m_btnLeft = new QPushButton(this);
    m_btnLeft->setObjectName("TitleAccountBtn");                                    // 绑定 QSS ID
    m_btnLeft->setCursor(Qt::PointingHandCursor);

    // 给一个初始的最小宽度 (例如和高度一致，呈正方形)
    m_btnLeft->setMinimumWidth(56);                                                 // 设置初始参考宽度
    m_btnLeft->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    // 实例化自绘系统按钮
    m_btnMin = new SysButton(SysBtnType::Minimize, this);
    m_btnClose = new SysButton(SysBtnType::Close, this);

    m_btnMin->setObjectName("TitleMinBtn");                                         // 绑定 QSS ID
    m_btnClose->setObjectName("TitleCloseBtn");                                     // 绑定 QSS ID

    for (QPushButton* btn : { m_btnMin, m_btnClose }) {
        btn->setFixedSize(36, 36);                                                  // 设置按钮尺寸
        btn->setCursor(Qt::PointingHandCursor);                                     // 设置手型光标
    }
}

void TitleBar::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        m_dragOffset = e->globalPos() - window()->frameGeometry().topLeft();        // 记录鼠标相对偏移
        e->accept();
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent* e)
{
    if (e->buttons() & Qt::LeftButton) {
        window()->move(e->globalPos() - m_dragOffset);                             // 移动顶层窗口
        e->accept();
    }
}

void TitleBar::paintEvent(QPaintEvent* e)
{
    QWidget::paintEvent(e);                                                         // 支持 QSS 渲染
}