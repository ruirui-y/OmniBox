#ifndef TITLEBAR_H
#define TITLEBAR_H

#include <QWidget>

class QPushButton;

class TitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(QWidget* parent = nullptr);

signals:
    void backRequested();
    void settingsRequested();
    void minimizeRequested();
    void closeRequested();

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;

private:
    void InitBtns();

private:
    QPoint m_dragOffset;                                                                // 鼠标相对窗口左上角的偏移
    QPushButton* m_btnName = nullptr;                                                   // 账号名称按钮
    QPushButton* m_btnMin = nullptr;                                                    // 最小化按钮 (自绘)
    QPushButton* m_btnClose = nullptr;                                                  // 关闭按钮 (自绘)
};

#endif // TITLEBAR_H