#ifndef MY_LOGIN_SERVICE_H
#define MY_LOGIN_SERVICE_H

#include <string>
#include <memory>
#include "login.pb.h"

// 前置声明，避免在头文件中引入过多的底层依赖
class EventLoop;
class ThreadPool;

class MyLoginService : public game::rpc::LoginService
{
public:
    MyLoginService(EventLoop* loop, std::shared_ptr<ThreadPool> threadPool);
    ~MyLoginService() = default;

    virtual void Login(::google::protobuf::RpcController* controller,
        const ::game::rpc::LoginRequest* request,
        ::game::rpc::LoginResponse* response,
        ::google::protobuf::Closure* done) override;

    virtual void Heartbeat(::google::protobuf::RpcController* controller,
        const ::game::rpc::HeartbeatRequest* request,
        ::game::rpc::HeartbeatResponse* response,
        ::google::protobuf::Closure* done) override;

private:
    std::string sha256(const std::string& str);

private:
    EventLoop* loop_;
    std::shared_ptr<ThreadPool> thread_pool_;
};

#endif // MY_LOGIN_SERVICE_H