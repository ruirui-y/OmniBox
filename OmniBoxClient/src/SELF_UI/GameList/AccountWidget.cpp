#include "AccountWidget.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include "ThreadPool.h"
#include "CinemaMessageBox.h"

AccountWidget::AccountWidget(QWidget* parent)
    : CinemaDialogBase(parent)                                                  // 👑 向上调用基类构造
{
    // 如果基类没有默认设置 Popup 属性，但你想保留点击外部自动关闭的特性，取消下面注释：
    // setWindowFlags(windowFlags() | Qt::Popup);

    BuildUI();
}

void AccountWidget::BuildUI()
{
    SetDialogTitle(u8"账号管理");

    // ====================================================================
    // 1. 创建用于承载 QSS 样式的核心面板
    // ====================================================================
    QWidget* panel = new QWidget(this);
    panel->setObjectName("accountPanel");

    QVBoxLayout* mainLay = new QVBoxLayout(panel);
    mainLay->setContentsMargins(20, 20, 20, 20);                                // 四周留出舒适呼吸感
    mainLay->setSpacing(10);                                                    // 元素之间的纵向间距

    // ====================================================================
    // 2. 账号展示表单区
    // ====================================================================
    QFormLayout* form = new QFormLayout();
    form->setContentsMargins(10, 10, 10, 0);
    form->setSpacing(0);

    QLabel* keyLabel = new QLabel(u8"当前在线:", panel);
    keyLabel->setObjectName("keyLabel");

    m_name = new QLabel(u8"--", panel);                                         // 默认占位符
    m_name->setObjectName("valueLabel");

    form->addRow(keyLabel, m_name);

    // ====================================================================
    // 3. 标准化的操作按钮区
    // ====================================================================
    switchBtn = new QPushButton(u8"切换 / 退出账号", panel);
    switchBtn->setObjectName("switchBtn");
    switchBtn->setCursor(Qt::PointingHandCursor);

    // ====================================================================
    // 5. 内部组装
    // ====================================================================
    mainLay->addLayout(form);
    mainLay->addWidget(switchBtn);

    // ====================================================================
    // 6. 填入基类与事件绑定
    // ====================================================================
    GetContentLayout()->addWidget(panel);

    connect(switchBtn, &QPushButton::clicked, this, &AccountWidget::SlotLoginOut);

    // 锁定精致尺寸 (比原来更大气)
    setFixedSize(300, 220);
}

void AccountWidget::SlotLoginOut()
{
    if (CinemaMessageBox::ShowQuestion(this, u8"提示", u8"确定要退出当前账号吗？"))
    {
        // 用户确认请求下线
        ThreadPool::Instance()->PostTask(ThreadPool::Instance()->GetTCPMgr(), [](TCPMgr* tcp)
            {
                tcp->AccountLoginOut();
            });
    }
}

void AccountWidget::setUserName(const QString& name)
{
    m_name->setText(name);
}