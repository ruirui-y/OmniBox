#include "MyChannel.h"
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <mymuduo/net/ThreadSwitcher.h>
#include <mymuduo/Log/Logger.h>
#include <vector>
#include <endian.h>
#include "common.pb.h"
#include "ConnectionPool.h"

using namespace std::placeholders;

MyChannel::MyChannel(EventLoop* loop, const std::string& ip, int port)
    : loop_(loop), ip_(ip), port_(port)
{
    // 1. 直接使用传进来的 loop 创建客户端
    client_.reset(new TcpClient(loop_, ip_, port_, "RpcClient"));

    // 2. 绑定回调
    using std::placeholders::_1;
    using std::placeholders::_2;
    client_->SetConnectionCallback(std::bind(&MyChannel::OnConnection, this, _1));
    client_->SetMessageCallback(std::bind(&MyChannel::OnMessage, this, _1, _2));

    // 3. 发起异步连接
    client_->Connect();
}

MyChannel::~MyChannel()
{
    client_->Disconnect();

    // 叫醒后台大管家下班
    if (loop_) {
        loop_->Quit();
    }

    // 等待后台线程安全结束
    if (loop_thread_.joinable()) {
        loop_thread_.join();
    }
}

void MyChannel::OnConnection(const TcpConnectionPtr& conn)
{
    if (conn->Connected()) 
    {
        conn_ = conn;
        LOG_INFO << "[MyChannel] Successfully connected to RPC Server " << ip_ << ":" << port_;
    }
    else
    {
        conn_.reset();
        LOG_INFO << "[MyChannel] Disconnected from RPC Server " << ip_ << ":" << port_;
    }
}

void MyChannel::OnMessage(const TcpConnectionPtr& conn, Buffer* buffer)
{
    // TCP 是字节流，只要缓冲区里数据大于等于包头长度 (4字节)，我们就尝试拆包
    while (buffer->ReadableBytes() >= 4)
    {
        // 1. 偷看前 4 个字节 (peek 不会清除数据)
        uint32_t net_len = 0;
        memcpy(&net_len, buffer->peek(), 4);
        uint32_t total_len = ntohl(net_len);

        // 2. 关键防御：如果连一个完整包的数据都还没到齐，直接 break！
        // 等底层的 epoll 下次收齐了再触发这个函数，线程绝对不在这里死等！
        if (buffer->ReadableBytes() < total_len + 4)
        {
            break;
        }

        // 3. 既然到齐了，就正式把包取出来
        buffer->retrieve(4); // 扔掉长度字段

        // 防御异常脏数据
        if (total_len < 8) {
            buffer->retrieve(total_len);
            continue;
        }

        // 4. 拆出单号 seq_id
        uint64_t network_seq_id = 0;
        memcpy(&network_seq_id, buffer->peek(), 8);
        uint64_t seq_id = be64toh(network_seq_id);
        buffer->retrieve(8);                                                                    // 扔掉单号字段

        // 5. 拿出纯 Protobuf 数据
        std::string pb_data = buffer->RetrieveAsString(total_len - 8);
        
        // 6. 查找对应的回调上下文
        CallContext ctx;
        auto it = pending_calls_.find(seq_id);
        if (it != pending_calls_.end())
        {
            ctx = it->second;
            pending_calls_.erase(it);
        }

        if (ctx.response && ctx.done) 
        {
            ctx.response->ParseFromString(pb_data);

            // 执行业务层传进来的回调！
            // (注意：这里默认是在 MyChannel 的后台 loop 线程里执行的，
            // 如果你想严格按照你说的“推送到发起请求的线程”，
            // 业务层在构造 done 的时候，自己用 runInLoop 包装一下即可！)
            ctx.done->Run();
        }
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
    omnibox::RpcHeader header;
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

    // 5. 登记回调上下文，投递到事件循环去操作
    ThreadSwitcher::Run(loop_, [this, controller, done, seq_id, response, send_rpc_str]()
        {
            if (!conn_ || !conn_->Connected())
            {
                if (controller) controller->SetFailed("Connection is down!");
                if (done) done->Run();
                return;
            }

            pending_calls_[seq_id] = { response, done };
            conn_->Send(send_rpc_str);
        });
}