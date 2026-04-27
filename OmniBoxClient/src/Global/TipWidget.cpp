#include "TipWidget.h"

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGraphicsDropShadowEffect>
#include <QScreen>
#include <QGuiApplication>

TipWidget::TipWidget(QWidget *parent)
	: QWidget(parent,
		Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setWindowOpacity(0.0);

    m_label = new QLabel(this);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setStyleSheet(
        "QLabel{ color:#EAECEF; background:rgba(0,0,0,180);"
        " border:1px solid rgba(255,255,255,32); border-radius:10px;"
        " padding:10px 16px; font-size:14px; }");
}

// 统一的面板/按钮样式
static QString kPanelStyle()
{
    return QString::fromUtf8(
        "#panel{ background:rgba(24,25,32,230); border-radius:14px; }"
        "QLabel{ color:#EAF0F6; font-size:14px; }"
        "QPushButton{ min-height:15px; padding:8px 18px; border-radius:12px; }"
        "QPushButton#btnCancel{ color:#EAF0F6; background:rgba(255,255,255,36); "
        " border:1px solid rgba(255,255,255,50); }"
        "QPushButton#btnCancel:hover{ background:rgba(255,255,255,54);} "
        "QPushButton#btnCancel:pressed{ background:rgba(255,255,255,80);} "
        "QPushButton#btnOk{ color:#FFFFFF; background:#169CFF; border:0; "
        " font-weight:400; }"
        "QPushButton#btnOk:hover{ background:#2AA8FF; }"
        "QPushButton#btnOk:pressed{ background:#0E87E6; }"
    );
}

static void centerOnParent(QDialog* dlg, QWidget* parent)
{
    dlg->adjustSize();
    QRect pr = parent->rect();

    QPoint p((pr.width() - dlg->width()) / 2,
        int(pr.height() * 0.7) - dlg->height() / 2 + 15);
    dlg->move(parent->mapToGlobal(p));                                                                    // 把 parent 坐标转换到屏幕坐标
}

void TipWidget::showTip(QWidget* parent, const QString& text, int ms)
{
    auto* tipWidget = new TipWidget(parent);
    tipWidget->m_label->setText(text);
    tipWidget->m_label->adjustSize();
    tipWidget->resize(tipWidget->m_label->size());

    QRect pr = parent->rect();                                                                                  // parent 的本地坐标系
    QPoint p((pr.width() - tipWidget->width()) / 2,
        int(pr.height() * 0.7) - tipWidget->height() / 2 + 15);
    tipWidget->move(parent->mapToGlobal(p));                                                                    // 把 parent 坐标转换到屏幕坐标
    tipWidget->show();

    auto* ani = new QPropertyAnimation(tipWidget, "windowOpacity", tipWidget);
    ani->setDuration(250);
    ani->setStartValue(0.0);
    ani->setEndValue(1.0);
    ani->start(QAbstractAnimation::DeleteWhenStopped);

    QTimer::singleShot(ms, tipWidget, [tipWidget]
        {
            auto* out = new QPropertyAnimation(tipWidget, "windowOpacity", tipWidget);
            out->setDuration(250);
            out->setStartValue(1.0);
            out->setEndValue(0.0);
            QObject::connect(out, &QPropertyAnimation::finished, tipWidget, &QWidget::deleteLater);
            out->start(QAbstractAnimation::DeleteWhenStopped);
        });
}

bool TipWidget::confirm(QWidget* parent, const QString& text, const QString& okText, const QString& cancelText)
{
    QDialog dlg(parent, Qt::FramelessWindowHint | Qt::Dialog);
    dlg.setModal(true);
    dlg.setAttribute(Qt::WA_TranslucentBackground);

    auto* root = new QVBoxLayout(&dlg);
    root->setContentsMargins(0, 0, 0, 0);

    auto* panel = new QWidget(&dlg);
    panel->setObjectName("panel");
    panel->setStyleSheet(kPanelStyle());
    root->addWidget(panel);

    // 阴影
    auto* shadow = new QGraphicsDropShadowEffect(panel);
    shadow->setBlurRadius(24);
    shadow->setColor(QColor(0, 0, 0, 180));
    shadow->setOffset(0, 6);
    panel->setGraphicsEffect(shadow);

    auto* lay = new QVBoxLayout(panel);
    lay->setContentsMargins(24, 20, 24, 16);
    lay->setSpacing(16);

    auto* lb = new QLabel(text, panel);
    lb->setWordWrap(true);
    lay->addWidget(lb);

    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(12);
    btnRow->addStretch();

    // 按钮
    auto* btnCancel = new QPushButton(cancelText, panel);
    btnCancel->setObjectName("btnCancel");
    btnCancel->setCursor(Qt::PointingHandCursor);
    auto* btnOk = new QPushButton(okText, panel);
    btnOk->setObjectName("btnOk");
    btnOk->setCursor(Qt::PointingHandCursor);

    btnRow->addWidget(btnCancel);
    btnRow->addWidget(btnOk);
    lay->addLayout(btnRow);

    QObject::connect(btnCancel, &QPushButton::clicked, &dlg, &QDialog::reject);
    QObject::connect(btnOk, &QPushButton::clicked, &dlg, &QDialog::accept);

    centerOnParent(&dlg, parent);
    const int ret = dlg.exec();
    return (ret == QDialog::Accepted);
}
