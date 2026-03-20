#ifndef MY_GATEWAY_SERVICE_H
#define MY_GATEWAY_SERVICE_H

#include "gateway.pb.h"
#include <mymuduo/Log/Logger.h>
#include <mymuduo/net/TcpConnection.h>
#include <mutex>
#include "GatewayTcpServer.h"
using namespace game::rpc;

/*
* 其余服务通过rpc调用gateway，gateway再通过tcp连接推送给客户端
*/

class MyGatewayService : public game::rpc::GatewayService
{
public:
    MyGatewayService(GatewayTcpServer* tcp_server) : tcp_server_(tcp_server) {}
    virtual ~MyGatewayService() {}

public:
    virtual void PushMessage(::google::protobuf::RpcController* controller,
        const ::game::rpc::PushMessageRequest* request,
        ::game::rpc::PushMessageResponse* response,
        ::google::protobuf::Closure* done) override
    {
        int32_t uid = request->user_id();
        int32_t type = request->msg_type();
        std::string content = request->content();

        LOG_INFO << "Gateway received push request from backend! Target User: " << uid;
        
        // 将消息推送给对应的客户端
        bool success = tcp_server_->PushMessageToClient(uid, type, content);

        // 返回rpc响应
        if (success)
        {
            response->set_errcode(0);
            response->set_errmsg("Push success");
        }
        else 
        {
            LOG_ERROR << "[Gateway Internal] Push failed, User " << uid << " is offline.";
            response->set_errcode(1);
            response->set_errmsg("User offline");
        }
        done->Run();
    }

private:
    GatewayTcpServer* tcp_server_;
};

#endif