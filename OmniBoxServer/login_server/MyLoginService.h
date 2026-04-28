#ifndef MY_LOGIN_SERVICE_H
#define MY_LOGIN_SERVICE_H

#include <string>
#include <memory>
#include "server_msg.pb.h"
#include "internal_rpc.pb.h"

// 前置声明，避免在头文件中引入过多的底层依赖
class EventLoop;
class ThreadPool;

using namespace omnibox;

class MyLoginService : public LoginService
{
public:
    MyLoginService(EventLoop* loop, std::shared_ptr<ThreadPool> threadPool);
    ~MyLoginService() = default;

    virtual void Login(::google::protobuf::RpcController* controller,
        const ::LoginRequest* request,
        ::LoginResponse* response,
        ::google::protobuf::Closure* done) override;

    virtual void Heartbeat(::google::protobuf::RpcController* controller,
        const ::HeartbeatRequest* request,
        ::HeartbeatResponse* response,
        ::google::protobuf::Closure* done) override;

private:
    std::string sha256(const std::string& str);

private:
    EventLoop* loop_;
    std::shared_ptr<ThreadPool> thread_pool_;
};

#endif // MY_LOGIN_SERVICE_H