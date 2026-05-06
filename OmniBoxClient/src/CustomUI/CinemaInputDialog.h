#ifndef CINEMAINPUTDIALOG_H
#define CINEMAINPUTDIALOG_H

#include "CinemaDialogBase.h"
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

class CinemaInputDialog : public CinemaDialogBase
{
    Q_OBJECT
public:
    // 直接替代 QInputDialog::getText
    static QString GetInput(QWidget* parent, const QString& title, const QString& labelText, const QString& defaultText, bool* ok = nullptr)
    {
        CinemaInputDialog box(parent, title, labelText, defaultText);
        if (box.exec() == QDialog::Accepted)
        {
            if (ok) *ok = true;
            return box.GetText();
        }
        if (ok) *ok = false;
        return QString();
    }

private:
    CinemaInputDialog(QWidget* parent, const QString& title, const QString& labelText, const QString& defaultText)
        : CinemaDialogBase(parent)
    {
        this->resize(420, 240);                                         // 宽度给够，输入框才大气

        // 1. 设置标题 (默认带个编辑小图标)
        this->SetDialogTitle(u8"✏️ " + title);

        QVBoxLayout* content = this->GetContentLayout();

        // 2. 提示文字
        QLabel* lbl_msg = new QLabel(labelText, this);
        lbl_msg->setObjectName("CinemaMsgLabel");                       // 复用你的提示文本样式

        // 3. 核心输入框
        m_lineEdit = new QLineEdit(this);
        m_lineEdit->setText(defaultText);
        m_lineEdit->setObjectName("CinemaLineEdit");                    // 专属 QSS ID
        m_lineEdit->selectAll();                                        // 默认全选文本，方便用户直接打字覆盖

        // 4. 底部按钮组装
        QHBoxLayout* btn_layout = new QHBoxLayout();
        btn_layout->addStretch();

        QPushButton* btn_cancel = new QPushButton(u8"取 消", this);
        btn_cancel->setMinimumSize(100, 38);
        btn_cancel->setObjectName("btnCinemaCancel");                   // 复用取消按钮样式

        QPushButton* btn_confirm = new QPushButton(u8"确 定", this);
        btn_confirm->setMinimumSize(100, 38);
        btn_confirm->setObjectName("btnCinemaInfo");                    // 使用正常的确认按钮样式(不要用红色危险样式)

        btn_layout->addWidget(btn_cancel);
        btn_layout->addSpacing(20);
        btn_layout->addWidget(btn_confirm);
        btn_layout->addStretch();

        // 5. 组装整体内容
        content->addStretch();
        content->addWidget(lbl_msg);
        content->addSpacing(10);
        content->addWidget(m_lineEdit);
        content->addStretch();
        content->addLayout(btn_layout);

        // 6. 信号绑定
        connect(btn_cancel, &QPushButton::clicked, this, &CinemaInputDialog::reject);
        connect(btn_confirm, &QPushButton::clicked, this, &CinemaInputDialog::accept);

        // 按回车键直接确认
        connect(m_lineEdit, &QLineEdit::returnPressed, this, &CinemaInputDialog::accept);
    }

    QString GetText() const {
        return m_lineEdit->text().trimmed();                            // 顺手去除首尾空格
    }

private:
    QLineEdit* m_lineEdit;
};

#endif // CINEMAINPUTDIALOG_H