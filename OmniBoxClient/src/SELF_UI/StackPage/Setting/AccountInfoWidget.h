#ifndef ACCOUNTINFOWIDGET_H
#define ACCOUNTINFOWIDGET_H

#include <QWidget>

class QComboBox;
class QLineEdit;
class QPushButton;

class AccountInfoWidget : public QWidget
{
	Q_OBJECT

public:
	explicit AccountInfoWidget(QWidget* parent = nullptr);
	~AccountInfoWidget();

private:
	void BuildUI();
	void BindSignals();

private:
	QComboBox* m_langCombo;																	// 语言选项下拉框
	QPushButton* m_logoutBtn;																// 退出按钮
	QLineEdit* m_batteryEdit;																// 电量输入框
	QPushButton* m_confirmBtn;																// 确定按钮
};

#endif // ACCOUNTINFOWIDGET_H