#include "CustomizeTextEdit.h"

CustomizeTextEdit::CustomizeTextEdit(QWidget *parent)
	: QTextEdit(parent)
{}

CustomizeTextEdit::~CustomizeTextEdit()
{}

void CustomizeTextEdit::focusOutEvent(QFocusEvent * event)
{
	QTextEdit::focusOutEvent(event);
	emit SigFocusOut();
}