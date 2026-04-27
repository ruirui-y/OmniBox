#include "UserMgr.h"

UserMgr::UserMgr(QObject *parent)
	: QObject(parent)
{}

UserMgr::~UserMgr()
{}

void UserMgr::setUserInfo(const UserInfo & info)
{
	m_userinfo = info;
}

UserInfo UserMgr::getUserInfo()
{
	return m_userinfo;
}

void UserMgr::ClearUser()
{
	m_userinfo.ResetData();
}