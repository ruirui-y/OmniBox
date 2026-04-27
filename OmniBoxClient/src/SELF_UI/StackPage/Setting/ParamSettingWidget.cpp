#include "ParamSettingWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>

// =========================================================================================
// [内部辅助组件]：ParamCard - 参数卡片
// =========================================================================================
class ParamCard : public QWidget
{
public:
	ParamCard(const QString& title, const QString& bottomText, QWidget* parent = nullptr)
		: QWidget(parent), m_rowIndex(0)
	{
		setObjectName("paramCard");
		QVBoxLayout* mainLayout = new QVBoxLayout(this);
		mainLayout->setContentsMargins(2, 2, 2, 2);
		mainLayout->setSpacing(0);

		// 1. 顶部左上角标题 (如 "规则", "武器")
		QLabel* titleLabel = new QLabel(title, this);
		titleLabel->setObjectName("cardTitle");
		mainLayout->addWidget(titleLabel, 0, Qt::AlignLeft);

		// 2. 中间表单容器
		QWidget* formWidget = new QWidget(this);
		m_formLayout = new QVBoxLayout(formWidget);
		m_formLayout->setContentsMargins(10, 10, 10, 10);
		m_formLayout->setSpacing(2);												// 行与行的微小间距
		mainLayout->addWidget(formWidget, 1);
		mainLayout->addStretch(1);													// 底部推平

		// 3. 底部切换栏
		QWidget* bottomBar = new QWidget(this);
		bottomBar->setObjectName("cardBottomBar");
		bottomBar->setFixedHeight(35);
		QHBoxLayout* bottomLayout = new QHBoxLayout(bottomBar);
		bottomLayout->setContentsMargins(10, 0, 10, 0);

		QPushButton* leftBtn = new QPushButton(u8"◀", bottomBar);
		leftBtn->setObjectName("arrowBtn");
		leftBtn->setCursor(Qt::PointingHandCursor);

		QLabel* bottomLabel = new QLabel(bottomText, bottomBar);
		bottomLabel->setObjectName("cardBottomText");
		bottomLabel->setAlignment(Qt::AlignCenter);

		QPushButton* rightBtn = new QPushButton(u8"▶", bottomBar);
		rightBtn->setObjectName("arrowBtn");
		rightBtn->setCursor(Qt::PointingHandCursor);

		bottomLayout->addStretch(1);
		bottomLayout->addWidget(leftBtn);
		bottomLayout->addWidget(bottomLabel);
		bottomLayout->addWidget(rightBtn);
		bottomLayout->addStretch(1);

		mainLayout->addWidget(bottomBar);
	}

