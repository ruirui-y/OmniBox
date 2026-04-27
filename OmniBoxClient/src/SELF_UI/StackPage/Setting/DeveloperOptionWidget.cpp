#include "DeveloperOptionWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include "DeviceRowWidget.h"
#include <qDebug>

// =========================================================================================
// [主窗口]：DeveloperOptionWidget
// =========================================================================================
DeveloperOptionWidget::DeveloperOptionWidget(QWidget* parent)
	: QWidget(parent)
{
	BuildUI();
	BindSignals();
}

DeveloperOptionWidget::~DeveloperOptionWidget()
{
}

void DeveloperOptionWidget::BuildUI()
{
	QVBoxLayout* root = new QVBoxLayout(this);
	root->setContentsMargins(20, 20, 20, 20);
	root->setSpacing(15);

	// ==========================================================
	// 1. 顶部表头 (说明每一列是什么)
	// ==========================================================
	QWidget* headerWidget = new QWidget(this);
	headerWidget->setObjectName("devHeader");
	headerWidget->setFixedHeight(40);

	QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
	headerLayout->setContentsMargins(20, 0, 20, 0);
	headerLayout->setSpacing(20);

	QLabel* hName = new QLabel(u8"设备昵称", headerWidget);
	hName->setFixedWidth(150);
	hName->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

	QLabel* hStatus = new QLabel(u8"当前状态", headerWidget);
	hStatus->setFixedWidth(100);
	hStatus->setAlignment(Qt::AlignCenter);

	QLabel* hOperate = new QLabel(u8"设备操作", headerWidget);
	hOperate->setFixedWidth(180);															// 覆盖两个按钮的宽度
	hOperate->setAlignment(Qt::AlignCenter);

	headerLayout->addWidget(hName);
	headerLayout->addWidget(hStatus);
	headerLayout->addStretch(1);
	headerLayout->addWidget(hOperate);

	// 所有表头文字统一样式
	for (auto* label : headerWidget->findChildren<QLabel*>()) {
		label->setStyleSheet("color: #00e5ff; font-weight: bold; font-size: 15px;");
	}

	root->addWidget(headerWidget);

	// ==========================================================
	// 2. 中间滚动列表区
	// ==========================================================
	QScrollArea* scrollArea = new QScrollArea(this);
	scrollArea->setObjectName("devScrollArea");
	scrollArea->setWidgetResizable(true);													// 允许内部 Widget 随滚动区拉伸
	scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);						// 关掉横向滚动条

	QWidget* listContainer = new QWidget(scrollArea);
	listContainer->setObjectName("devListContainer");
	m_listLayout = new QVBoxLayout(listContainer);
	m_listLayout->setContentsMargins(0, 0, 0, 0);
	m_listLayout->setSpacing(0);															// 行与行无缝拼接

	// 循环创建 20 个设备行
	for (int i = 0; i < 20; ++i)
	{
		int deviceId = i + 1;
		QString deviceName = (deviceId <= 18) ? "Player" : "ScorePro";

		// 传入 i % 2 == 0 来判断奇偶，生成斑马线
		DeviceRowWidget* row = new DeviceRowWidget(deviceId, deviceName, i % 2 == 0, listContainer);
		m_listLayout->addWidget(row);

		// 💡 外部捕获信号的演示
		connect(row, &DeviceRowWidget::nameChanged, this, [](int id, const QString& newName) {
			// 在这里调用数据库或网络，通知后台更新名字！
			qDebug() << "设备" << id << "改名为:" << newName;
			});

		connect(row, &DeviceRowWidget::wakeClicked, this, [](int id) {
			qDebug() << "请求唤醒设备" << id;
			// row->setDeviceStatus(true); // 收到网络反馈后调用这句更新UI状态
			});
		m_listLayout->addWidget(row);
	}
	m_listLayout->addStretch(1);															// 底部加上弹簧，防止行数少时被拉长

	scrollArea->setWidget(listContainer);
	root->addWidget(scrollArea, 1);															// 滚动区占据主要高度

	// ==========================================================
	// 3. 底部全局控制栏
	// ==========================================================
	QWidget* bottomBar = new QWidget(this);
	QHBoxLayout* bottomLayout = new QHBoxLayout(bottomBar);
	bottomLayout->setContentsMargins(0, 10, 0, 0);
	bottomLayout->setSpacing(20);

	QPushButton* wakeAllBtn = new QPushButton(u8"▶ 设备全部唤醒 ◀", bottomBar);
	wakeAllBtn->setObjectName("devGlobalBtn");
	wakeAllBtn->setCursor(Qt::PointingHandCursor);

	QPushButton* sleepAllBtn = new QPushButton(u8"▶ 设备全部睡眠 ◀", bottomBar);
	sleepAllBtn->setObjectName("devGlobalBtn");
	sleepAllBtn->setCursor(Qt::PointingHandCursor);

	// 两个按钮居中显示
	bottomLayout->addStretch(1);
	bottomLayout->addWidget(wakeAllBtn);
	bottomLayout->addWidget(sleepAllBtn);
	bottomLayout->addStretch(1);

	root->addWidget(bottomBar);
}

void DeveloperOptionWidget::BindSignals()
{
	// 后续在这里绑定批量或单体唤醒、睡眠的信号
}