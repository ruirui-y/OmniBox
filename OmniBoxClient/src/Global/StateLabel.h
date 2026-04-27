#ifndef STATELABEL_H
#define STATELABEL_H

#include <QLabel>
#include <QMouseEvent>
#include "Enum.h"


class StateLabel  : public QLabel
{
	Q_OBJECT

public:
	StateLabel(QWidget *parent = 0);

	virtual void mousePressEvent(QMouseEvent *event) override;
	virtual void mouseReleaseEvent(QMouseEvent *event) override;
	virtual void enterEvent(QEvent *event) override;
	virtual void leaveEvent(QEvent *event) override;

	/* 设置状态对应的文本，方便从样式表中切换UI */
	void SetState(QString normal = "", QString hover = "", QString press = "",
		QString select = "", QString select_hover = "", QString select_press = "");

	~StateLabel();

private:
	void UpdateStyleSheet(QString str);													// 刷新样式

private:
	QString _Normal;																	// 未选中
	QString _NormalHover;
	QString _NormalPress;

	QString _Selected;																	// 选中
	QString _SelectedHover;
	QString _SelectedPress;

	ClickLabelState _CurrState;															// 当前标签状态
};
#endif // STATELABEL_H