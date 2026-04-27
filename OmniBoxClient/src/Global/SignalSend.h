#ifndef SIGNALSEND_H
#define SIGNALSEND_H

#include <QObject>
#include "Struct.h"
#include "singletion.h"
#include <functional>
using Task = std::function<void()>;

class SignalSend  : public QObject, public Singleton<SignalSend>
{
	Q_OBJECT

	friend class Singleton<SignalSend>;
public:
	~SignalSend();

private:
	SignalSend(QObject* parent = 0);

signals:
	void SigSendMessage(UDPSendMessage Msg);																	// 发送UDP消息
	void SigOnDeviceLogin();																					// 添加设备到UI窗口中，创建设备项
	void SigOnDeviceOffline();																					// 设备断开，超时检测触发，通知UI窗口将设备置为NULL

	void SigNotifyDeviceRename(QString id, QString newName);													// 通知设备重命名，发送UDP讯号到头显中修改昵称
	void SigNotifyGameExit(QString);																			// 游戏累计超过阈值，设备退出
	void SigGameStartResult(bool, QString);																		// 游戏启动结果

	void SigAddDeviceRet(bool);																					// 添加设备结果
	void SigRemoveDeviceRet(bool);																				// 移除设备结果
	void SigStopHearBeat(QString);																				// 停止心跳

	void SigAddTaskToThreadPool(Task);																			// 添加任务到线程池的任务队列中
};

#endif // SIGNALSEND_H