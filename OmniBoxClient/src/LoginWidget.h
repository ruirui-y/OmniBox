#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include <QDialog>
#include "ui_LoginWidget.h"
#include "Enum.h"

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QPixmap;
class QFont;
class TitleBar;

class LoginWidget : public QWidget
{
	Q_OBJECT

public:
	LoginWidget(QWidget* parent = nullptr);
	~LoginWidget();

public:
	bool EnableBtn(bool);
	void ClearInputs();																				// 切换回登录界面清空登录残留信息
	TitleBar* GetTitle(){ return m_title; }

private:
	void BindSlots();
	void BuildUI();																					// 构建UI
	bool CheckUserValid();																			// 检查用户名是否合法
	bool CheckPasswordValid();																		// 检查密码是否合法
	void AutoLogin();																				// 自动登录
	void WriteLoginConfig();																		// 写入登录配置

public slots:
	void OnLoginButtonClicked();																	// 登录按钮被点击
	void slot_tcp_con_finished(bool success);														// TCP连接成功或失败的响应
	void slot_login_failed(int);																	// 登录失败

signals:
	void sig_connect_tcp();																			// 连接服务器

private:
	QMap<TipErr, QString> _TipErrs;																	// 错误提示记录
	QWidget* _Parent;																				// 父窗口

private:
	TitleBar* m_title = nullptr;																// 标题栏
	QWidget* m_panel = nullptr;																		// 中央面板
	QLabel* m_logo = nullptr;

	QLabel* m_userLbl = nullptr;
	QLineEdit* m_userEdit = nullptr;																// 账户

	QLabel* m_passLbl = nullptr;
	QLineEdit* m_passEdit = nullptr;																// 密码

	QCheckBox* m_remember = nullptr;																// 记住密码
	QPushButton* m_loginBtn = nullptr;

	QPixmap      m_bgRaw;
	QFont m_font;

	bool bIsAutoLogin = true;																		// 是否自动登录
};

#endif // LOGINWIDGET_H