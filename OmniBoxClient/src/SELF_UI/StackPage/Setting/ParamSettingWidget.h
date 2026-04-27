#ifndef PARAMSETTINGWIDGET_H
#define PARAMSETTINGWIDGET_H

#include <QWidget>

class QGridLayout;

class ParamSettingWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ParamSettingWidget(QWidget* parent = nullptr);
	~ParamSettingWidget();

private:
	void BuildUI();

private:
	QGridLayout* m_gridLayout;
};

#endif // PARAMSETTINGWIDGET_H