#ifndef CUSTOMIZETEXTEDIT_H
#define CUSTOMIZETEXTEDIT_H

#include <QTextEdit>

class CustomizeTextEdit  : public QTextEdit
{
	Q_OBJECT

public:
	CustomizeTextEdit(QWidget *parent = 0);
	~CustomizeTextEdit();

signals:
	void SigFocusOut();

protected:
	virtual void focusOutEvent(QFocusEvent *event) override;
};

#endif // CUSTOMIZETEXTEDIT_H