#ifndef _STATEWIDGET_H_
#define _STATEWIDGET_H_

#include <QWidget>
#include <QLabel>
#include "Enum.h"

class StateWidget  : public QWidget
{
	Q_OBJECT

public:
	StateWidget(QWidget *parent = 0);
	~StateWidget();

public:
	/* 设置状态对应的文本，方便从样式表中切换UI */
	void SetState(QString normal = "", QString hover = "", QString press = "",
		QString select = "", QString select_hover = "", QString select_press = "");

	ClickLabelState GetCurrState() const;
	void ClearState();

	void SetSelected(bool bSelected);
	void AddRedPoint();
	void ShowRedPoint(bool bShow = true);

protected:
	virtual void paintEvent(QPaintEvent* event) override;

	virtual void mousePressEvent(QMouseEvent* ev) override;
	virtual void mouseReleaseEvent(QMouseEvent* ev) override;
	virtual void enterEvent(QEvent* event) override;
	virtual void leaveEvent(QEvent* event) override;

private:
	void UpdateStyleSheet(QString str);													// 刷新样式

signals:
	void clicked(void);

public slots:

private:
	QString _Normal;																	// 未选中
	QString _NormalHover;
	QString _NormalPress;

	QString _Selected;																	// 选中
	QString _SelectedHover;
	QString _SelectedPress;

	ClickLabelState _CurrState;															// 当前标签状态
	QLabel* _RedPoint;
};

#endif // _STATEWIDGET_H_