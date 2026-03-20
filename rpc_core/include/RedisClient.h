#ifndef REDIS_CLIENT_H
#define REDIS_CLIENT_H
#include <hiredis/hiredis.h>
#include <string>

class RedisClient 
{
public:
    RedisClient();
    ~RedisClient();

    // 连接与断开
    bool Connect(const std::string& host = "127.0.0.1", int port = 6379);
    void Disconnect();

    // key-value操作
    bool Set(const std::string& key, const std::string& value);
    std::string Get(const std::string& key, std::string* value);
    bool Del(const std::string& key);

    // Hash操作
    bool HSet(const std::string& key, const std::string& field, const std::string& value);
    std::string HGet(const std::string& key, const std::string& field);

    // 原子操作：如果Key不存在则设置并返回true(抢锁成功)，存在则返回false(抢锁失败)
    // 必须带有过期时间，防止拿到锁的微服务突然崩溃导致死锁
    bool SetNx(const std::string& key, const std::string& value, int expire_seconds);

private:
    redisContext* context_;                                     // redis 同步连接上下文
};

#endif