	// 💡 技能树点亮：固定列宽精准瘦身，防止超出父容器导致挤压重叠
	void addRow(const QString& labelText, const QString& val, const QString& unitText = "", bool hasCheckbox = false, const QString& cbText = "")
	{
		QWidget* rowWidget = new QWidget(this);
		rowWidget->setObjectName(m_rowIndex % 2 == 0 ? "rowWidgetEven" : "rowWidgetOdd");
		rowWidget->setFixedHeight(35);

		QHBoxLayout* rowLayout = new QHBoxLayout(rowWidget);
		rowLayout->setContentsMargins(0, 0, 0, 0);
		rowLayout->setSpacing(0);																	// 彻底清空默认间距，完全由我们手动掌控

		// 1. 左侧放一个弹簧，把整个内容往中间挤
		rowLayout->addSpacing(10);

		// 2. 属性名 (右对齐)
		QLabel* label = new QLabel(labelText, rowWidget);
		label->setObjectName("paramLabel");
		label->setFixedWidth(110);																	// 瘦身到 95px (刚好够装下最长的7个字)
		label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		rowLayout->addWidget(label);

		rowLayout->addSpacing(8);																	// 手动控制间距

		// 3. 复选框区域 
		if (hasCheckbox) {
			QCheckBox* cb = new QCheckBox(cbText, rowWidget);
			cb->setObjectName("paramCheck");
			cb->setFixedWidth(80);
			rowLayout->addWidget(cb);
		}
		else {
			// 如果没有复选框，放一个空气墙占位
			QLabel* spacer = new QLabel(rowWidget);
			spacer->setFixedWidth(80);
			rowLayout->addWidget(spacer);
		}

		rowLayout->addStretch(1);																	// 给输入框前面留点呼吸感

		// 4. 输入框 (居中对齐)
		QLineEdit* edit = new QLineEdit(val, rowWidget);
		edit->setObjectName("paramInput");
		edit->setFixedWidth(50);																	// 瘦身到 50px (输入几位数字足够了)
		edit->setAlignment(Qt::AlignCenter);
		rowLayout->addWidget(edit);

		rowLayout->addSpacing(8);

		// 5. 单位 (左对齐)
		QLabel* unitLabel = new QLabel(unitText, rowWidget);
		unitLabel->setObjectName("paramUnit");
		unitLabel->setFixedWidth(30);																// 瘦身到 30px
		unitLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		rowLayout->addWidget(unitLabel);

		// 6. 右侧再放一个弹簧
		rowLayout->addSpacing(10);

		m_formLayout->addWidget(rowWidget);
		m_rowIndex++;
	}

private:
	QVBoxLayout* m_formLayout;
	int m_rowIndex;
};

// =========================================================================================
// [主窗口]：ParamSettingWidget
// =========================================================================================
ParamSettingWidget::ParamSettingWidget(QWidget* parent)
	: QWidget(parent)
{
	BuildUI();
}

ParamSettingWidget::~ParamSettingWidget()
{
}

void ParamSettingWidget::BuildUI()
{
	m_gridLayout = new QGridLayout(this);
	m_gridLayout->setContentsMargins(20, 20, 20, 20);
	m_gridLayout->setSpacing(20);

	// --- 1. 左上：死斗/道具模式 - 规则 ---
	ParamCard* card1 = new ParamCard(u8"规则", u8"死斗/道具模式", this);
	card1->addRow(u8"时长", "100", u8"分", true, u8"不限时");
	card1->addRow(u8"教学时长", "20", u8"秒");
	card1->addRow(u8"玩家血量", "100", u8"HP");
	card1->addRow(u8"复活时间", "10", u8"秒");
	card1->addRow(u8"击中外置靶得分", "50", u8"分");
	card1->addRow(u8"占塔得分", "500", u8"分");

	// --- 2. 右上：死斗/道具模式 - 武器 ---
	ParamCard* card2 = new ParamCard(u8"武器", u8"死斗/道具模式", this);
	card2->addRow(u8"手枪伤害数值", "10");
	card2->addRow(u8"冲锋枪伤害数值", "20");
	card2->addRow(u8"击杀玩家得分", "100");

	// --- 3. 左下：团队模式 - 规则 ---
	ParamCard* card3 = new ParamCard(u8"规则", u8"团队模式", this);
	card3->addRow(u8"时长", "100", u8"分");
	card3->addRow(u8"玩家血量", "100", u8"HP");
	card3->addRow(u8"复活时间", "3", u8"秒");
	card3->addRow(u8"占塔得分", "500", u8"分");

	// --- 4. 右下：团队模式 - 武器 ---
	ParamCard* card4 = new ParamCard(u8"武器", u8"团队模式", this);
	card4->addRow(u8"冲锋枪伤害数值", "20");
	card4->addRow(u8"击杀玩家得分", "100");
	card4->addRow(u8"击中外置靶得分", "50");
	card4->addRow(u8"手枪伤害数值", "10");

	// 加入网格
	m_gridLayout->addWidget(card1, 0, 0);
	m_gridLayout->addWidget(card2, 0, 1);
	m_gridLayout->addWidget(card3, 1, 0);
	m_gridLayout->addWidget(card4, 1, 1);
}