#include "GatewayTcpServer.h"
#include <mymuduo/Log/Logger.h>
#include <mymuduo/db/DbExecutor.h>
#include <any>
#include <arpa/inet.h>
#include "server_msg.pb.h"
#include "internal_rpc.pb.h"
#include "RedisClient.h"   
#include "MyController.h"
#include "TcpRpcClosure.h"

using namespace std::placeholders;
using namespace omnibox;

#define CONN_TIME_OUT                                       60                                  

GatewayTcpServer::GatewayTcpServer(EventLoop* loop, const std::string& ip, uint16_t port)
    : server_(loop, ip, port, CONN_TIME_OUT), loop_(loop), ip_(ip), port_(port)
{
    server_.SetConnectionCallback(std::bind(&GatewayTcpServer::OnConnection, this, _1));
    server_.SetMessageCallback(std::bind(&GatewayTcpServer::OnMessage, this, _1, _2));

    // ================== 路由表注册 ==================
    RegisterHandler(ID_LOGIN_REQ, std::bind(&GatewayTcpServer::HandleLoginReq, this, _1, _2,_3));

    // 建立长连接

    // 1. 启动一个专门针对后端微服务通信的后台 IO 线程
    rpc_client_thread_ = std::make_unique<EventLoopThread>();
    rpc_client_loop_ = rpc_client_thread_->StartLoop();

    login_channel_ = std::make_shared<MyChannel>(rpc_client_loop_, "127.0.0.1", 9090);
    //transfer_channel_ = std::make_shared<MyChannel>("127.0.0.1", 8082);
    //meta_channel_ = std::make_shared<MyChannel>("127.0.0.1", 8090);

    // 初始化线程池
    thread_pool_ = std::make_shared<ThreadPool>("gateway_pool_");
    thread_pool_->start(4);
}

void GatewayTcpServer::Start(int thread_num)
{
    server_.Start(thread_num);
}

void GatewayTcpServer::RegisterHandler(uint32_t msg_id, MsgHandler handler)
{
    msg_dispatcher_[msg_id] = handler;
}

void GatewayTcpServer::OnConnection(const std::shared_ptr<TcpConnection>& conn)
{
    // 断开连接，移除映射
    if (!conn->Connected())
    {
        // 这里断开只有三种情况，客户端主动断开，服务器主动断开以及超时断开
        // 不论是哪种断开，都需要判断是否登录成功，登录成功的标志就是conn是否绑定了uid
        bool bLogin = conn->GetContext().has_value();
        if (bLogin)
        {
            // 移除映射
            int32_t uid = std::any_cast<int32_t>(conn->GetContext());
            
            {
                std::lock_guard<std::mutex> lock(session_mutex_);
                user_sessions_.erase(uid);
            }

            // 由于登录服务不再以心跳时间作为登录条件，而是挤掉之前登录的账户，所以这里不需要更新心跳时间了

            LOG_INFO << "[Gateway] Player " << uid << " disconnected. Session removed.";
        }
    }
    else
    {
        LOG_INFO << "[Gateway] New external client connected!";
    }
}

void GatewayTcpServer::OnMessage(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer)
{
    // 协议格式: [TotalLen (4字节)] + [HeaderLen (2字节)] + [PacketHeader 字节流] + [Body 字节流]
    // 约定：TotalLen = 4(TotalLen) + 2(HeaderLen的长度) + PacketHeader的长度 + Body的长度
    // 最小包长：至少要有 TotalLen(4) + HeaderLen(2) = 6 字节才能开始初步解析
    while (buffer->ReadableBytes() >= 6)
    {
        // 1. 偷看出包体的总长度 (网络字节序转主机字节序)
        uint32_t total_len = buffer->PeekInt32();

        // 2. 检查缓冲区是否收到了一个完整的物理包 (TotalLen 不包含自身的 4 字节)
        if (buffer->ReadableBytes() >= total_len)
        {
            buffer->retrieve(4);                                                                                // 弹出 TotalLen (4字节)

            // 3. 提取 Header 的长度
            uint16_t header_len = buffer->RetrieveInt16();                                                      // 弹出 HeaderLen (2字节)

            // 👑 安全防线：防止恶意的/损坏的包导致内存溢出
            if (header_len > total_len - 4 - 2)
            {
                LOG_ERROR << "[Gateway] Invalid HeaderLen (" << header_len << "). Dropping connection.";
                conn->ForceClose();                                                                             // 发现脏数据，果断断开“有毒”的连接
                return;
            }

            // 4. 提取 PacketHeader 字节流并进行反序列化
            std::string header_str = buffer->RetrieveAsString(header_len);
            omnibox::PacketHeader header;
            if (!header.ParseFromString(header_str))
            {
                LOG_ERROR << "[Gateway] Failed to parse PacketHeader! Dropping connection.";
                conn->ForceClose();
                return;
            }

            // 5. 提取真正的业务 Body 字节流
            uint32_t body_len = total_len - 4 - 2 - header_len;
            std::string body_str = buffer->RetrieveAsString(body_len);

            // 6. 获取路由的核心凭证 MsgId
            uint32_t msg_id = header.msg_id();

            // ================== 路由表分发 ==================
            auto it = msg_dispatcher_.find(msg_id);
            if (it != msg_dispatcher_.end())
            {
                // 找到了对应的处理函数，直接把纯粹的 Body 二进制流扔给它去解析！
                // (注意：你的 HandleLoginReq 完全不用改，它依然只负责解析 Body)
                it->second(conn, header, body_str);
            }
            else
            {
                LOG_ERROR << "[Gateway] Unregistered MsgID: " << msg_id << ". Dropping packet.";
            }
        }
        else
        {
            break; // 发生了半包 (TCP碎片化)，退出循环，等待底层的下一次数据到达
        }
    }
}

