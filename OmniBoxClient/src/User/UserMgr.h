#ifndef USERMGR_H
#define USERMGR_H

#include <QObject>
#include "singletion.h"
#include "Struct.h"

class UserMgr  : public QObject, public Singleton<UserMgr>
{
	Q_OBJECT
	friend class Singleton<UserMgr>;

public:
	~UserMgr();

public:
	void setUserInfo(const UserInfo& info);
	UserInfo getUserInfo();

	void ClearUser();

private:
	UserMgr(QObject* parent = 0);

private:
	UserInfo m_userinfo;
};

#endif // USERMGR_H