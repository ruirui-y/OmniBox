#include "StateWidget.h"
#include "Global.h"
#include <QStyleOption>
#include <QPainter>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QVBoxLayout>
#include <QDebug>

StateWidget::StateWidget(QWidget *parent)
	: QWidget(parent), _CurrState(ClickLabelState::Normal)
{
	setCursor(Qt::PointingHandCursor);
	AddRedPoint();
}

StateWidget::~StateWidget()
{}

ClickLabelState StateWidget::GetCurrState() const
{
	return _CurrState;
}

void StateWidget::ClearState()
{
	_CurrState = ClickLabelState::Normal;
	UpdateStyleSheet(_Normal);
}

void StateWidget::SetSelected(bool bSelected)
{
	if (bSelected)
	{
		_CurrState = ClickLabelState::Selected;
		UpdateStyleSheet(_Selected);
		return;
	}

	_CurrState = ClickLabelState::Normal;
	UpdateStyleSheet(_Normal);
	return;
}

void StateWidget::AddRedPoint()
{
	_RedPoint = new QLabel;
	_RedPoint->setObjectName("RedPoint");
	QVBoxLayout* vLayout = new QVBoxLayout;
	_RedPoint->setAlignment(Qt::AlignCenter);
	vLayout->addWidget(_RedPoint);
	vLayout->setMargin(0);
	_RedPoint->setVisible(false);
	setLayout(vLayout);
}

void StateWidget::ShowRedPoint(bool bShow)
{
	_RedPoint->setVisible(bShow);
}

void StateWidget::paintEvent(QPaintEvent* event)
{
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void StateWidget::mousePressEvent(QMouseEvent* ev)
{
	if (ev->button() == Qt::LeftButton)
	{
		_CurrState = ClickLabelState::Selected;
		UpdateStyleSheet(_SelectedPress);
		return;
	}
	QWidget::mousePressEvent(ev);
}

void StateWidget::mouseReleaseEvent(QMouseEvent* ev)
{
	if (ev->button() == Qt::LeftButton)
	{
		_CurrState = ClickLabelState::Normal;
		UpdateStyleSheet(_NormalPress);
		emit clicked();
		return;
	}
	QWidget::mouseReleaseEvent(ev);
}

void StateWidget::enterEvent(QEvent* event)
{
	if (_CurrState == ClickLabelState::Normal)
	{
		UpdateStyleSheet(_NormalHover);
	}
	else if (_CurrState == ClickLabelState::Selected)
	{
		UpdateStyleSheet(_SelectedHover);
	}
	QWidget::enterEvent(event);
}

void StateWidget::leaveEvent(QEvent* event)
{
	if (_CurrState == ClickLabelState::Normal)
	{
		UpdateStyleSheet(_Normal);
	}
	else if (_CurrState == ClickLabelState::Selected)
	{
		UpdateStyleSheet(_Selected);
	}
	QWidget::leaveEvent(event);
}

void StateWidget::UpdateStyleSheet(QString str)
{
	setProperty("state", str);
	repolish(this);
	update();
}

void StateWidget::SetState(QString normal, QString hover, QString press, QString select, QString select_hover, QString select_press)
{
	_Normal = normal;
	_NormalHover = hover;
	_NormalPress = press;

	_Selected = select;
	_SelectedHover = select_hover;
	_SelectedPress = select_press;

	UpdateStyleSheet(_Normal);
}