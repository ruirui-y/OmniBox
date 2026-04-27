#ifndef CMDTOOLS_H
#define CMDTOOLS_H

#include <QObject>

class CMDTools  : public QObject
{
	Q_OBJECT

public:
	CMDTools(QObject *parent);
	~CMDTools();

public:
	bool RunBlocking(const QStringList& args, QString& outAll, int timeoutMs) const;
};

#endif // CMDTOOLS_H