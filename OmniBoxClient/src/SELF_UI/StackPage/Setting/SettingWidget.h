#ifndef SETTINGWIDGET_H
#define SETTINGWIDGET_H

#include <QWidget>

class QStackedWidget;
class QButtonGroup;

class SettingWidget : public QWidget
{
	Q_OBJECT

public:
	explicit SettingWidget(QWidget* parent = nullptr);
	~SettingWidget();

private:
	// --- 核心功能模块化拆分 ---
	void BuildUI();
	void BindSignals();

private:
	QButtonGroup* m_navGroup = nullptr;														// 导航按钮组
	QStackedWidget* m_stackedWidget = nullptr;												// 堆栈窗口
};

#endif // SETTINGWIDGET_H