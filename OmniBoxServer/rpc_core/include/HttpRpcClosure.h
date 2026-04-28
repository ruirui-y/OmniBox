#ifndef HTTP_RPC_CLOSURE_H
#define HTTP_RPC_CLOSURE_H

#include <google/protobuf/stubs/callback.h>
#include <memory>
#include <functional>
#include <mymuduo/net/TcpConnection.h>
#include "MyController.h"
// 这是一个模板类，RespType 代表具体的 Protobuf 响应类型 (比如 LoginResponse)

template <typename RespType>
class HttpRpcClosure : public google::protobuf::Closure
{
public:
	using SuccessCallback = std::function<void(const TcpConnectionPtr&, const std::shared_ptr<RespType>&)>;

    // 构造函数：接管所有资源的生命周期
    HttpRpcClosure(TcpConnectionPtr conn,
        std::shared_ptr<MyController> controller,
        std::shared_ptr<RespType> response,
        SuccessCallback success_cb = nullptr)
        : conn_(std::move(conn)),
        controller_(std::move(controller)),
        response_(std::move(response)),
        success_cb_(std::move(success_cb))
    {
    }

    void Run() override
    {
        // 1. 统一处理 RPC 层的物理级失败 (如 TransferServer 挂了、网络断了)
        if (controller_->Failed())
        {
            conn_->Send("HTTP/1.1 502 Bad Gateway\r\nConnection: close\r\n\r\n");
        }
        // 2. RPC 通信成功，执行业务层的逻辑
        else
        {
            if (success_cb_) 
            {
                // 如果外部传了定制的回调，就执行外部的
                success_cb_(conn_, response_);
            }

            conn_->Send("HTTP/1.1 200 OK\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n");
        }

        // Protobuf Closure 的绝对铁律：执行完毕后必须自我销毁！
        // 因为我们等会儿是用 new 创建它的，用完就杀，绝不内存泄漏。
        delete this;
    }

private:
    TcpConnectionPtr conn_;
    std::shared_ptr<MyController> controller_;
    std::shared_ptr<RespType> response_;
    SuccessCallback success_cb_;
};

#endif