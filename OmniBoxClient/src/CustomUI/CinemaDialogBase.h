#ifndef CINIMADIALOGBASE_H
#define CINIMADIALOGBASE_H

#include <QDialog>
#include <QMouseEvent>
#include <QPoint>
#include <QVBoxLayout>
#include <QLabel>

class CinemaDialogBase : public QDialog
{
    Q_OBJECT

public:
    explicit CinemaDialogBase(QWidget* parent = nullptr);
    virtual ~CinemaDialogBase() = default;

    // 子类可以通过这个方法修改标题栏文字
    void SetDialogTitle(const QString& title);

    // 极其重要：子类的所有 UI 控件，都必须塞进这个 Layout 里！
    QVBoxLayout* GetContentLayout() const;

protected:
    // 拖拽事件（已经被基类接管，子类不用再管了）
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void SetupBaseUI();

    bool is_dragging_ = false;
    QPoint drag_start_position_;

    QLabel* lbl_title_;
    QVBoxLayout* content_layout_; // 留给子类的专属容器
};

#endif // CINIMADIALOGBASE_H