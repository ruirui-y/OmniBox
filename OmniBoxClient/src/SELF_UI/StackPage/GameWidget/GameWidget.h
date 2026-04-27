#ifndef GAME_WIDGET_H
#define GAME_WIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QProcess>
#include <QFrame>

class GameWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GameWidget(QWidget* parent = nullptr);                     // 构造函数
    ~GameWidget();                                                      // 析构函数

private:
    void BuildUI();                                                     // 构建前端界面
    void LoadStyleSheet();                                              // 加载外部样式表
    void AddGame(const QString& name, const QString& timeStr);          // 添加游戏项的工厂

private slots:
    void onGameSelected(QListWidgetItem* current, QListWidgetItem* pre);// 处理卡片选中高亮
    void onPlayClicked();                                               // 全局启动按钮槽
    void onStopClicked();                                               // 全局停止按钮槽
    void onGameProcessFinished();                                       // 游戏进程结束槽

private:
    QListWidget* m_listWidget;                                          // 核心展示列表
    QPushButton* m_btnPlay;                                             // 全局启动大按钮
    QPushButton* m_btnStop;                                             // 全局停止大按钮

    QProcess* m_gameProcess;                                            // 当前运行的游戏进程
    QString       m_selectedGameName;                                   // 记录当前选中的游戏
};

#endif // GAMELAUNCHER_H