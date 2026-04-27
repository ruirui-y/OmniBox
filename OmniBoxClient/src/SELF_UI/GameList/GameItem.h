#ifndef GAMEITEM_H
#define GAMEITEM_H

#include <QToolButton>
class QMouseEvent;
class QLabel;

class GameItem  : public QToolButton
{
	Q_OBJECT
public:
	GameItem(QString name, QString iconPath, QWidget *parent = 0);
	~GameItem();

protected:
	virtual QSize sizeHint() const override;

private:
	void BuildUI();

signals:
	void ItemClicked(QWidget* widget);

private:
	QLabel* m_pIconLabel = nullptr;
	QLabel* m_pNameLabel = nullptr;
	QString m_name;
	QString m_iconPath;
};

#endif // GAMEITEM_H