#include "Global.h"
#include <QDir>
#include <QCoreApplication>
#include "Macro.h"
#include <QDebug>
#include <QProcess>


// 初始化声明的全局变量
function<void(QWidget*)> repolish = [](QWidget* Widget)
{
	Widget->style()->unpolish(Widget);
	Widget->style()->polish(Widget);
};

/* 密码加密 */
function<QString(QString)> xorString = [](QString input)
	{
		QString result = input;
		int length = input.length();
		length %= 255;
		for (int i = 0; i < length; ++i)
		{
			result[i] = QChar(static_cast<ushort>(input[i].unicode() ^ static_cast<ushort>(length)));
		}
		return result;
	};

// 配置文件路径
QString ConfigPath = QDir::currentPath() + "/Config/Config.ini";

// 管理配置文件对象
QSettings* ConfigSettings = new QSettings(ConfigPath, QSettings::IniFormat);

// 客户端配置文件路径
QString ClientConfigPath = QDir(QCoreApplication::applicationDirPath()).filePath("ClientInstall/Configs/config.json");

// 登录配置文件路径
QString LoginConfigPath = QDir(QCoreApplication::applicationDirPath()).filePath("ClientInstall/Configs/login.json");

const int tipOffset = 5;