#ifndef DEVELOPEROPTIONWIDGET_H
#define DEVELOPEROPTIONWIDGET_H

#include <QWidget>

class QVBoxLayout;
class QScrollArea;

class DeveloperOptionWidget : public QWidget
{
	Q_OBJECT

public:
	explicit DeveloperOptionWidget(QWidget* parent = nullptr);
	~DeveloperOptionWidget();

private:
	void BuildUI();
	void BindSignals();

private:
	QVBoxLayout* m_listLayout;																// 存放所有设备行的列表布局
};

#endif // DEVELOPEROPTIONWIDGET_H