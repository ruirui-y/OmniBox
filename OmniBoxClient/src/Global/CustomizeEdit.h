#ifndef CUSTOMIZEEDIT_H
#define CUSTOMIZEEDIT_H

#include <QLineEdit>
#include <QWidget>
#include <QFocusEvent>

class CustomizeEdit  : public QLineEdit
{
	Q_OBJECT

public:
	CustomizeEdit(QWidget *parent = 0);
	~CustomizeEdit();
	void SetMaxLength(int maxLen);

protected:
	virtual void focusOutEvent(QFocusEvent *event) override;

signals:
	void sigFoucusOut();

private:
	void LimitTextLength(QString text);

private:
	int _maxLen;
};

#endif // CUSTOMIZEEDIT_H