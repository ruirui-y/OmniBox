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
    void ReceiverTask();

private:
    std::string ip_;
    int port_;

    int client_fd_;
    bool is_running_;
    std::thread recv_thread_;
    std::mutex write_mutex_;                                                                    // 保护发包，防止多个线程把数据写串
    
    std::mutex map_mutex_;                                                                      // 保护Map 的 锁
    std::unordered_map<uint64_t, std::shared_ptr<std::promise<std::string>>> pending_calls_;    // 快递单号映射表

    inline static std::atomic<uint64_t> seq_id_allocator_{ 1 };                                 // 1. 全局唯一的流水号生成器
};
#endif