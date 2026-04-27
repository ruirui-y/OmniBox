#include "DeviceRowWidget.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

DeviceRowWidget::DeviceRowWidget(int id, const QString& name, bool isEven, QWidget* parent)
	: QWidget(parent), m_id(id)
{
	// 奇偶行交替赋名，实现斑马线效果
	setObjectName(isEven ? "devRowEven" : "devRowOdd");
	setFixedHeight(45);

	QHBoxLayout* rowLayout = new QHBoxLayout(this);
	rowLayout->setContentsMargins(20, 0, 20, 0);
	rowLayout->setSpacing(20);

	// ==========================================================
	// 1. 第一列：固定的设备编号 + 可编辑的昵称
	// ==========================================================
	QWidget* nameWidget = new QWidget(this);
	nameWidget->setFixedWidth(150);															// 依然锁死整列宽度
	QHBoxLayout* nameLayout = new QHBoxLayout(nameWidget);
	nameLayout->setContentsMargins(0, 0, 0, 0);
	nameLayout->setSpacing(5);

	// 固定的编号部分 (如 "01 - ")
	QString idStr = QString("%1 - ").arg(id, 2, 10, QChar('0'));
	QLabel* idLabel = new QLabel(idStr, nameWidget);
	idLabel->setObjectName("devRowText");

	// 动态可编辑的名称部分
	m_nameEdit = new QLineEdit(name, nameWidget);
	m_nameEdit->setObjectName("devRowNameEdit");

	nameLayout->addWidget(idLabel);
	nameLayout->addWidget(m_nameEdit, 1);
	rowLayout->addWidget(nameWidget);

	// ==========================================================
	// 2. 第二列：动态状态
	// ==========================================================
	m_statusLabel = new QLabel(u8"● 唤醒", this);
	m_statusLabel->setObjectName("devRowStatus");
	m_statusLabel->setFixedWidth(100);
	m_statusLabel->setAlignment(Qt::AlignCenter);
	rowLayout->addWidget(m_statusLabel);

	rowLayout->addStretch(1);

	// ==========================================================
	// 3. 第三、四列：操作按钮
	// ==========================================================
	QPushButton* wakeBtn = new QPushButton(u8"唤醒", this);
	wakeBtn->setObjectName("devRowBtnWake");
	wakeBtn->setFixedSize(80, 28);
	wakeBtn->setCursor(Qt::PointingHandCursor);
	rowLayout->addWidget(wakeBtn);

	QPushButton* sleepBtn = new QPushButton(u8"睡眠", this);
	sleepBtn->setObjectName("devRowBtnSleep");
	sleepBtn->setFixedSize(80, 28);
	sleepBtn->setCursor(Qt::PointingHandCursor);
	rowLayout->addWidget(sleepBtn);

	// ==========================================================
	// 4. 信号与槽绑定 (核心逻辑)
	// ==========================================================

	// 当用户在输入框里敲击回车，或者点击其他地方失去焦点时触发
	connect(m_nameEdit, &QLineEdit::editingFinished, this, [this]() {
		// 消除焦点，并把新名字连带 ID 发射出去
		m_nameEdit->clearFocus();
		emit nameChanged(m_id, m_nameEdit->text());
		});

	// 按钮点击发射对应设备的 ID
	connect(wakeBtn, &QPushButton::clicked, this, [this]() {
		emit wakeClicked(m_id);
		});
	connect(sleepBtn, &QPushButton::clicked, this, [this]() {
		emit sleepClicked(m_id);
		});
}

// 供外部中控调用的更新函数：改名
void DeviceRowWidget::setDeviceName(const QString& name)
{
	m_nameEdit->setText(name);
}

// 供外部中控调用的更新函数：切状态
void DeviceRowWidget::setDeviceStatus(bool isAwake)
{
	if (isAwake) {
		m_statusLabel->setText(u8"● 唤醒");
		m_statusLabel->setStyleSheet("color: #00ffaa;");									// 亮绿色
	}
	else {
		m_statusLabel->setText(u8"○ 睡眠");
		m_statusLabel->setStyleSheet("color: #aaaaaa;");									// 灰暗色
	}
}