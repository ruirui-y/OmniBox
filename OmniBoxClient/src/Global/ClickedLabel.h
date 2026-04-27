#ifndef CLICKEDLABEL_H
#define CLICKEDLABEL_H

#include <QLabel>
#include <QString>
#include "Enum.h"

class ClickedLabel  : public QLabel
{
	Q_OBJECT

public:
	ClickedLabel(QWidget *parent = 0);
	~ClickedLabel();

public:
	virtual void mousePressEvent(QMouseEvent *event) override;
	virtual void mouseReleaseEvent(QMouseEvent *event) override;
	virtual void enterEvent(QEvent *event) override;
	virtual void leaveEvent(QEvent *event) override;

	ClickLabelState GetState() const;
	void SetCurrState(ClickLabelState state);

	/* 设置状态对应的文本，方便从样式表中切换UI */
	void SetState(QString normal = "", QString hover = "", QString press = "",
		QString select = "", QString select_hover = "", QString select_press = "");

signals:
	void clicked(QString, ClickLabelState);

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
#endif // CLICKEDLABEL_H