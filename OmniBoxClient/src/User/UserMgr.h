#ifndef USERMGR_H
#define USERMGR_H

#include <QObject>
#include <QString>
#include <QReadWriteLock>
#include "singletion.h" 

class UserMgr : public QObject, public Singleton<UserMgr>
{
    Q_OBJECT
        friend class Singleton<UserMgr>;

public:
    ~UserMgr();

    // --- 线程安全的 Setter ---
    void SetUserName(const QString& user_name);
    void SetPassword(const QString& password);
    void SetToken(const QString& token);
    void SetId(const int32_t& id);

    // 一次性设置所有信息
    void SetUserInfo(const QString& user_name, const QString& password, const QString& token, const int32_t& id);

    // --- 线程安全的 Getter ---
    QString GetUserName() const;
    QString GetPassword() const;
    QString GetToken() const;
    int32_t GetId() const;

    // 清空用户信息
    void ClearUser();

private:
    UserMgr(QObject* parent = nullptr);

private:
    // 严格遵循 snake_case_ 风格的成员变量
    QString user_name_;
    QString password_;
    QString token_;
    int32_t id_;

    // 读写锁
    mutable QReadWriteLock rw_lock_;
};

#endif // USERMGR_H