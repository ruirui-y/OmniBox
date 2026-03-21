#include "GatewayTcpServer.h"
#include <mymuduo/Log/Logger.h>
#include <any>
#include <arpa/inet.h>
#include <fstream>
#include <sstream>
#include <regex>
#include "client_gateway.pb.h"
#include "MsgID.h"
#include "login.pb.h"     
#include "RedisClient.h"   
#include "transfer.pb.h"
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
    transfer_channel_ = std::make_shared<MyChannel>("127.0.0.1", 8082);
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
    // 判断是不是http请求
    if (buffer->ReadableBytes() >= 4)
    {
        // 核心：只看不取 (peek)。绝对不能用 retrieve，否则原来 RPC 的包头就被吞了！
        std::string header_peek = std::string(buffer->peek(), 4);

        if (header_peek == "GET " || header_peek == "POST")
        {
            // 嗅探命中！这是来自浏览器的 HTTP 请求
            HandleHttpRequest(conn, buffer);
            return;                                                                                 
        }
    }

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

void GatewayTcpServer::HandleHttpRequest(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer)
{
    // ==========================================
    // 阶段 1：TCP 半包拦截与头部解析
    // ==========================================
    std::string raw_data = std::string(buffer->peek(), buffer->ReadableBytes());

    // 查找HTTP头部和包体的分界线
    size_t header_end_pos = raw_data.find("\r\n\r\n");
    if (header_end_pos == std::string::npos)
    {
        // 没有收齐头部
        return;
    }

    // 分离Header文本
    std::string headers = raw_data.substr(0, header_end_pos);

    // 解析出请求方法和URI
    char method[16], uri[256], version[16];
    sscanf(headers.c_str(), "%s %s %s", method, uri, version);

    // ==========================================
    // 阶段 2：路由分发
    // ==========================================

    if (strcmp(method, "GET") == 0)
    {
        // 1. 动态寻址：直接用浏览器发来的 uri 拼接本地路径
        std::string target_file = "./www";
        if (strcmp(uri, "/") == 0) 
        {
            target_file += "/main.html";                                    // 根目录默认打向主页
        }
        else
        {
            target_file += uri;                                             // 比如 uri 是 "/uploader.js"，这里自动变成 "./www/uploader.js"
        }

        // 2. 尝试打开浏览器索要的这个文件
        std::ifstream file(target_file);
        if (file.is_open())
        {
            // 3. 简易 MIME 推演机制 (告诉浏览器该怎么解析这堆二进制)
            std::string content_type = "text/plain; charset=utf-8"; // 默认纯文本
            if (target_file.find(".html") != std::string::npos) {
                content_type = "text/html; charset=utf-8";
            }
            else if (target_file.find(".js") != std::string::npos) {
                content_type = "application/javascript; charset=utf-8";
            }
            else if (target_file.find(".css") != std::string::npos) {
                content_type = "text/css; charset=utf-8";
            }
            else if (target_file.find(".png") != std::string::npos) {
                content_type = "image/png";
            }

            // 4. 读取文件内容
            std::stringstream file_buffer;
            file_buffer << file.rdbuf();
            std::string file_content = file_buffer.str();

            // 5. 返回文件
            std::string http_response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: " + content_type + "\r\n"
                "Connection: close\r\n\r\n" + file_content;

            conn->Send(http_response);
            buffer->RetrieveAllAsString(); // 清空buffer
        }
        else
        {
            LOG_ERROR << "❌ 404 拦截: 试图访问不存在的资源 -> " << target_file;
            conn->Send("HTTP/1.1 404 Not Found\r\n\r\n 404: 找不到该前端资源！");
        }

        conn->ForceClose();
    }
    else if (strcmp(method, "POST") == 0 && strcmp(uri, "/upload_chunk") == 0)
    {
        // 1. 解析Content-Length
        size_t cl_pos = headers.find("Content-Length: ");
        if (cl_pos == std::string::npos) return;

        size_t cl_end = headers.find("\r\n", cl_pos);
        int content_length = std::stoi(headers.substr(cl_pos + 16, cl_end - cl_pos - 16));

        // 2. 判断Body数据是否全部到达网卡
        size_t total_expected_bytes = header_end_pos + 4 + content_length;
        if (buffer->ReadableBytes() < total_expected_bytes)
        {
            // LOG_INFO << "正在接收大文件碎片... 目前进度: "
                //<< buffer->ReadableBytes() << " / " << total_expected_bytes;
            return;
        }

        // 文件接收完毕

        // 3. 从header中提取键值
        auto get_header_value = [&](const std::string& key) ->std::string
            {
                size_t pos = headers.find(key + ": ");
                if (pos == std::string::npos) return "";
                size_t end = headers.find("\r\n", pos);
                return headers.substr(pos + key.length() + 2, end - pos - key.length() - 2);
            };

        std::string file_name = get_header_value("X-File-Name");
        int64_t offset = std::stoll(get_header_value("X-File-Offset"));
        bool is_eof = (get_header_value("X-File-Eof") == "1");

        // 4. 提取二进制文件
        std::string full_request = buffer->RetrieveAsString(total_expected_bytes);
        std::string pure_chunk = full_request.substr(header_end_pos + 4);

        LOG_INFO << "🔪 [透传引擎] 收到碎片 | 文件: " << file_name
            << " | Offset: " << offset
            << " | Size: " << pure_chunk.size() << " bytes | EOF: " << is_eof;

        // 5. 转交给文件服务进行处理
        omnibox::FileChunkUploadRequest rpc_req;
        rpc_req.set_file_name(file_name);
        rpc_req.set_offset(offset);
        rpc_req.set_data(pure_chunk);
        rpc_req.set_is_eof(is_eof);

        omnibox::FileChunkUploadResponse rpc_resp;

        // rpc请求文件服务
        omnibox::TransferService_Stub stub(transfer_channel_.get());
        MyController controller;
        stub.UploadChunk(&controller, &rpc_req, &rpc_resp, nullptr);

        // 防御 1：RPC 链路断裂？
        if (controller.Failed())
        {
            LOG_ERROR << "[RPC 熔断] 连接 TransferServer 失败: " << controller.ErrorText();
            // 霸道断开：回发 HTTP 502 Bad Gateway，彻底切断前端的幻想
            std::string http_error = "HTTP/1.1 502 Bad Gateway\r\nConnection: close\r\n\r\n";
            conn->Send(http_error);
            return;
        }

        // 防御 2：TransferServer 业务拒绝？
        if (!rpc_resp.success())
        {
            LOG_ERROR << "[业务熔断] TransferServer 拒收碎片: " << rpc_resp.message();
            // 霸道断开：回发 HTTP 500 Internal Server Error
            std::string http_error = "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n";
            conn->Send(http_error);
            return;
        }

        // 5. 瞬间回发 HTTP 200 给浏览器，让它发射下一块！
        std::string http_response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n";
        conn->Send(http_response);
    }
    else
    {
        // 其他未知路由
        conn->Send("HTTP/1.1 404 Not Found\r\n\r\n 404: 路由未配置");
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