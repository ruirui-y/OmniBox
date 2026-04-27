#include "CustomizeEdit.h"

CustomizeEdit::CustomizeEdit(QWidget *parent)
	: QLineEdit(parent), _maxLen(0)
{
	connect(this, &QLineEdit::textChanged, this, &CustomizeEdit::LimitTextLength);
}

CustomizeEdit::~CustomizeEdit()
{}

void CustomizeEdit::SetMaxLength(int maxLen)
{
	_maxLen = maxLen;
}

void CustomizeEdit::focusOutEvent(QFocusEvent* event)
{
	QLineEdit::focusOutEvent(event);
	emit sigFoucusOut();
}

void CustomizeEdit::LimitTextLength(QString text)
{
	if(_maxLen <= 0)
		return;

	QByteArray byteArray = text.toUtf8();

	if (byteArray.size() > _maxLen)
	{
		byteArray = byteArray.left(_maxLen);
		setText(QString::fromLocal8Bit(byteArray));
	}
}