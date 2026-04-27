#include "CMDTools.h"
#include <QProcess>


CMDTools::CMDTools(QObject *parent)
	: QObject(parent)
{}

CMDTools::~CMDTools()
{}

bool CMDTools::RunBlocking(const QStringList & args, QString & outAll, int timeoutMs) const
{
    QProcess p;
    p.setProgram("cmd.exe");
    p.setArguments(args);
    // 合并 stdout/stderr，阅读更简单
    p.setProcessChannelMode(QProcess::MergedChannels);

    p.start();
    if (!p.waitForStarted(qMax(3000, timeoutMs / 10)))
    {
        return false;
    }

    if (!p.waitForFinished(timeoutMs))
    {
        p.kill();
        p.waitForFinished(2000);
        return false;
    }

    outAll = QString::fromLocal8Bit(p.readAll());
    return (p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0);
}