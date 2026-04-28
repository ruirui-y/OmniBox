#ifndef MY_CHANNEL_H
#define MY_CHANNEL_H

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <string>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <future>
#include <atomic>
#include <mymuduo/net/EventLoop.h>
#include <mymuduo/net/TcpClient.h>
#include <mymuduo/net/TcpConnection.h>
#include <mymuduo/net/Buffer.h>

class MyChannel : public google::protobuf::RpcChannel
{
public:
    MyChannel(const std::string& ip, int port);
    ~MyChannel();

public:
    void CallMethod(const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController* controller,
        const google::protobuf::Message* request,
        google::protobuf::Message* response,
        google::protobuf::Closure* done);

private:
    void OnConnection(const TcpConnectionPtr& conn);
    void OnMessage(const TcpConnectionPtr& conn, Buffer* buffer);

private:
    std::string ip_;
    int port_;

    // 异步
    EventLoop* loop_;
    std::thread loop_thread_;
    std::unique_ptr<TcpClient> client_;

    TcpConnectionPtr conn_;
    std::mutex conn_mutex_;

    struct CallContext
    {
        google::protobuf::Message* response;
        google::protobuf::Closure* done;
    };

    std::mutex map_mutex_;                                                                      // 保护Map 的 锁
    std::unordered_map<uint64_t, CallContext> pending_calls_;                                   // 快递单号映射表

    inline static std::atomic<uint64_t> seq_id_allocator_{ 1 };                                 // 1. 全局唯一的流水号生成器
};
#endif