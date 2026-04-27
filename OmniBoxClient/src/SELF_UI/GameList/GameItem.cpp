#include "GameItem.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QIcon>
#include <QMouseEvent>
#include "Macro.h"

static  QString Style(R"(
QToolButton {
    color: #EDEDED;
    background: transparent;
    border: none;
    padding: 4px 5px;

    border-left: 4px solid transparent;			/* 左侧选中条（默认透明） */
	spacing: 10px;								/* 图标与文字间距 */
	qproperty-iconSize: 40px 40px;				/* 图标大小 */
}

/* 悬浮 */
QToolButton:hover {
    background: rgba(255,255,255,0.06);
    border-left-color: rgba(255,255,255,0.25);
}

/* 按下 */
QToolButton:pressed {
    background: rgba(255,255,255,0.12);
}

/* 选中 */
QToolButton:checked {
    background: rgba(255,255,255,0.10);
    border-left-color: #FFFFFF;				/* 左侧白色高亮条 */
    color: #FFFFFF;
})");

GameItem::GameItem(QString name, QString iconPath, QWidget* parent)
	: m_name(name), m_iconPath(iconPath), QToolButton(parent)
{
	BuildUI();
	setCursor(Qt::PointingHandCursor);
	setStyleSheet(Style);
}

GameItem::~GameItem()
{}

QSize GameItem::sizeHint() const
{
	return QSize(HZ_LIST_WIDTH, 60);
}

void GameItem::BuildUI()
{
	setText(m_name);
	setIcon(QIcon(m_iconPath));
	setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	setCheckable(true);
	setAutoExclusive(true);																		// 同父级目录下互斥选中
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}