#include "SettingWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QButtonGroup>
#include <QStackedWidget>
#include <QLabel>
#include "ParamSettingWidget.h"
#include "DeveloperOptionWidget.h"
#include "AccountInfoWidget.h"

SettingWidget::SettingWidget(QWidget* parent)
	: QWidget(parent)
{
	BuildUI();
	BindSignals();
}

SettingWidget::~SettingWidget()
{
}

// ---------------------------------------------------------
// 模块 1：构建纯 UI 结构
// ---------------------------------------------------------
void SettingWidget::BuildUI()
{
	QHBoxLayout* root = new QHBoxLayout(this);
	root->setContentsMargins(0, 0, 0, 0);
	root->setSpacing(0);

	// ==========================================================
	// 1. 左侧导航栏
	// ==========================================================
	QWidget* leftNavWidget = new QWidget(this);
	leftNavWidget->setObjectName("settingLeftNav");
	leftNavWidget->setFixedWidth(160);														// 设定固定宽度

	QVBoxLayout* leftNavLayout = new QVBoxLayout(leftNavWidget);
	leftNavLayout->setContentsMargins(15, 40, 15, 40);
	leftNavLayout->setSpacing(30);															// 按钮之间的垂直间距

	m_navGroup = new QButtonGroup(this);
	m_navGroup->setExclusive(true);															// 互斥，单选

	// 生成导航按钮
	auto createNavBtn = [&](const QString& text, int id) -> QPushButton* {
		QPushButton* btn = new QPushButton(text, leftNavWidget);
		btn->setObjectName("settingNavBtn");
		btn->setCheckable(true);
		btn->setCursor(Qt::PointingHandCursor);
		m_navGroup->addButton(btn, id);
		return btn;
		};

	QPushButton* paramBtn = createNavBtn(u8"参数设置", 0);
	QPushButton* devBtn = createNavBtn(u8"开发者选项", 1);
	QPushButton* accBtn = createNavBtn(u8"账户信息", 2);

	leftNavLayout->addWidget(paramBtn);
	leftNavLayout->addWidget(devBtn);
	leftNavLayout->addStretch(1);															// 弹簧：把账户信息顶到最下面
	leftNavLayout->addWidget(accBtn);

	paramBtn->setChecked(true);																// 默认选中第一个

	// ==========================================================
	// 2. 右侧堆栈窗口 (管理你的三个子页面)
	// ==========================================================
	m_stackedWidget = new QStackedWidget(this);
	m_stackedWidget->setObjectName("settingStackedWidget");

	ParamSettingWidget* paramWidget = new ParamSettingWidget(this);							// 0. 参数设置
	DeveloperOptionWidget* devWidget = new DeveloperOptionWidget(this);						// 1. 开发者选项

	AccountInfoWidget* accWidget = new AccountInfoWidget(this);								// 2. 账户信息
	// --------------------------------------------------------

	// 严格按顺序塞入堆栈
	m_stackedWidget->addWidget(paramWidget);												// Index 0
	m_stackedWidget->addWidget(devWidget);													// Index 1
	m_stackedWidget->addWidget(accWidget);													// Index 2

	// ==========================================================
	// 3. 将左右部件装入根布局
	// ==========================================================
	root->addWidget(leftNavWidget);
	root->addWidget(m_stackedWidget, 1);													// 1 代表占据右侧剩余全部空间
}

// ---------------------------------------------------------
// 模块 2：信号与槽绑定
// ---------------------------------------------------------
void SettingWidget::BindSignals()
{
	// 绑定左侧按钮组点击，自动切换右侧堆栈窗口页面 (Qt5 兼容写法)
	connect(m_navGroup, QOverload<int>::of(&QButtonGroup::buttonClicked),
		m_stackedWidget, &QStackedWidget::setCurrentIndex);
}