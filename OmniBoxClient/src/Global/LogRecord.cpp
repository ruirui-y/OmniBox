#include "LogRecord.h"
#include <QTime>
#include <QFileInfo>
#include <QMutex>
#include <QDir>
#include <QCoreApplication>


// 头文件或顶层
static QFile* s_file = nullptr;
static QMutex   s_mutex;
static thread_local bool s_reentry = false;

LogRecord::LogRecord(QObject* parent)
    : QObject(parent)
{

}

LogRecord::~LogRecord()
{
}

void LogRecord::InitLog(const QString& path)
{
    if (!s_file) {
        s_file = new QFile(path);
        // 确保目录存在
        QDir().mkpath(QFileInfo(*s_file).absolutePath());
        s_file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);

        // 退出时卸载日志
        if (qApp) {
            QObject::connect(qApp, &QCoreApplication::aboutToQuit, []() {
                LogRecord::ShutdownLog();
                });
        }
    }
    qInstallMessageHandler(&LogRecord::customMessageHandler);
}

void LogRecord::ShutdownLog()
{
    // 1. 最优先：卸载 handler，切断新日志产生的源头
    qInstallMessageHandler(nullptr);

    // 2：必须加锁！等所有正在写的线程写完，再安全关门
    QMutexLocker locker(&s_mutex);
    if (s_file) {
        s_file->flush();
        s_file->close();
        delete s_file;
        s_file = nullptr; // 极其重要：置空，防止野指针
    }
}

void LogRecord::customMessageHandler(QtMsgType type,
    const QMessageLogContext& ctxt,
    const QString& msg)
{
    // 防递归
    if (s_reentry) return;
    s_reentry = true;

    // 保护s_file的安全
    QMutexLocker locker(&s_mutex);

    // 在锁的保护下检查，绝对不会发生“查完被别的线程关了”的情况
    if (!s_file || !s_file->isOpen()) {
        s_reentry = false;
        return;
    }

    // 过滤空白
    QString t = msg; t.remove('"'); t = t.trimmed();
    if (t.isEmpty()) { s_reentry = false; return; }

    // 过滤 重复日志信息
    if (msg.contains("Unknown property letter-spacing") ||
        msg.contains("libpng warning: iCCP"))
    {
        s_reentry = false;
        return;
    }

    // Silence qWarning spam due to bug in QTextBrowser (trying to open a file for base64 images)
    if (QString::fromUtf8(ctxt.function) == QString("virtual bool QFSFileEngine::open(QIODevice::OpenMode)")
        && msg == QString("QFSFileEngine::open: No file name specified")) {
        return;
    }
    if (msg.startsWith("Unable to find any suggestion for")) {
        return;
    }

    if (msg == QString("attempted to send message with network family 10 (probably IPv6) on IPv4 socket")) {
        return;
    }

    QRegExp snoreFilter{ QStringLiteral("Snore::Notification.*was already closed") };
    if (type == QtWarningMsg
        && msg.contains(snoreFilter))
    {
        return;
    }

    // 获取文件名
    QString fullPath = QString::fromUtf8(ctxt.file);
    QFileInfo fileInfo(fullPath);
    QString fileName = fileInfo.fileName();

    QDateTime now = QDateTime::currentDateTime();
    QString timeStr = now.toString("yyyy-MM-dd HH:mm:ss.zzz");

    QString LogMsg =
        QString("[%1 UTC] %2:%3 : ").arg(timeStr).arg(fileName).arg(ctxt.line);
    switch (type) {
    case QtDebugMsg:
        LogMsg += "Debug";
        break;
    case QtInfoMsg:
        LogMsg += "Info";
        break;
    case QtWarningMsg:
        LogMsg += "Warning";
        break;
    case QtCriticalMsg:
        LogMsg += "Critical";
        break;
    case QtFatalMsg:
        LogMsg += "Fatal";
        break;
    default:
        break;
    }

    LogMsg += ": " + msg + "\n";

    // 写入文件
    if (s_file->isOpen())
    {
        s_file->write(LogMsg.toUtf8());
        s_file->flush();
    }

    s_reentry = false;

    if (type == QtFatalMsg) {
        // Fatal 后按 Qt 约定中止
        abort();
    }
}

void LogRecord::startRecord(QString fileName)
{
    InitLog(fileName);
}