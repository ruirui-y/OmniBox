#include "GameCenterWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QDebug>
#include <QGuiApplication>
#include <QScreen>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QMediaContent> // Qt5 专属
#include <QUrl>
#include <QPixmap>

GameCenterWidget::GameCenterWidget(QWidget* parent)
	: QWidget(parent)
{
	BuildUI();
	InitVideoPlayer();
	BindSignals();
}

GameCenterWidget::~GameCenterWidget()
{
	if (m_secondScreenWindow) {
		m_secondScreenWindow->deleteLater();
	}
}

void GameCenterWidget::resizeEvent(QResizeEvent* event)
{
	// qDebug() << "GameCenterWidget::resizeEvent size() = " << size();
}

// ---------------------------------------------------------
// 模块 1：只负责画界面
// ---------------------------------------------------------
void GameCenterWidget::BuildUI()
{
	QVBoxLayout* root = new QVBoxLayout(this);
	root->setContentsMargins(20, 20, 20, 20);
	root->setSpacing(20);

	// =====================================
	// 顶部：控制面板
	// =====================================
	m_controlPanel = new QWidget(this);
	m_controlPanel->setObjectName("videoControlPanel");
	QHBoxLayout* controlLayout = new QHBoxLayout(m_controlPanel);
	controlLayout->setContentsMargins(0, 0, 0, 0);

	QLabel* infoLabel = new QLabel(u8"宣传视频控制台", m_controlPanel);
	infoLabel->setObjectName("videoInfoLabel");

	m_playBtn = new QPushButton(u8"▶ 在副屏播放", m_controlPanel);
	m_playBtn->setObjectName("videoPlayBtn");
	m_playBtn->setCursor(Qt::PointingHandCursor);

	m_stopBtn = new QPushButton(u8"■ 停止播放", m_controlPanel);
	m_stopBtn->setObjectName("videoStopBtn");
	m_stopBtn->setCursor(Qt::PointingHandCursor);

	controlLayout->addWidget(infoLabel);
	controlLayout->addStretch(1);
	controlLayout->addWidget(m_playBtn);
	controlLayout->addWidget(m_stopBtn);

	// =====================================
	// 下方：巨幅海报展示区
	// =====================================
	m_posterLabel = new QLabel(this);
	m_posterLabel->setObjectName("videoPosterLabel");
	m_posterLabel->setAlignment(Qt::AlignCenter);

	// ⚠️ 记得替换为你真实的海报路径
	QPixmap posterPixmap(":/MiNi/Images/MiNiWorld/4.png");
	m_posterLabel->setPixmap(posterPixmap);
	m_posterLabel->setScaledContents(true); // 自动拉伸填满红框区域

	// 组装
	root->addWidget(m_controlPanel);
	root->addWidget(m_posterLabel, 1); // 加上比例 1，让海报暴力霸占下方所有剩余空间
}

// ---------------------------------------------------------
// 模块 2：只负责初始化播放引擎
// ---------------------------------------------------------
void GameCenterWidget::InitVideoPlayer()
{
	m_secondScreenWindow = new QWidget();
	m_secondScreenWindow->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
	m_secondScreenWindow->setStyleSheet("background-color: black;"); // 唯一保留的内联样式，防副屏刺眼

	QVBoxLayout* videoLayout = new QVBoxLayout(m_secondScreenWindow);
	videoLayout->setContentsMargins(0, 0, 0, 0);

	m_videoWidget = new QVideoWidget(m_secondScreenWindow);
	videoLayout->addWidget(m_videoWidget);

	m_player = new QMediaPlayer(this);
	m_player->setVideoOutput(m_videoWidget);
	m_player->setVolume(100);

	// ⚠️ 记得替换为你真实的视频路径
	m_player->setMedia(QMediaContent(QUrl::fromLocalFile("D:/TestVideo.mp4")));
}

// ---------------------------------------------------------
// 模块 3：只负责绑定连线
// ---------------------------------------------------------
void GameCenterWidget::BindSignals()
{
	connect(m_playBtn, &QPushButton::clicked, this, &GameCenterWidget::OnPlayClicked);
	connect(m_stopBtn, &QPushButton::clicked, this, &GameCenterWidget::OnStopClicked);
}

// ---------------------------------------------------------
// 模块 4：具体的业务逻辑槽函数
// ---------------------------------------------------------
void GameCenterWidget::OnPlayClicked()
{
	QList<QScreen*> screens = QGuiApplication::screens();

	if (screens.size() > 1) {
		// 投射到第二屏幕
		QRect secondScreenRect = screens.at(1)->geometry();
		m_secondScreenWindow->setGeometry(secondScreenRect);
		m_secondScreenWindow->showFullScreen();
	}
	else {
		// 单屏测试
		qDebug() << u8"未检测到双屏，在主屏幕测试播放！";
		m_secondScreenWindow->resize(800, 600);
		m_secondScreenWindow->show();
	}

	m_player->play();
}

void GameCenterWidget::OnStopClicked()
{
	m_player->stop();
	m_secondScreenWindow->hide();
}