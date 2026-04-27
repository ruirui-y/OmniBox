#include "LoginWidget.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QCheckBox>
#include <QFontDatabase>
#include <QFont>
#include <QApplication>
#include "common.pb.h"
#include "TCPMgr.h"
#include "LogRecord.h"
#include "Global.h"
#include "UserMgr.h"
#include "JsonTool.h"
#include "CinemaMessageBox.h"
#include "ThreadPool.h"
#include "TitleBar.h"


LoginWidget::LoginWidget(QWidget* parent)
	: QWidget(parent), _Parent(parent)
{
	setFixedSize(600, 500);

	// 同步无边框与透明属性
	setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);
	setAttribute(Qt::WA_TranslucentBackground);
	setAttribute(Qt::WA_OpaquePaintEvent);

	BuildUI();

	hide();
	BindSlots();
	emit sig_connect_tcp();
}

LoginWidget::~LoginWidget()
{
	if (m_remember->checkState())
	{
		WriteLoginConfig();
	}
}

void LoginWidget::BuildUI()
{
	// 1. 根布局
	auto* root = new QVBoxLayout(this);
	root->setContentsMargins(0, 0, 0, 0);
	root->setSpacing(0);

	// 2. 顶部栏复用
	m_title = new TitleBar(this);
	m_title->SetLogo(":/demand/Images/Login.jpg");
	root->addWidget(m_title);

	// 3. 中央容器
	m_panel = new QWidget(this);
	m_panel->setObjectName("loginPanel");
	m_panel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	// 表单内容布局
	auto* formLayout = new QVBoxLayout(m_panel);
	formLayout->setContentsMargins(60, 40, 60, 60);
	formLayout->setSpacing(20);

	// 账号部分
	m_userLbl = new QLabel(u8"账号", m_panel);
	m_userEdit = new QLineEdit(m_panel);
	m_userEdit->setObjectName("loginInput");
	m_userEdit->setPlaceholderText(u8"请输入账号");

	// ==========================================================
	// 密码部分 (新增了可视化按钮)
	// ==========================================================
	m_passLbl = new QLabel(u8"访问密码", m_panel);
	m_passEdit = new QLineEdit(m_panel);
	m_passEdit->setObjectName("loginInput");
	m_passEdit->setPlaceholderText(u8"请输入密码");
	m_passEdit->setEchoMode(QLineEdit::Password);

	// 1. 实例化眼睛按钮 (父对象必须是 m_passEdit)
	auto* eyeBtn = new QPushButton(m_passEdit);
	eyeBtn->setObjectName("PasswordEyeBtn");
	eyeBtn->setCursor(Qt::PointingHandCursor);
	eyeBtn->setFixedSize(24, 24);
	eyeBtn->setToolTip(u8"显示密码");
	eyeBtn->setProperty("isOpen", false);                                           // 自定义属性：配合 QSS 切换图标

	// 2. 将按钮固定在输入框的最右侧
	auto* passLayout = new QHBoxLayout(m_passEdit);
	passLayout->setContentsMargins(0, 0, 10, 0);                                    // 右侧留 10px 边距
	passLayout->addStretch();                                                       // 将按钮挤到右边
	passLayout->addWidget(eyeBtn);

	// 3. 点击切换可见性逻辑
	connect(eyeBtn, &QPushButton::clicked, this, [=]() {
		bool isPassword = (m_passEdit->echoMode() == QLineEdit::Password);

		// 切换输入框模式
		m_passEdit->setEchoMode(isPassword ? QLineEdit::Normal : QLineEdit::Password);
		eyeBtn->setToolTip(isPassword ? u8"隐藏密码" : u8"显示密码");

		// 切换 QSS 状态 (动态属性)
		eyeBtn->setProperty("isOpen", isPassword);
		eyeBtn->style()->unpolish(eyeBtn);
		eyeBtn->style()->polish(eyeBtn);
		});
	// ==========================================================

	// 辅助项
	m_remember = new QCheckBox(u8"记住登录状态", m_panel);
	m_remember->setObjectName("loginCheckBox");

	m_loginBtn = new QPushButton(u8"登 录", m_panel);
	m_loginBtn->setObjectName("loginSubmitBtn");
	m_loginBtn->setCursor(Qt::PointingHandCursor);
	m_loginBtn->setFixedHeight(50);

	// 封装输入框布局的 Lambda
	auto addField = [&](QLabel* lbl, QLineEdit* edit) {
		lbl->setObjectName("fieldLabel");
		lbl->setContentsMargins(2, 0, 0, 0);
		formLayout->addWidget(lbl);
		formLayout->addWidget(edit);
		formLayout->addSpacing(5);
		};

	// 组装
	formLayout->addStretch(1);
	addField(m_userLbl, m_userEdit);
	addField(m_passLbl, m_passEdit);
	formLayout->addWidget(m_remember);
	formLayout->addSpacing(10);
	formLayout->addWidget(m_loginBtn);
	formLayout->addStretch(2);

	root->addWidget(m_panel, 1);
}

void LoginWidget::BindSlots()
{
	// 绑定信号槽
	connect(m_loginBtn, &QPushButton::clicked, this, &LoginWidget::OnLoginButtonClicked);										// 登录按钮点击事件
	connect(this, &LoginWidget::sig_connect_tcp, ThreadPool::Instance()->GetTCPMgr(), &TCPMgr::SlotTcpConnect);
	connect(ThreadPool::Instance()->GetTCPMgr(), &TCPMgr::SigConnectSuccess, this, &LoginWidget::slot_tcp_con_finished);
	connect(ThreadPool::Instance()->GetTCPMgr(), &TCPMgr::SigLoginFailed, this, &LoginWidget::slot_login_failed);
}

