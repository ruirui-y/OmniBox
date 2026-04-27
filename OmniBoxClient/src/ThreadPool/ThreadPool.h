#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <QObject>
#include <QMutex>
#include <QVector>
#include <memory>
#include <type_traits>
#include <QWeakPointer>
#include "singletion.h"
#include "Global.h"
#include "WorkerThread.h"
#include "TCPMgr.h"
#include "UdpManager.h"

class ThreadPool  : public QObject, public Singleton<ThreadPool>
{
	Q_OBJECT

	friend class Singleton<ThreadPool>;
public:
	ThreadPool(QObject *parent = 0);
	~ThreadPool();

	void Start(size_t threadNum);
	void Stop();

public:
	// ================= 暴露的服务接口 =================
	TCPMgr* GetTCPMgr() const { return tcp_mgr_.get(); }
	UdpManager* GetUdpManager() const { return udp_manager_.get(); }

public:
	// ================= 泛型服务接口 =================
	// 创建对象
	template<class T, template<typename> class SmartPtr = QSharedPointer, class... Args>
	SmartPtr<T> CreateQObject(Args&&... args)
	{
		// 1. 安全提权，获取工作线程
		if (auto thread = GetThread())
		{
			// 2. 完美转发：将类型 T、指针类型 SmartPtr，以及所有构造参数原封不动地转发给子线程
			return thread->CreateQObject<T, SmartPtr>(std::forward<Args>(args)...);
		}

		// 3. 提权失败（线程池未启动或线程已销毁）时的安全兜底
		qWarning() << "ThreadPool Exception: Failed to get thread for creating" << typeid(T).name();
		return SmartPtr<T>();
	}

	// 万能跨线程任务投递通道
	// 参数1：你要操作的目标对象指针
	// 参数2：你想在这个对象所在的线程里干什么事（Lambda）
	template<typename T, typename TaskFunc>
	void PostTask(T* targetObj, TaskFunc&& task)
	{
		QMutexLocker locker(&mutex_); // 保障线程池状态安全
		if (!started_ || !targetObj) return;

		// 完美转发，跨线程投递
		QMetaObject::invokeMethod(targetObj, [obj = targetObj, func = std::forward<TaskFunc>(task)]() mutable {
			func(obj); // 在目标线程内解包并执行
			}, Qt::QueuedConnection);
	}

	// 终极泛型分发引擎：自动处理线程获取、生命周期保持与跨线程跳跃
	template<typename TaskFunc>
	void DispatchToWorker(TaskFunc&& task)
	{
		// 1. 获取线程智能指针（必须按值捕获以延长生命周期）
		auto thread = GetThread();
		if (!thread) return;

		// 2. 获取目标子线程的锚点
		QObject* dispatcher = thread->Dispatcher();
		if (!dispatcher) return;

		// 3. 跨线程时空跳跃，并在子线程中解包执行真正的任务
		QMetaObject::invokeMethod(dispatcher, [thread, func = std::forward<TaskFunc>(task)]() mutable 
			{
				// 编译期分支判断
				// 判断 func 是否可以接受 WorkerThread* 类型的参数
				if constexpr (std::is_invocable_v<TaskFunc, WorkerThread*>) 
				{
					func(thread.data());																	// 如果可以，就把线程指针传进去（适合需要获取线程上下文的任务）
				}
				else
				{
					func();																					// 如果不可以，就直接执行（适合像 TCPMgr::Instance() 这种任务）
				}
			}, Qt::QueuedConnection);
	}

private:
	QSharedPointer<WorkerThread> GetThread();
	void InitNetwork();																						// 初始化网络相关的变量

private:
	size_t threadNum_ = 0;
	int currentThreadIndex_ = 0;
	QMutex mutex_;
	bool started_ = false;
	QVector<QSharedPointer<WorkerThread>> threads_;

private:
	QSharedPointer<TCPMgr> tcp_mgr_;																		// Tcp
	QSharedPointer<UdpManager> udp_manager_;																// Udp
};

#endif // THREADPOOL_H