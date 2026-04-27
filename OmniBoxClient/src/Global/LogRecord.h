#ifndef LOGRECORD_H
#define LOGRECORD_H

#include <QObject>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QDebug>
#include <QtGlobal>


class LogRecord  : public QObject
{
	Q_OBJECT

public:
	LogRecord(QObject *parent = nullptr);
	~LogRecord();

public:
	static void customMessageHandler(QtMsgType type, 
		const QMessageLogContext &context, const QString &msg);							// 自定义消息处理函数，将Qt日志输出到文件
	static void startRecord(QString fileName);											// 开始记录日志

	static void InitLog(const QString& path);
	static void ShutdownLog();
};

#endif // LOGRECORD_H