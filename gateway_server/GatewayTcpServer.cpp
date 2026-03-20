#include "GatewayTcpServer.h"
#include <mymuduo/Log/Logger.h>
#include <any>
#include <arpa/inet.h>
#include "client_gateway.pb.h"
#include "MsgID.h"
#include "login.pb.h"     
#include "RedisClient.h"     
#include "MyController.h"
using namespace std::placeholders;

GatewayTcpServer::GatewayTcpServer(EventLoop* loop, const std::string& ip, uint16_t port)
    : server_(loop, ip, port, 100), ip_(ip), port_(port)
{
    server_.SetConnectionCallback(std::bind(&GatewayTcpServer::OnConnection, this, _1));
    server_.SetMessageCallback(std::bind(&GatewayTcpServer::OnMessage, this, _1, _2));

    // ================== 路由表注册 ==================
    RegisterHandler(game::net::MSG_LOGIN_REQ, std::bind(&GatewayTcpServer::HandleLoginReq, this, _1, _2));

    // 建立长连接
    login_channel_ = std::make_shared<MyChannel>("127.0.0.1", 9090);
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
        if (conn->GetContext().has_value())
        {
            int32_t uid = std::any_cast<int32_t>(conn->GetContext());
            std::lock_guard<std::mutex> lock(session_mutex_);
            user_sessions_.erase(uid);
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
            buffer->retrieve(4);                                                                    // 清空len(4)
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
            break;
        }
    }
}

// ================== 具体的业务处理 (Handler) ==================
void GatewayTcpServer::HandleLoginReq(const std::shared_ptr<TcpConnection>& conn, const std::string& pb_data)
{
    // 1. 解包外网请求
    game::client::ClientLoginRequest client_req;
    if (!client_req.ParseFromString(pb_data)) {
        LOG_ERROR << "[Gateway] Failed to parse ClientLoginRequest!";
        return;
    }

    LOG_INFO << "[Gateway] User attempting login with account: " << client_req.username();

    // 2. 转换成内网RPC请求
    game::rpc::LoginRequest rpc_req;
    rpc_req.set_username(client_req.username());
    rpc_req.set_password(client_req.password());
    game::rpc::LoginResponse rpc_resp;
    
    // 3. 化身 RPC Client，向 LoginServer 发起登录请求
    game::rpc::LoginService_Stub stub(login_channel_.get());
    MyController controller;
    stub.Login(&controller, &rpc_req, &rpc_resp, nullptr);

    // 4. 检查底层网络是否报错
    if (controller.Failed())
    {
        LOG_ERROR << "[Gateway] RPC Login call failed:" << controller.ErrorText();
        game::client::ClientLoginResponse client_resp;
        client_resp.set_errcode(500);
        client_resp.set_errmsg("Gateway Internal RPC Error");

        std::string resp_data;
        client_resp.SerializeToString(&resp_data);
        SendToConn(conn, game::net::MSG_LOGIN_RESP, resp_data);

        return;
    }

    // 5. 解析 RPC 响应，组装成外网响应准备发给 UE5
    game::client::ClientLoginResponse client_resp;
    client_resp.set_errcode(rpc_resp.errcode());
    client_resp.set_errmsg(rpc_resp.errmsg());

    if (rpc_resp.errcode() == 0)
    {
        // 登录成功！拿到后端分配的真实 UID，绑定到当前长连接上
        int32_t real_uid = rpc_resp.user_id();
        conn->SetContext(real_uid);

        {
            std::lock_guard<std::mutex> lock(session_mutex_);
            user_sessions_[real_uid] = conn;
        }
        LOG_INFO << "[Gateway] " << client_req.username() << " login success! uid = " << real_uid;
        client_resp.set_user_id(real_uid);

        // 注册网关信息到redis的用户表中
        RedisClient redis;
        if (redis.Connect("127.0.0.1", 6379)) 
        {
            // 设置rpc网关地址信息
            std::string rpc_addr = "127.0.0.1:8080";
            redis.HSet("session:users", std::to_string(real_uid), rpc_addr);
            LOG_INFO << "[Gateway] " << client_req.username()  << " globally registered id : " << real_uid << " to Redis ->" << rpc_addr;
        }
        else 
        {
            LOG_ERROR << "[Gateway] Redis connect failed, global routing registration skipped!";
        }
    }

    // 6. 序列化外网响应，直接打回给客户端
    std::string resp_data;
    client_resp.SerializeToString(&resp_data);
    SendToConn(conn, game::net::MSG_LOGIN_RESP, resp_data);
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