#include "ClickedLabel.h"

#include <QMouseEvent>
#include <QDebug>
#include "Global.h"

ClickedLabel::ClickedLabel(QWidget *parent)
	: QLabel(parent), _CurrState(ClickLabelState::Normal)
{
	setCursor(Qt::PointingHandCursor);
}

ClickedLabel::~ClickedLabel()
{}

/* 刷新样式 */
void ClickedLabel::UpdateStyleSheet(QString str)
{
	setProperty("state", str);
	repolish(this);
	update();
}

/* 鼠标按下事件 */
void ClickedLabel::mousePressEvent(QMouseEvent * event)
{
	QLabel::mousePressEvent(event);
}

void ClickedLabel::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		if (_CurrState == ClickLabelState::Normal)
		{
			_CurrState = ClickLabelState::Selected;
			UpdateStyleSheet(_NormalPress);
		}
		else
		{
			_CurrState = ClickLabelState::Normal;
			UpdateStyleSheet(_SelectedPress);
		}
		emit clicked(text(), _CurrState);
	}
}

/* 鼠标悬停进入事件 */
void ClickedLabel::enterEvent(QEvent* event)
{
	if (_CurrState == ClickLabelState::Normal)
	{
		UpdateStyleSheet(_NormalHover);
	}
	else
	{
		UpdateStyleSheet(_SelectedHover);
	}

	QLabel::enterEvent(event);
}

/* 鼠标离开事件 */
void ClickedLabel::leaveEvent(QEvent* event)
{
	if (_CurrState == ClickLabelState::Normal)
	{
		UpdateStyleSheet(_Normal);
	}
	else
	{
		UpdateStyleSheet(_Selected);
	}
	QLabel::leaveEvent(event);
}

ClickLabelState ClickedLabel::GetState() const
{
	return _CurrState;
}

void ClickedLabel::SetCurrState(ClickLabelState state)
{
	_CurrState = state;
	if (state == ClickLabelState::Normal)
	{
		UpdateStyleSheet(_Normal);
	}
	else
	{
		UpdateStyleSheet(_Selected);
	}
}

void ClickedLabel::SetState(QString normal, QString hover, QString press, QString select, QString select_hover, QString select_press)
{
	_Normal = normal;
	_NormalHover = hover;
	_NormalPress = press;

	_Selected = select;
	_SelectedHover = select_hover;
	_SelectedPress = select_press;

	_CurrState = ClickLabelState::Normal;

	UpdateStyleSheet(_Normal);
}