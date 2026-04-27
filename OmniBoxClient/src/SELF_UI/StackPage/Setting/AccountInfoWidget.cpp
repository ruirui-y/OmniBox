#include "AccountInfoWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>

AccountInfoWidget::AccountInfoWidget(QWidget* parent)
	: QWidget(parent)
{
	BuildUI();
	BindSignals();
}

AccountInfoWidget::~AccountInfoWidget()
{
}

void AccountInfoWidget::BuildUI()
{
	// ==========================================================
	// 1. 外层布局：利用弹簧把面板挤到正中间
	// ==========================================================
	QVBoxLayout* root = new QVBoxLayout(this);
	root->setContentsMargins(0, 0, 0, 0);

	QWidget* centerContainer = new QWidget(this);
	QHBoxLayout* centerLayout = new QHBoxLayout(centerContainer);
	centerLayout->setContentsMargins(0, 0, 0, 0);

	// 核心悬浮面板
	QWidget* panelWidget = new QWidget(centerContainer);
	panelWidget->setObjectName("accountPanel");
	panelWidget->setFixedSize(700, 320);													// 锁死面板尺寸

	centerLayout->addStretch(1);
	centerLayout->addWidget(panelWidget);
	centerLayout->addStretch(1);

	root->addStretch(1);
	root->addWidget(centerContainer);
	root->addStretch(1);

	// ==========================================================
	// 2. 面板内部结构
	// ==========================================================
	QVBoxLayout* panelLayout = new QVBoxLayout(panelWidget);
	panelLayout->setContentsMargins(2, 2, 2, 2);
	panelLayout->setSpacing(0);

	// --- 顶部标题栏 ---
	QLabel* titleLabel = new QLabel(u8"账户信息", panelWidget);
	titleLabel->setObjectName("accountTitle");
	titleLabel->setAlignment(Qt::AlignCenter);
	titleLabel->setFixedHeight(40);
	panelLayout->addWidget(titleLabel);

	// --- 提取一个行容器创建的 Lambda 辅助函数 ---
	auto createRow = [&](int height) -> QWidget* {
		QWidget* row = new QWidget(panelWidget);
		row->setObjectName("accountRow");
		row->setFixedHeight(height);
		return row;
		};

	// --- 第一行：语言选项 ---
	QWidget* row1 = createRow(50);
	QHBoxLayout* l1 = new QHBoxLayout(row1);
	l1->setContentsMargins(40, 0, 40, 0);

	QLabel* langLabel = new QLabel(u8"语言选项", row1);
	langLabel->setObjectName("accountText");
	langLabel->setFixedWidth(120);
	langLabel->setAlignment(Qt::AlignCenter);

	m_langCombo = new QComboBox(row1);
	m_langCombo->setObjectName("accountCombo");
	m_langCombo->addItem("ZH");
	m_langCombo->addItem("EN");
	m_langCombo->setFixedSize(100, 30);

	l1->addWidget(langLabel);
	l1->addSpacing(20);
	l1->addWidget(m_langCombo);
	l1->addStretch(1);
	panelLayout->addWidget(row1);

	// --- 第二行：账户 ---
	QWidget* row2 = createRow(50);
	QHBoxLayout* l2 = new QHBoxLayout(row2);
	l2->setContentsMargins(40, 0, 40, 0);

	QLabel* icon1 = new QLabel(u8"👤", row2);												// 占位图标，后续可用QSS换图
	icon1->setObjectName("accountIcon");
	QLabel* accLabel = new QLabel(u8"账户", row2);
	accLabel->setObjectName("accountText");
	accLabel->setFixedWidth(90);

	QLabel* accValue = new QLabel("NoNet", row2);
	accValue->setObjectName("accountText");
	accValue->setFixedWidth(100);

	m_logoutBtn = new QPushButton(u8"退出", row2);
	m_logoutBtn->setObjectName("accountBtn");
	m_logoutBtn->setFixedSize(80, 30);
	m_logoutBtn->setCursor(Qt::PointingHandCursor);

	l2->addWidget(icon1);
	l2->addWidget(accLabel);
	l2->addSpacing(20);
	l2->addWidget(accValue);
	l2->addSpacing(40);
	l2->addWidget(m_logoutBtn);
	l2->addStretch(1);
	panelLayout->addWidget(row2);

	// --- 第三行：运营模式 ---
	QWidget* row3 = createRow(50);
	QHBoxLayout* l3 = new QHBoxLayout(row3);
	l3->setContentsMargins(40, 0, 40, 0);

	QLabel* icon2 = new QLabel(u8"👥", row3);
	icon2->setObjectName("accountIcon");
	QLabel* modeLabel = new QLabel(u8"运营模式", row3);
	modeLabel->setObjectName("accountText");
	modeLabel->setFixedWidth(90);

	QLabel* modeVal1 = new QLabel(u8"利润分成", row3);
	modeVal1->setObjectName("accountText");
	modeVal1->setFixedWidth(100);

	QLabel* modeVal2 = new QLabel(u8"充值点数", row3);
	modeVal2->setObjectName("accountText");
	modeVal2->setFixedWidth(100);

	QLabel* modeVal3 = new QLabel(u8"人均扣除点数: 1", row3);
	modeVal3->setObjectName("accountText");

	l3->addWidget(icon2);
	l3->addWidget(modeLabel);
	l3->addSpacing(20);
	l3->addWidget(modeVal1);
	l3->addSpacing(20);
	l3->addWidget(modeVal2);
	l3->addStretch(1);
	l3->addWidget(modeVal3);
	panelLayout->addWidget(row3);

	// --- 第四行：最低电量 ---
	QWidget* row4 = createRow(50);
	QHBoxLayout* l4 = new QHBoxLayout(row4);
	l4->setContentsMargins(40, 0, 40, 0);

	QLabel* icon3 = new QLabel(u8"⚡", row4);
	icon3->setObjectName("accountIcon");
	QLabel* batLabel = new QLabel(u8"设备使用的最低电量(0~100)", row4);
	batLabel->setObjectName("accountText");

	m_batteryEdit = new QLineEdit("0", row4);
	m_batteryEdit->setObjectName("accountInput");
	m_batteryEdit->setFixedSize(80, 30);

	m_confirmBtn = new QPushButton(u8"确定", row4);
	m_confirmBtn->setObjectName("accountBtn");
	m_confirmBtn->setFixedSize(80, 30);
	m_confirmBtn->setCursor(Qt::PointingHandCursor);

	l4->addWidget(icon3);
	l4->addWidget(batLabel);
	l4->addStretch(1);
	l4->addWidget(m_batteryEdit);
	l4->addSpacing(40);
	l4->addWidget(m_confirmBtn);
	panelLayout->addWidget(row4);

	// --- 底部翻页栏占位 ---
	QWidget* bottomBar = new QWidget(panelWidget);
	bottomBar->setFixedHeight(40);
	QHBoxLayout* bottomLayout = new QHBoxLayout(bottomBar);

	QPushButton* leftArrow = new QPushButton(u8"◀", bottomBar);
	QPushButton* rightArrow = new QPushButton(u8"▶", bottomBar);
	leftArrow->setObjectName("accountArrowBtn");
	rightArrow->setObjectName("accountArrowBtn");

	bottomLayout->addStretch(1);
	bottomLayout->addWidget(leftArrow);
	bottomLayout->addSpacing(40);
	bottomLayout->addWidget(rightArrow);
	bottomLayout->addStretch(1);

	panelLayout->addWidget(bottomBar);
	panelLayout->addStretch(1);
}

void AccountInfoWidget::BindSignals()
{
	connect(m_logoutBtn, &QPushButton::clicked, this, []() {
		// 退出逻辑
		});

	connect(m_confirmBtn, &QPushButton::clicked, this, [this]() {
		// 保存电量逻辑
		});
}