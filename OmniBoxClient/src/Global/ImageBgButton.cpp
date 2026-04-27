#include "ImageBgButton.h"
#include <QStyleOptionButton>
#include <QPainter>

void ImageBgButton::paintEvent(QPaintEvent*)
{
    QStyleOptionButton opt; initStyleOption(&opt);

    // 是否按下（Sunken/Down）
    const bool down = (opt.state & QStyle::State_Sunken) || isDown();
    const QPoint offset(0, down ? 2 : 0);  // 下移 2px

    QPainter p(this);
    QRect r = rect().translated(offset);

    // 背景图铺满
    if (!bg.isNull()) {
        p.drawPixmap(r, bg);
    }

    // 可选：画按钮 bevel 等（若需要视觉按压，可以让样式处理）
    style()->drawControl(QStyle::CE_PushButtonBevel, &opt, &p, this);

    // 用 QSS/Palette 的颜色画文字（QSS 会改 palette 的 ButtonText）
    style()->drawItemText(&p, r, Qt::AlignCenter,
        opt.palette, isEnabled(), text(), QPalette::ButtonText);
}