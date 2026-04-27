#ifndef IMAGE_BG_BUTTON_H
#define IMAGE_BG_BUTTON_H

#include <QPushButton>

class ImageBgButton : public QPushButton {
    Q_OBJECT
public:
    explicit ImageBgButton(const QString& text, const QString& img, QWidget* p = nullptr)
        : QPushButton(text, p), bg(img) 
    {
    }
    void setBackground(const QString& img) { bg = QPixmap(img); update(); }
protected:
    void paintEvent(QPaintEvent*) override;
private:
    QPixmap bg;
};

#endif