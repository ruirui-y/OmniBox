#include "GatewayTcpServer.h"
#include <mymuduo/Log/Logger.h>
#include <mymuduo/db/DbExecutor.h>
#include <any>
#include <arpa/inet.h>
#include "common.pb.h"
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
    RegisterHandler(ID_LOGIN_REQ, std::bind(&GatewayTcpServer::HandleLoginReq, this, _1, _2));

    // 建立长连接
    login_channel_ = std::make_shared<MyChannel>("127.0.0.1", 9090);
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

            // 将最后心跳时间设置为2002年 todo


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
    // len(4) + msg_id(4) + data, len = msg_id + data
    while (buffer->ReadableBytes() >= 8)
    {
        uint32_t total_len = buffer->PeekInt32();
        if (buffer->ReadableBytes() >= total_len + 4)
        {
            buffer->retrieve(4);                                                                      // 清空len(4)
            uint32_t msg_id = buffer->RetrieveInt32();
            std::string pb_data = buffer->RetrieveAsString(total_len - 4);

            // ================== 路由表分发 ==================
            auto it = msg_dispatcher_.find(msg_id);
            if (it != msg_dispatcher_.end())
            {
                // 找到了对应的处理函数，直接把原始二进制流扔给它去解析！
                it->second(conn, pb_data);
            }
            else
            {
                LOG_ERROR << "[Gateway] Unregistered MsgID: " << msg_id << ". Dropping packet.";
            }
        }
        else
        {
            break; // 发生了半包，等下一次数据到来
        }
    }
}

// ================== 具体的业务处理 (Handler) ==================
void GatewayTcpServer::HandleLoginReq(const std::shared_ptr<TcpConnection>& conn, const std::string& pb_data)
{
    // 1. 解包外网请求
    auto req = std::make_shared<LoginRequest>();
    if (!req->ParseFromString(pb_data)) 
    {
        LOG_ERROR << "[Gateway] Failed to parse ClientLoginRequest!";
        conn->ForceClose();
        return;
    }

    std::string username = req->username();
    LOG_INFO << "[Gateway] User attempting login with account: " << req->username();
    
    // 2. 封装登录回调
    auto success_cb = [this, username](const std::shared_ptr<TcpConnection>& conn, const std::shared_ptr<LoginResponse>& response)
        {
            // 判断登录是否成功
            if (response->errcode() == ErrorCode::ERR_SUCCESS)
            {
                int32_t uid = response->user_id();
                conn->SetContext(uid);                                                              // 向底层连接绑定用户id
                
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
                SendToConn(conn, omnibox::ID_LOGIN_RSP, response->SerializeAsString());
            }
            else
            {
                // 登录失败，强制断开连接
                LOG_INFO << "[Gateway] Player " << username << " login failed! Error code: " << response->errcode();
                
                // 由于发送操作是异步的，所以关闭连接也就是下面调用的这个conn函数也必须要是异步的，这样才能通过事件循环排队的方式强制同步
                SendToConn(conn, omnibox::ID_LOGIN_RSP, response->SerializeAsString());
                conn->ForceClose();
            }
        };

    // 3. 转发登录请求
    auto controler = std::make_shared<MyController>();
    auto rsp = std::make_shared<LoginResponse>();
    auto closure = new TcpRpcClosure<LoginResponse>(conn, controler, rsp, success_cb);
    LoginService_Stub stub(login_channel_.get());
    stub.Login(controler.get(), req.get(), rsp.get(), closure);
}

// ================== 推送响应回包 ==================
void GatewayTcpServer::SendToConn(const std::shared_ptr<TcpConnection>& conn, uint32_t msg_id, const std::string& pb_data)
{
    uint32_t res_len = 4 + pb_data.size();
    uint32_t net_res_len = htonl(res_len);
    uint32_t net_msg_id = htonl(msg_id);

    std::string send_buf;
    send_buf.append((char*)&net_res_len, 4);
    send_buf.append((char*)&net_msg_id, 4);
    send_buf.append(pb_data);

    conn->Send(send_buf);
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

    if (target_conn)
    {
        // 组装外网协议发给他 (长度 + MsgID + 数据)
        SendToConn(target_conn, msg_type, content);
        return true;
    }
    return false; // 玩家不在线
}