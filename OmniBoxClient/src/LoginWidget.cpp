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


LoginWidget::LoginWidget(QWidget* parent)
	: QWidget(parent), _Parent(parent)
{
	setFixedSize(1100, 700);

	setAttribute(Qt::WA_StyledBackground, true);
	setObjectName("LoginWidget");
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
	// 中央面板
	m_panel = new QWidget(this);
	m_panel->setObjectName("loginPanel");                                       // 绑定专属面板 ID
	m_panel->setMinimumWidth(560);

	// 阴影
	auto* shadow = new QGraphicsDropShadowEffect(this);
	shadow->setOffset(0, 8);
	shadow->setBlurRadius(32);
	shadow->setColor(QColor(0, 0, 0, 150));
	m_panel->setGraphicsEffect(shadow);

	// 内容 (Logo)
	m_logo = new QLabel(m_panel);
	m_logo->setObjectName("loginLogo");                                         // 绑定 Logo ID
	m_logo->setMinimumHeight(96);
	m_logo->setFixedWidth(300);

	m_userLbl = new QLabel(QStringLiteral("账号"), m_panel);
	m_userEdit = new QLineEdit(m_panel);
	m_userEdit->setObjectName("loginInput");                                    // 统一输入框 ID
	m_userEdit->setPlaceholderText(QStringLiteral("请输入账号"));

	m_passLbl = new QLabel(QStringLiteral("密码"), m_panel);
	m_passEdit = new QLineEdit(m_panel);
	m_passEdit->setObjectName("loginInput");                                    // 统一输入框 ID
	m_passEdit->setPlaceholderText(QStringLiteral("请输入密码"));
	// m_passEdit->setEchoMode(QLineEdit::Password);

	m_remember = new QCheckBox(QStringLiteral("记住密码"), m_panel);
	m_remember->setObjectName("loginCheckBox");                                 // 绑定多选框 ID
	m_remember->setChecked(true);

	m_loginBtn = new QPushButton(QStringLiteral("登 录"), m_panel);
	m_loginBtn->setObjectName("loginSubmitBtn");                                // 👑 核心登录大按钮专属 ID
	m_loginBtn->setCursor(Qt::PointingHandCursor);
	m_loginBtn->setFixedHeight(54);

	// 布局（面板）
	auto* form = new QVBoxLayout(m_panel);
	form->setContentsMargins(32, 32, 32, 32);
	form->setSpacing(16);
	form->addWidget(m_logo, 0, Qt::AlignHCenter);

	auto addField = [&](QLabel* lbl, QLineEdit* edit) {
		lbl->setObjectName("fieldLabel");                                       // 标题 Label 统一 ID
		auto* v = new QVBoxLayout();
		v->setSpacing(8);
		v->addWidget(lbl);
		v->addWidget(edit);
		form->addLayout(v);
		};
	addField(m_userLbl, m_userEdit);
	addField(m_passLbl, m_passEdit);

	form->addWidget(m_remember);
	form->addSpacing(4);
	form->addWidget(m_loginBtn);

	// 外层布局：让面板居中
	auto* outer = new QHBoxLayout(this);
	outer->setContentsMargins(48, 48, 48, 48);
	outer->addStretch();
	outer->addWidget(m_panel, 0, Qt::AlignVCenter);
	outer->addStretch();
	setLayout(outer);
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

	qDebug() << QString::fromLocal8Bit("[LoginWidget] 登录失败, 错误码:") << errCode << " 描述:" << result;

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