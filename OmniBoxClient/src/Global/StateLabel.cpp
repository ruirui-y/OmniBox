#include "StateLabel.h"
#include "Global.h"
#include <QDebug>


StateLabel::StateLabel(QWidget *parent)
	: QLabel(parent), _CurrState(ClickLabelState::Normal)
{
	setCursor(Qt::PointingHandCursor);
}

void StateLabel::mousePressEvent(QMouseEvent * event)
{
	if (event->button() == Qt::LeftButton)
	{
		if (_CurrState == ClickLabelState::Selected)
		{
			_CurrState = ClickLabelState::Normal;
			UpdateStyleSheet(_NormalPress);
		}
		if (_CurrState == ClickLabelState::Normal)
		{
			_CurrState = ClickLabelState::Selected;
			UpdateStyleSheet(_SelectedPress);
		}
	}
	QLabel::mousePressEvent(event);
}

void StateLabel::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		if (_CurrState == ClickLabelState::Normal)
		{
			UpdateStyleSheet(_NormalHover);
		}
		else
		{
			UpdateStyleSheet(_SelectedHover);
		}
	}
	QLabel::mouseReleaseEvent(event);
}

void StateLabel::enterEvent(QEvent* event)
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

void StateLabel::leaveEvent(QEvent* event)
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

StateLabel::~StateLabel()
{}

void StateLabel::SetState(QString normal, QString hover, QString press,
	QString select, QString select_hover, QString select_press)
{
	_Normal = normal;
	_NormalHover = hover;
	_NormalPress = press;

	_Selected = select;
	_SelectedHover = select_hover;
	_SelectedPress = select_press;

	UpdateStyleSheet(_Normal);
}

void StateLabel::UpdateStyleSheet(QString str)
{
	setProperty("state", str);
	repolish(this);
	update();
}