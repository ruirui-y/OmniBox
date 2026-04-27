#ifndef CONTROLHUBWINDOW_H
#define CONTROLHUBWINDOW_H

#include <QWidget>
#include <QPaintEvent>
#include <QMap>
#include <QPixmap>

class QStackedLayout;
class TitleBar;
class QListWidget;
class QButtonGroup;

class ControlHubWindow : public QWidget
{
    Q_OBJECT
public:
    explicit ControlHubWindow(QWidget* parent = nullptr);

public:
    TitleBar* GetTitle() { return m_title; }

private:
    void AddGameItem(QString name);                                                     // 添加侧边栏选项

private:
    TitleBar* m_title = nullptr;                                                        // 标题栏
    QStackedLayout* m_stack = nullptr;                                                  // 页面堆栈
    QPixmap m_bgCache;                                                                  // 缓存的背景图

    QListWidget* m_leftList_c = nullptr;                                                // 侧边栏
    QButtonGroup* m_leftList_btns = nullptr;                                            // 侧边栏按钮组
    QMap<QString, QString> m_gameItems;                                                 // 选项名称与图标路径的映射
};

#endif // CONTROLHUBWINDOW_H