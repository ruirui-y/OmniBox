#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QThread>
#include "ui_mainWindow.h"

class LoginWidget;
class ControlHubWindow;
class QStackedWidget;

class mainWindow : public QMainWindow
{
    Q_OBJECT

public:
    mainWindow(QWidget *parent = nullptr);
    ~mainWindow();

    void BindSlots();

private slots:
    void SlotSwitchToControlHubWidget();
    void SlotSwitchToLoginWidget();
    void CloseWidget();

private:
    void SetFrameless(bool on);

private:
    Ui::mainWindowClass ui;
    QStackedWidget* m_pages = nullptr;
    LoginWidget* _LoginWidget = nullptr;
    ControlHubWindow* _ControlHub = nullptr;
};

#endif // MAINWINDOW_H