// ================== 具体的业务处理 (Handler) ==================
void GatewayTcpServer::HandleLoginReq(const std::shared_ptr<TcpConnection>& conn, const omnibox::PacketHeader& header, const std::string& pb_data)
{
    // 1. 解包外网请求
    auto req = std::make_shared<omnibox::LoginRequest>();
    if (!req->ParseFromString(pb_data))
    {
        LOG_ERROR << "[Gateway] Failed to parse ClientLoginRequest!";
        conn->ForceClose();
        return;
    }

    std::string username = req->username();
    uint64_t client_seq_id = header.seq_id(); // 👑 提取客户端发来的序列号

    LOG_INFO << "[Gateway] User attempting login with account: " << username << ", seq_id: " << client_seq_id;

    // 2. 封装登录回调 (👑 注意这里捕获了 client_seq_id)
    auto success_cb = [this, username, client_seq_id](const std::shared_ptr<TcpConnection>& conn, const std::shared_ptr<omnibox::LoginResponse>& response)
        {
            // 判断登录是否成功
            if (response->errcode() == omnibox::ERR_SUCCESS)
            {
                int32_t uid = response->user_id();
                conn->SetContext(uid);                                                      // 向底层连接绑定用户id

                // 判断是否重复登录
                std::shared_ptr<TcpConnection> old_conn;
                {
                    std::lock_guard<std::mutex> lock(session_mutex_);
                    auto it = user_sessions_.find(uid);
                    if (it != user_sessions_.end())
                    {
                        // 发现这个 UID 已经在网关里了！把它拿出来准备踢掉
                        old_conn = it->second;
                    }
                    // 无论如何，新连接上位
                    user_sessions_[uid] = conn;
                }

                // 踢人
                if (old_conn)
                {
                    LOG_WARN << "[Gateway] User " << uid << " logged in from a new device. Kicking old connection.";
                    old_conn->ForceClose();
                }

                LOG_INFO << "[Gateway] Player " << uid << " login success! Token generated.";
            }
            else
            {
                // 登录失败，强制断开连接
                LOG_INFO << "[Gateway] Player " << username << " login failed! Error code: " << response->errcode();
                
                // 将关闭连接操作交给客户端
            }

            // 返回响应
            SendToConn(conn, omnibox::ID_LOGIN_RSP, omnibox::ErrorCode(response->errcode()),
                response->errmsg(), client_seq_id, response->SerializeAsString());
        };

    // 3. 转发登录请求
    auto controler = std::make_shared<MyController>();
    auto rsp = std::make_shared<omnibox::LoginResponse>();
    auto closure = new TcpRpcClosure<omnibox::LoginResponse>(conn, controler, rsp, success_cb);
    omnibox::LoginService_Stub stub(login_channel_.get());
    stub.Login(controler.get(), req.get(), rsp.get(), closure);
}

// ================== 推送响应回包 ==================
void GatewayTcpServer::SendToConn(const std::shared_ptr<TcpConnection>& conn, uint32_t msg_id,
    omnibox::ErrorCode err_code,
    const std::string& err_msg,
    uint64_t seq_id, const std::string& pb_data)
{
    // 1. 组装 PacketHeader
    omnibox::PacketHeader header;
    header.set_msg_id(static_cast<omnibox::MsgId>(msg_id));
    header.set_seq_id(seq_id);
    header.set_error_code(err_code);
    header.set_error_msg(err_msg);

    std::string header_str = header.SerializeAsString();
    uint16_t header_len = static_cast<uint16_t>(header_str.size());

    // 2. 计算 TotalLen: 4 + HeaderLen本身(2) + Header实体长度 + Body实体长度
    uint32_t total_len = 4 + 2 + header_len + pb_data.size();

    // 3. 按照协议格式打包到 Buffer
    Buffer buf;
    buf.AppendInt32(total_len);
    buf.AppendInt16(header_len);
    buf.Append(header_str.data(), header_str.size());
    buf.Append(pb_data.data(), pb_data.size());

    // 4. 发送给客户端
    conn->Send(buf.RetrieveAllAsString());
}

bool GatewayTcpServer::PushMessageToClient(int32_t uid, uint32_t msg_type, const std::string& content)
{
    std::shared_ptr<TcpConnection> target_conn = nullptr;
    {
        std::lock_guard<std::mutex> lock(session_mutex_);
        auto it = user_sessions_.find(uid);
        if (it != user_sessions_.end())
        {
            target_conn = it->second;
        }
    }

    //if (target_conn)
    //{
    //    // 组装外网协议发给他 (长度 + MsgID + 数据)
    //    SendToConn(target_conn, msg_type, 1, content);
    //    return true;
    //}
    return false; // 玩家不在线
}