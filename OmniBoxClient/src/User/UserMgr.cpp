#include "UserMgr.h"
#include <QReadLocker>
#include <QWriteLocker>

UserMgr::UserMgr(QObject* parent) : QObject(parent)
{
}

UserMgr::~UserMgr()
{
}

void UserMgr::SetUserName(const QString& user_name)
{
    QWriteLocker locker(&rw_lock_);
    user_name_ = user_name;
}

void UserMgr::SetPassword(const QString& password)
{
    QWriteLocker locker(&rw_lock_);
    password_ = password;
}

void UserMgr::SetToken(const QString& token)
{
    QWriteLocker locker(&rw_lock_);
    token_ = token;
}

void UserMgr::SetId(const int32_t& id)
{
    QWriteLocker locker(&rw_lock_);
    id_ = id;
}

void UserMgr::SetUserInfo(const QString& user_name, const QString& password, const QString& token, const int32_t& id)
{
    QWriteLocker locker(&rw_lock_);
    user_name_ = user_name;
    password_ = password;
    token_ = token;
    id_ = id;
}

QString UserMgr::GetUserName() const
{
    QReadLocker locker(&rw_lock_);
    return user_name_;
}

QString UserMgr::GetPassword() const
{
    QReadLocker locker(&rw_lock_);
    return password_;
}

QString UserMgr::GetToken() const
{
    QReadLocker locker(&rw_lock_);
    return token_;
}

int32_t UserMgr::GetId() const
{
    QReadLocker locker(&rw_lock_);
    return id_;
}

void UserMgr::ClearUser()
{
    QWriteLocker locker(&rw_lock_);
    user_name_.clear();
    password_.clear();
    token_.clear();
    id_ = -1;
}