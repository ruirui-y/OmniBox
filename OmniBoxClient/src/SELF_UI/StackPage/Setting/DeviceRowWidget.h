#ifndef DEVICEROWWIDGET_H
#define DEVICEROWWIDGET_H

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;

class DeviceRowWidget : public QWidget
{
	Q_OBJECT

public:
	explicit DeviceRowWidget(int id, const QString& name, bool isEven, QWidget* parent = nullptr);

	// --- 提供给外部调用的动态更新接口 ---
	void setDeviceName(const QString& name);
	void setDeviceStatus(bool isAwake);

signals:
	// --- 向外发送的交互信号 ---
	void nameChanged(int id, const QString& newName);										// 昵称修改完毕信号
	void wakeClicked(int id);																// 点击唤醒信号
	void sleepClicked(int id);																// 点击睡眠信号

private:
	int m_id;																				// 记录当前行绑定的设备ID
	QLineEdit* m_nameEdit;																	// 可编辑的昵称框
	QLabel* m_statusLabel;																	// 状态文本
};

#endif // DEVICEROWWIDGET_H