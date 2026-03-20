#ifndef MY_LOGIN_SERVICE_H
#define MY_LOGIN_SERVICE_H

#include <string>
#include "login.pb.h"
#include "RedisClient.h"
#include <mymuduo/Log/Logger.h>
using namespace game::rpc;

class MyLoginService : public game::rpc::LoginService
{
public:

    virtual void Login(::google::protobuf::RpcController* controller,
        const ::LoginRequest* request,
        ::LoginResponse* response,
        ::google::protobuf::Closure* done)
    {
        // 1. 框架给业务吐出请求参数
        std::string name = request->username();
        std::string pwd = request->password();

        RedisClient redis;
        if (redis.Connect("127.0.0.1", 6379) == false)
        {
            response->set_errcode(500);
            response->set_errmsg("redis connect error");
            if (done) done->Run();
            return;
        }

        // 2. 分布式锁防御
        std::string lock_key = "lock:login:" + name;
        // 尝试加锁，过期时间为3s
        if (!redis.SetNx(lock_key, "1", 3))
        {
            response->set_errcode(403);
            response->set_errmsg("server is busy, please try again later");
            if (done) done->Run();
            return;
        }

        // 2. 执行本地业务todo                                                  

        // 3. 把响应写回给框架
        response->set_errcode(0);
        response->set_errmsg("Login Success");
        response->set_token("11111111");

        // 测试
        int32_t uid = name == "mars" ? 1 : 2;
        LOG_INFO << "login client name = " << name << " uid = " << uid;
        response->set_user_id(uid);

        // 4. 释放分布式锁并执行回调操作
        // 这里的 done 本质是 RpcProvider 传进来的 SendRpcResponse 回调
        redis.Del(lock_key);
        done->Run();                                                                                
    }
};

#endif