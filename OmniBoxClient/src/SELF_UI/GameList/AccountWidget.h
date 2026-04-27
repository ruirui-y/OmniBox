#ifndef ACCOUNT_WIDGET_H
#define ACCOUNT_WIDGET_H

#include "CinemaDialogBase.h"                                                   

class QLabel;
class QPushButton;

class AccountWidget : public CinemaDialogBase
{
    Q_OBJECT

public:
    explicit AccountWidget(QWidget* parent = nullptr);
    void setUserName(const QString& name);

private:
    void BuildUI();

private slots:
    void SlotLoginOut();                                                        // 登出触发槽

private:
    QLabel* m_name = nullptr;                                                   // 账号名称动态标签
    QPushButton* switchBtn = nullptr;                                           // 切换/登出核心按钮
};

#endif // !ACCOUNT_WIDGET_H