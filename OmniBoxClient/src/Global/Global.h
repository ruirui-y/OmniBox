#ifndef GLOBAL_H
#define GLOBAL_H
#include <QWidget>
#include <functional>
#include <QRegularExpression>
#include "QStyle"

#include <memory>
#include <iostream>
#include <mutex>

#include <QString>
#include <QSettings>
#include <vector>

using namespace std;


// extern 声明此变量是在其他文件中定义的全局变量
extern function<void(QWidget*)> repolish;

extern function<QString(QString)> xorString;			                                                        // 密码加密

extern QString ConfigPath;								                                                        // 配置文件路径
extern QSettings* ConfigSettings;						                                                        // 配置文件对象

extern QString ClientConfigPath;                                                                                // 客户端配置文件路径
extern QString LoginConfigPath;                                                                                 // 登录配置文件路径

extern const int tipOffset;								                                                        // 提示框偏移量

using DeferFunc = std::function<void()>;
class Defer
{
public:
    Defer(DeferFunc func) : m_func(func) {}
    ~Defer() { m_func(); }
private:
    DeferFunc m_func;
};
#endif // GLOBAL_H