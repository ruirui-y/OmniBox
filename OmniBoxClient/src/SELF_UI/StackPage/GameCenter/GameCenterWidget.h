#ifndef GAMECENTERWIDGET_H
#define GAMECENTERWIDGET_H

#include <QWidget>

// 引入多媒体及组件的前置声明
class QMediaPlayer;
class QVideoWidget;
class QLabel;
class QPushButton;

class GameCenterWidget : public QWidget
{
	Q_OBJECT

public:
	explicit GameCenterWidget(QWidget* parent = nullptr);
	~GameCenterWidget();
	virtual void resizeEvent(QResizeEvent* event) override;

private:
	// --- 核心功能模块化拆分 ---
	void BuildUI();														// 纯粹负责搭建界面控件
	void InitVideoPlayer();												// 纯粹负责初始化播放器引擎
	void BindSignals();													// 纯粹负责绑定信号与槽

private slots:
	// --- 具体的动作执行函数 ---
	void OnPlayClicked();												// 播放按钮触发
	void OnStopClicked();												// 停止按钮触发

private:
	// 中控端：UI 组件
	QWidget* m_controlPanel;
	QLabel* m_posterLabel;												// 用于展示下方海报
	QPushButton* m_playBtn;
	QPushButton* m_stopBtn;

	// 播放端：投射到副屏的独立窗口与组件
	QWidget* m_secondScreenWindow = nullptr;
	QVideoWidget* m_videoWidget = nullptr;
	QMediaPlayer* m_player = nullptr;
};

#endif // GAMECENTERWIDGET_H