bool LoginWidget::EnableBtn(bool enable)
{
	m_loginBtn->setEnabled(enable);
	return enable;
}

void LoginWidget::AutoLogin()
{
	if (!bIsAutoLogin)
		return;

	bIsAutoLogin = false;

	QJsonDocument doc;
	JsonTool::Instance()->readJsonFile(LoginConfigPath, doc);
	QJsonObject obj = doc.object();
	if (obj.isEmpty())
	{
		qDebug() << "AutoLogin: config file is empty";
		show();
		_Parent->show();
		return;
	}

	QString user = obj["Account"].toString();
	QString pass = obj["Password"].toString();

	if (user.isEmpty() || pass.isEmpty())
	{
		qDebug() << "AutoLogin: user or pass is empty";
		show();
		_Parent->show();
		return;
	}

	qDebug() << "AutoLogin: user:" << user << " pass:" << pass;

	m_userEdit->setText(user);
	m_passEdit->setText(pass);
	OnLoginButtonClicked();
}

void LoginWidget::WriteLoginConfig()
{
	UserInfo user_info = UserMgr::Instance()->getUserInfo();

	QJsonObject login_json;
	login_json["Account"] = user_info.UserName;
	login_json["Password"] = user_info.Password;
	login_json["icon"] = "";

	// 写入配置文件中
	QJsonDocument login_doc(login_json);
	JsonTool::Instance()->writeJsonFile(LoginConfigPath, login_doc);
}

/* 检查用户名是否合法 */
bool LoginWidget::CheckUserValid()
{
	if (m_userEdit->text().isEmpty())
	{
		qDebug() << QString::fromLocal8Bit("用户名不能为空");
		return false;
	}

	return true;
}

/* 检验密码是否合法 */
bool LoginWidget::CheckPasswordValid()
{
	QString passText = m_passEdit->text();
	if (passText.length() < 6 || passText.length() > 15)
	{
		qDebug() << QString::fromLocal8Bit("密码长度必须在6-15位之间");
		// CinemaMessageBox::ShowWarning(this, u8"密码校验", u8"密码长度必须在6-15位之间");
		return false;
	}

	/* 密码长度至少6位 可以是字母、数字、特定的特殊字符 */
	QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*]{6,15}$");
	bool match = regExp.match(passText).hasMatch();
	if (!match)
	{
		qDebug() << QString::fromLocal8Bit("密码只能包含字母、数字、!@#$%^&*");
		return false;
	}

	return true;
}

/* 登录按钮点击时 */
void LoginWidget::OnLoginButtonClicked()
{
	if (!CheckUserValid())
	{
		EnableBtn(true);
		return;
	}

	if (!CheckPasswordValid())
	{
		EnableBtn(true);
		return;
	}

	QString user = m_userEdit->text();
	QString pass = m_passEdit->text();

	// 设置用户名和密码
	UserInfo user_info;
	user_info.UserName = user;
	user_info.Password = pass;
	UserMgr::Instance()->setUserInfo(user_info);

	// 禁止重复点击
	EnableBtn(false);

	// 写入配置文件中
	WriteLoginConfig();

	ThreadPool::Instance()->GetTCPMgr()->Login(user, pass);												// 发送登录请求

	QTimer::singleShot(2000, this, [this]()
		{
			EnableBtn(true);
		}
	);
}

/* 如果连接聊天服务器成功，则发送登录请求 */
void LoginWidget::slot_tcp_con_finished(bool success)
{
	if (success)
	{
		qDebug() << QString::fromLocal8Bit("网络正常,可以登录");
		// 自动登录
		AutoLogin();
	}
	else
	{
		qDebug() << QString::fromLocal8Bit("网络异常,连接服务器失败，无法登录");
		EnableBtn(true);
		_Parent->show();
	}
}

void LoginWidget::slot_login_failed(int errCode)
{
	using namespace ServerApi;
	QString result;

	// 👑 绝杀：使用 switch-case 替换 if-else，逻辑更清晰，执行效率更高
	switch (errCode)
	{
	case ErrorCode::ERR_SERVER_INTERNAL:
		result = QString::fromLocal8Bit("服务器内部错误，请稍后再试");
		break;
	case ErrorCode::ERR_WRONG_PWD:
		result = QString::fromLocal8Bit("账号不存在或密码错误");
		break;
	case ErrorCode::ERR_ACCOUNT_IN_USE:
		result = QString::fromLocal8Bit("账号已在其他设备登录，请勿重复登录");
		break;
	case ErrorCode::ERR_ACCOUNT_EXPIRED:
		result = QString::fromLocal8Bit("账号授权已过期，请联系管理员续期");
		break;
	default:
		// 兜底逻辑：如果出现其他未知的错误码（比如网络层的断层）
		result = QString::fromLocal8Bit("登录异常，未知错误码: ") + QString::number(errCode);
		break;
	}

	CinemaMessageBox::ShowWarning(this, u8"登录失败", result);

	_Parent->show();

	qDebug() << QString::fromLocal8Bit("[LoginWidget] 登录失败, 错误码:") << errCode << u8" 描述:" << result;

	// 恢复登录按钮状态
	EnableBtn(true);
}

// 用于注销退回登录页时，清空残留数据
void LoginWidget::ClearInputs()
{
	// 如果你不希望每次退出连账号都清空，可以注释掉 m_userEdit->clear()
	// m_userEdit->clear(); 

	m_passEdit->clear();																	// 密码必须清空，防泄露
	EnableBtn(true);																		// 确保按钮是可以点击的
	m_loginBtn->setText(u8"登 录");															// 恢复按钮文字
}