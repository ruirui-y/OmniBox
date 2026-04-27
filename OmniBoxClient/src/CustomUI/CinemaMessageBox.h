#ifndef CINEMAMESSAGEBOX_H
#define CINEMAMESSAGEBOX_H

#include "CinemaDialogBase.h"
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>

class CinemaMessageBox : public CinemaDialogBase
{
    Q_OBJECT
public:
    enum Type { Info, Warning, Error, Question };                       // 👑 新增 Question 类型

    static void ShowInfo(QWidget* parent, const QString& title, const QString& text) {
        CinemaMessageBox box(parent, title, text, Info);
        box.exec();
    }
    static void ShowWarning(QWidget* parent, const QString& title, const QString& text) {
        CinemaMessageBox box(parent, title, text, Warning);
        box.exec();
    }
    static void ShowError(QWidget* parent, const QString& title, const QString& text) {
        CinemaMessageBox box(parent, title, text, Error);
        box.exec();
    }

    // 👑 核心绝杀：带有布尔返回值的询问框
    static bool ShowQuestion(QWidget* parent, const QString& title, const QString& text) {
        CinemaMessageBox box(parent, title, text, Question);
        // 如果用户点击“确定”，accept() 会让 exec 返回 QDialog::Accepted
        return box.exec() == QDialog::Accepted;
    }

private:
    CinemaMessageBox(QWidget* parent, const QString& title, const QString& text, Type type)
        : CinemaDialogBase(parent)
    {
        this->resize(400, 220);                                         // 稍微加宽一点，显得更大气

        // 1. 设置带 Emoji 的标题
        QString fullTitle;
        if (type == Info) fullTitle = u8"💡 " + title;
        else if (type == Warning) fullTitle = u8"⚠️ " + title;
        else if (type == Error) fullTitle = u8"❌ " + title;
        else fullTitle = u8"❓ " + title;                               // 👑 询问类型的 Emoji
        this->SetDialogTitle(fullTitle);

        QVBoxLayout* content = this->GetContentLayout();

        // 2. 核心提示文字
        QLabel* lbl_msg = new QLabel(text, this);
        lbl_msg->setObjectName("CinemaMsgLabel");                       // 绑定专属 ID
        lbl_msg->setWordWrap(true);
        lbl_msg->setAlignment(Qt::AlignCenter);

        // 3. 底部按钮组装
        QHBoxLayout* btn_layout = new QHBoxLayout();
        btn_layout->addStretch();

        if (type == Question)
        {
            // 👑 询问框专属逻辑：双按钮布局
            QPushButton* btn_cancel = new QPushButton(u8"取 消", this);
            btn_cancel->setMinimumSize(100, 38);
            btn_cancel->setObjectName("btnCinemaCancel");               // 赋予“取消按钮”专属低调样式

            QPushButton* btn_confirm = new QPushButton(u8"确 定", this);
            btn_confirm->setMinimumSize(100, 38);
            // 删除操作属于高危动作，直接复用“影院金”或“警示红”，这里用警示红更有压迫感
            btn_confirm->setObjectName("btnCinemaError");

            btn_layout->addWidget(btn_cancel);
            btn_layout->addSpacing(20);                                 // 两个按钮拉开一点安全距离
            btn_layout->addWidget(btn_confirm);

            // 信号绑定：取消对应 reject(false)，确定对应 accept(true)
            connect(btn_cancel, &QPushButton::clicked, this, &CinemaMessageBox::reject);
            connect(btn_confirm, &QPushButton::clicked, this, &CinemaMessageBox::accept);
        }
        else
        {
            // 其他常规框逻辑：单按钮布局
            QPushButton* btn_ok = new QPushButton(u8"我知道了", this);
            btn_ok->setMinimumSize(120, 38);

            if (type == Info) btn_ok->setObjectName("btnCinemaInfo");
            else if (type == Warning) btn_ok->setObjectName("btnCinemaWarning");
            else btn_ok->setObjectName("btnCinemaError");

            btn_layout->addWidget(btn_ok);
            connect(btn_ok, &QPushButton::clicked, this, &CinemaMessageBox::accept);
        }

        btn_layout->addStretch();

        content->addStretch();
        content->addWidget(lbl_msg);
        content->addStretch();
        content->addLayout(btn_layout);
    }
};

#endif // CINEMAMESSAGEBOX_H