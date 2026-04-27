#include "CinemaDialogBase.h"

#include <QHBoxLayout>
#include <QPushButton>

CinemaDialogBase::CinemaDialogBase(QWidget* parent) : QDialog(parent)
{
    SetupBaseUI();
}

void CinemaDialogBase::SetupBaseUI()
{
    // 1. 斩断系统边框
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    this->setObjectName("CinemaDialogBase");                            // 👑 换成全新的影院底板 ID

    QVBoxLayout* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);

    // ==========================================
    // 2. 绘制统一的标题栏
    // ==========================================
    QWidget* title_bar = new QWidget(this);
    title_bar->setObjectName("DialogTitleBar");                         // 👑 换成弹窗专属标题栏 ID
    title_bar->setFixedHeight(40);

    QHBoxLayout* title_layout = new QHBoxLayout(title_bar);
    title_layout->setContentsMargins(15, 0, 10, 0);

    lbl_title_ = new QLabel(u8"对话框", title_bar);
    lbl_title_->setObjectName("DialogTitleLabel");                      // 👑 标题文本 ID

    QPushButton* btn_close = new QPushButton(u8"✕", title_bar);
    btn_close->setObjectName("DialogBtnClose");                         // 👑 关闭按钮 ID
    btn_close->setFixedSize(35, 30);

    title_layout->addWidget(lbl_title_);
    title_layout->addStretch();
    title_layout->addWidget(btn_close);

    // 基类直接处理掉关闭逻辑
    connect(btn_close, &QPushButton::clicked, this, &CinemaDialogBase::reject);

    // ==========================================
    // 3. 绘制供子类使用的内容容器
    // ==========================================
    QWidget* content_widget = new QWidget(this);
    content_layout_ = new QVBoxLayout(content_widget);
    content_layout_->setContentsMargins(15, 15, 15, 15); // 恢复正常的内边距

    main_layout->addWidget(title_bar);
    main_layout->addWidget(content_widget, 1); // 伸展因子设为1，撑满剩余空间
}

void CinemaDialogBase::SetDialogTitle(const QString& title)
{
    if (lbl_title_) {
        lbl_title_->setText(title);
    }
}

QVBoxLayout* CinemaDialogBase::GetContentLayout() const
{
    return content_layout_;
}

// ==========================================
// 拖拽逻辑 (基类包揽，子类躺平)
// ==========================================
void CinemaDialogBase::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && event->pos().y() < 40) {
        is_dragging_ = true;
        drag_start_position_ = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void CinemaDialogBase::mouseMoveEvent(QMouseEvent* event) {
    if (is_dragging_ && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - drag_start_position_);
        event->accept();
    }
}

void CinemaDialogBase::mouseReleaseEvent(QMouseEvent* event) {
    is_dragging_ = false;
    event->accept();
}