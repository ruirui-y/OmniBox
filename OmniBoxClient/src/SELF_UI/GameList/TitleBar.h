#ifndef TITLEBAR_H
#define TITLEBAR_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>

enum class TitleMode
{
    Login,                                              // 登录模式：显示 Logo，不可点击
    Hub                                                 // 中控模式：显示账户名，点击弹出菜单
};

class TitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(QWidget* parent = nullptr);

    // 核心接口：设置模式
    void SetMode(TitleMode mode);
    // 更新账户名称（仅在 Hub 模式下有效）
    void SetUserName(const QString& name);
    // 设置 Logo 图标（仅在 Login 模式下有效）
    void SetLogo(const QString& iconPath);

signals:
    void minimizeRequested();
    void closeRequested();

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void paintEvent(QPaintEvent* e) override;

private:
    void InitBtns();

private:
    QPoint m_dragOffset;
    TitleMode m_currentMode = TitleMode::Login;

    // 控件
    QPushButton* m_btnLeft = nullptr;  // 左侧内容按钮（复用，通过模式控制样式）
    QPushButton* m_btnMin = nullptr;
    QPushButton* m_btnClose = nullptr;
};

#endif // TITLEBAR_H