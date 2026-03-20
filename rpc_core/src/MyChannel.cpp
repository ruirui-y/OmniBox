#include "MyChannel.h"
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <mymuduo/Log/Logger.h>
#include <vector>
#include <endian.h>
#include "rpcheader.pb.h"
#include "ConnectionPool.h"

MyChannel::MyChannel(const std::string& ip, int port)
    : ip_(ip),
    port_(port),
    is_running_(true)
{
    client_fd_ = ConnectionPool::GetInstance().GetConnection(ip_, port_);
    recv_thread_ = std::thread(&MyChannel::ReceiverTask, this);
}

MyChannel::~MyChannel()
{
    is_running_ = false;
    
    // 如果MyChannel是成员变量就是长连接，如果是局部变量就是短连接
    if (client_fd_)
    {
        shutdown(client_fd_, SHUT_RDWR);
    }

    if (recv_thread_.joinable())
    {
        recv_thread_.join();
    }
}

void MyChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
    google::protobuf::RpcController* controller,
    const google::protobuf::Message* request,
    google::protobuf::Message* response,
    google::protobuf::Closure* done)
{
    // 1. 生成唯一单号
    uint64_t seq_id = seq_id_allocator_++;

    // 2. 组装请求头
    rpc::core::RpcHeader header;
    header.set_service_name(method->service()->name());
    header.set_method_name(method->name());
    header.set_method_index(method->index());
    header.set_args_size(request->ByteSizeLong());
    header.set_seq_id(seq_id);

    // 3. 序列化请求头和请求数据
    std::string header_str;
    header.SerializeToString(&header_str);
    uint32_t header_size = header_str.size();
    uint32_t net_header_size = htonl(header_size);                                                      // 发送网络字节序

    std::string request_str;
    request->SerializeToString(&request_str);

    // 4. 数据整合
    std::string send_rpc_str;;                                                      
    send_rpc_str.insert(0, std::string((char*)&net_header_size, 4));
    send_rpc_str += header_str;
    send_rpc_str += request_str;

    // 5. 准备睡觉的床和叫醒凭证
    auto prom = std::make_shared<std::promise<std::string>>();
    std::future<std::string> fut = prom->get_future();

    {
        // 去总台登记：单号 seq_id 对应的凭证是 prom
        std::lock_guard<std::mutex> lock(map_mutex_);
        pending_calls_[seq_id] = prom;
    }

    // 6. 加锁发送，防止多个工作线程同时写 Socket 导致数据错乱
    {
        std::lock_guard<std::mutex> write_lock(write_mutex_);
        if (send(client_fd_, send_rpc_str.c_str(), send_rpc_str.size(), 0) < 0)
        {
            if (controller) controller->SetFailed("send error! errno:" + std::to_string(errno));
            // 发送失败，把登记记录删掉，防止内存泄漏
            std::lock_guard<std::mutex> lock(map_mutex_);
            pending_calls_.erase(seq_id);
            return;
        }
    }

    // 7. 终极奥义：带超时的沉睡！不耗费一点 CPU 资源
    // 等待后台 ReceiverTask 来调用 set_value 唤醒它
    if (fut.wait_for(std::chrono::seconds(5)) == std::future_status::timeout)
    {
        if (controller) controller->SetFailed("RPC response timeout!");
        std::lock_guard<std::mutex> lock(map_mutex_);
        pending_calls_.erase(seq_id);                                                                   // 超时了，撤销登记
        return;
    }

    // 8. 醒来！拿取后台线程塞进来的纯正 pb 数据
    std::string response_data = fut.get();

    // 9. 反序列化
    if (!response->ParseFromString(response_data))
    {
        if (controller) controller->SetFailed("parse response error!");
    }
}

void MyChannel::ReceiverTask()
{
    while (is_running_)
    {
        // 1. 读取4字节的包头长度
        uint32_t network_response_size = 0;
        int n = recv(client_fd_, (char*)&network_response_size, 4, MSG_WAITALL);
        if (n <= 0)
        {
            break;
        }

        // 转化为主机字节序
        uint32_t response_size = ntohl(network_response_size);
        if (response_size > 10 * 1024 * 1024)
        {
            continue;
        }

        // 2. 读取完整的后续数据(包含8字节的SeqID + 纯PB数据)
        std::vector<char> buf(response_size);
        n = recv(client_fd_, buf.data(), response_size, MSG_WAITALL);
        if (n <= 0)
        {
            break;
        }

        if (response_size < 8)
        {
            continue;
        }

        // 3. 拆出单号
        uint64_t network_seq_id = 0;
        memcpy(&network_seq_id, buf.data(), 8);

        // 将 64 位网络字节序（大端序, Big Endian）转回本机的主机字节序（Host）
        // be64toh 意思是：Big Endian 64 to Host
        uint64_t seq_id = be64toh(network_seq_id);

        // 4. 拆出纯pb响应数据
        std::string pb_data(buf.data() + 8, response_size - 8);

        // 5. 查表唤醒等待的线程
        {
            std::lock_guard<std::mutex> lock(map_mutex_);
            auto it = pending_calls_.find(seq_id);
            if (it != pending_calls_.end())
            {
                // 把数据塞进 promise，这会瞬间唤醒 CallMethod 里正在 wait 的线程
                it->second->set_value(pb_data);
                // 唤醒后，注销这个单号
                pending_calls_.erase(it);
            }
        }
    }
}