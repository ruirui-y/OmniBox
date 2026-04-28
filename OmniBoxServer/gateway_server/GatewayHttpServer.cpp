#include "GatewayHttpServer.h"
#include <string.h>
#include <fstream>
#include <sstream>
#include <regex>
#include <any>
#include <nlohmann/json.hpp>
#include <arpa/inet.h>
#include <mymuduo/Log/Logger.h>
#include <mymuduo/db/DbExecutor.h>
#include "RedisClient.h"
#include "server_msg.pb.h"
#include "internal_rpc.pb.h"
#include "MyController.h"
#include "HttpRpcClosure.h"

using namespace std::placeholders;
using json = nlohmann::json;
using namespace omnibox;

GatewayHttpServer::GatewayHttpServer(EventLoop* loop, const std::string& ip, uint16_t port)
    : server_(loop, ip, port, 100), loop_(loop)
{
    // 1. 绑定 HTTP 服务器的网络回调
    server_.SetConnectionCallback(std::bind(&GatewayHttpServer::OnConnection, this, _1));
    server_.SetMessageCallback(std::bind(&GatewayHttpServer::OnMessage, this, _1, _2));

    // 2. 👑 自己创建打向后端微服务的 RPC 通道 (独立连接)
    login_channel_ = std::make_shared<MyChannel>("127.0.0.1", 9090);
    // transfer_channel_ = std::make_shared<MyChannel>("127.0.0.1", 8082);                                              
    // meta_channel_ = std::make_shared<MyChannel>("127.0.0.1", 8090);                                                  

    // 3. 👑 自己创建并启动专属的 HTTP 业务线程池
    thread_pool_ = std::make_shared<ThreadPool>("http_gateway_pool");
    thread_pool_->start(4);

    // 4. 注册所有 HTTP 接口的路由
    RegisterHttpHandler();
}

void GatewayHttpServer::Start(int thread_num)
{
    server_.Start(thread_num);
}

void GatewayHttpServer::RegisterHttpHandler()
{
    router_["POST /api/login"] = std::bind(&GatewayHttpServer::HandleLogin, this, _1, _2, _3, _4, _5, _6);
    router_["POST /api/heartbeat"] = std::bind(&GatewayHttpServer::HandleHeartbeat, this, _1, _2, _3, _4, _5, _6);
    router_["POST /upload_chunk"] = std::bind(&GatewayHttpServer::HandleUploadChunk, this, _1, _2, _3, _4, _5, _6);
    router_["GET /check_file"] = std::bind(&GatewayHttpServer::HandleCheckFile, this, _1, _2, _3, _4, _5, _6);
    router_["GET /api/list_dir"] = std::bind(&GatewayHttpServer::HandleListDirectory, this, _1, _2, _3, _4, _5, _6);
    router_["POST /api/create_folder"] = std::bind(&GatewayHttpServer::HandleCreateFolder, this, _1, _2, _3, _4, _5, _6);
    router_["POST /api/delete_node"] = std::bind(&GatewayHttpServer::HandleDeleteNode, this, _1, _2, _3, _4, _5, _6);
    router_["POST /api/rename_node"] = std::bind(&GatewayHttpServer::HandleRenameNode, this, _1, _2, _3, _4, _5, _6);
    router_["POST /api/move_node"] = std::bind(&GatewayHttpServer::HandleMoveNode, this, _1, _2, _3, _4, _5, _6);
}

void GatewayHttpServer::OnConnection(const std::shared_ptr<TcpConnection>& conn)
{
    if (conn->Connected())
    {
    }
    else
    {
    }
}


// ==============================================================================
/* 💡 真实 TCP 字节流形态 (浏览器发来的任意请求)：

   METHOD /PATH?PARAMS HTTP/1.1\r\n
   Header1: Value1\r\n
   Header2: Value2\r\n
   \r\n
   <可选的 Body 二进制包体>
*/
// ==============================================================================
void GatewayHttpServer::OnMessage(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer)
{
    // 获取当前缓冲区的全部数据（不消费）
    std::string raw_data = std::string(buffer->peek(), buffer->ReadableBytes());

    // 1. 查找 HTTP 头部和包体的分界线 \r\n\r\n
    size_t header_end_pos = raw_data.find("\r\n\r\n");
    if (header_end_pos == std::string::npos)
    {
        return; // TCP 半包，等下一次数据到达
    }

    // 2. 解析第一行 (例如: POST /api/login HTTP/1.1)
    std::string headers = raw_data.substr(0, header_end_pos);
    char method[16], uri[256], version[16];
    sscanf(headers.c_str(), "%s %s %s", method, uri, version);

    // 3. 剥离 URL 参数 (将 /check_file?hash=123 变成 /check_file)
    std::string full_uri = uri;
    std::string base_uri = full_uri;
    size_t question_pos = full_uri.find('?');
    if (question_pos != std::string::npos) {
        base_uri = full_uri.substr(0, question_pos);
    }

    // 4. 构造路由键 (例如 "POST /api/login")
    std::string route_key = std::string(method) + " " + base_uri;

    // 5. 路由分发
    auto it = router_.find(route_key);
    if (it != router_.end())
    {
        // 命中 API 路由，执行对应的业务处理函数
        it->second(conn, buffer, method, full_uri, headers, header_end_pos);
    }
    else if (strcmp(method, "GET") == 0)
    {
        // 兜底处理静态资源下载
        HandleStaticResource(conn, buffer, method, full_uri, headers, header_end_pos);
    }
    else
    {
        // 非法请求处理
        LOG_ERROR << "[GatewayHttp] ❌ 404 Not Found: " << route_key;
        conn->Send("HTTP/1.1 404 Not Found\r\n\r\n 404: Route not found");
        buffer->RetrieveAllAsString(); // 消费掉这个错误的包
        conn->ForceClose();
    }
}

// ==============================================================================
/* 💡 真实 TCP 字节流形态 (请求静态资源 /main.html)：

   GET /main.html HTTP/1.1\r\n
   Host: 127.0.0.1:8080\r\n
   Connection: keep-alive\r\n
   User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/120.0.0.0 Safari/537.36\r\n
   Accept: text/html\r\n
   \r\n
   (注意：GET 请求没有任何 Body 包体，以 \r\n\r\n 彻底结束)
*/
// ==============================================================================
void GatewayHttpServer::HandleStaticResource(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer, const std::string& method, const std::string& uri, const std::string& headers, size_t header_end_pos)
{
    // 1. 动态寻址：直接用浏览器发来的 uri 拼接本地路径
    std::string target_file = "./www";
    if (uri == "/")
    {
        target_file += "/main.html"; // 根目录默认打向主页
    }
    else
    {
        target_file += uri;          // 比如 uri 是 "/uploader.js"，这里变成 "./www/uploader.js"
    }

    // 2. 尝试打开浏览器索要的这个文件
    std::ifstream file(target_file);
    if (file.is_open())
    {
        // 3. 简易 MIME 推演机制
        std::string content_type = "text/plain; charset=utf-8"; // 默认纯文本
        if (target_file.find(".html") != std::string::npos) content_type = "text/html; charset=utf-8";
        else if (target_file.find(".js") != std::string::npos) content_type = "application/javascript; charset=utf-8";
        else if (target_file.find(".css") != std::string::npos) content_type = "text/css; charset=utf-8";
        else if (target_file.find(".png") != std::string::npos) content_type = "image/png";

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
        LOG_ERROR << "❌ 404 拦截: 试图访问不存在的静态资源 -> " << target_file;
        conn->Send("HTTP/1.1 404 Not Found\r\n\r\n 404: 找不到该前端资源！");
    }

    conn->ForceClose();
}

void GatewayHttpServer::HandleLogin(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer, const std::string& method,
    const std::string& uri, const std::string& headers, size_t header_end_pos)
{
    // 1. TCP 半包与粘包防御 (解析 Content-Length)
    size_t cl_pos = headers.find("Content-Length: ");
    if (cl_pos == std::string::npos) {
        conn->Send("HTTP/1.1 400 Bad Request\r\n\r\n Missing Content-Length");
        return;
    }

    size_t cl_end = headers.find("\r\n", cl_pos);
    int content_length = std::stoi(headers.substr(cl_pos + 16, cl_end - cl_pos - 16));
    size_t total_expected_bytes = header_end_pos + 4 + content_length;

    // 如果网卡收到的数据还不够一个完整的 HTTP 报文，直接 return，等下次 epoll 唤醒
    if (buffer->ReadableBytes() < total_expected_bytes) {
        return;
    }

    // 2. 提取纯 JSON 字符串
    std::string full_request = buffer->RetrieveAsString(total_expected_bytes);
    std::string json_body_str = full_request.substr(header_end_pos + 4);

    // 3. 解析 JSON
    json req_json = json::parse(json_body_str, nullptr, false);
    if (req_json.is_discarded()) {
        conn->Send("HTTP/1.1 400 Bad Request\r\n\r\n Invalid JSON format");
        return;
    }

    // 4. 组装发往 LoginService 的 RPC 请求
    // 注意：假设你的 LoginRequest 是在全局命名空间，如果是 game::rpc::LoginRequest 请补全
    ::LoginRequest rpc_req;
    rpc_req.set_username(req_json.value("username", ""));
    rpc_req.set_password(req_json.value("password", ""));

    auto rpc_resp = std::make_shared<::LoginResponse>();
    auto controller = std::make_shared<MyController>();

    // 5. 👑 定义 RPC 调用成功后，如何回复前端的 HTTP 请求
    std::function<void(const TcpConnectionPtr&, const std::shared_ptr<::LoginResponse>&)> success_callback =
        [](const TcpConnectionPtr& conn, const std::shared_ptr<::LoginResponse>& resp)
        {
            json res_json;
            // 这里的字段名必须和你的 AuthManager.js 里期待的一模一样！
            res_json["errcode"] = resp->errcode();
            res_json["errmsg"] = resp->errmsg();

            // 只有登录成功时，才下发敏感凭证
            if (resp->errcode() == 0) {
                res_json["token"] = resp->token();
                res_json["user_id"] = resp->user_id();
            }

            std::string body = res_json.dump();
            std::string http_response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json; charset=utf-8\r\n"
                "Content-Length: " + std::to_string(body.length()) + "\r\n"
                "Connection: keep-alive\r\n\r\n" + body;

            conn->Send(http_response);
        };

    auto done = new HttpRpcClosure<::LoginResponse>(conn, controller, rpc_resp, success_callback);

    // 6. 召唤 Stub 发送 RPC 请求
    LoginService_Stub stub(login_channel_.get());
    stub.Login(controller.get(), &rpc_req, rpc_resp.get(), done);
}

void GatewayHttpServer::HandleHeartbeat(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer, const std::string& method,
    const std::string& uri, const std::string& headers, size_t header_end_pos)
{
    // 1. TCP 半包与粘包防御：提取 Content-Length
    size_t cl_pos = headers.find("Content-Length: ");
    if (cl_pos == std::string::npos) {
        conn->Send("HTTP/1.1 400 Bad Request\r\n\r\n Missing Content-Length");
        return;
    }

    size_t cl_end = headers.find("\r\n", cl_pos);
    int content_length = std::stoi(headers.substr(cl_pos + 16, cl_end - cl_pos - 16));
    size_t total_expected_bytes = header_end_pos + 4 + content_length;

    // 数据未收全，等待下一次可读事件
    if (buffer->ReadableBytes() < total_expected_bytes) {
        return;
    }

    // 2. 提取并清理纯 JSON 字符串
    std::string full_request = buffer->RetrieveAsString(total_expected_bytes);
    std::string json_body_str = full_request.substr(header_end_pos + 4);

    // 3. 反序列化 JSON
    json req_json = json::parse(json_body_str, nullptr, false);
    if (req_json.is_discarded()) {
        conn->Send("HTTP/1.1 400 Bad Request\r\n\r\n Invalid JSON format");
        return;
    }

    // 4. 组装 RPC 请求发往 LoginService
    HeartbeatRequest rpc_req;
    // ⚠️ 注意：这里的键名 "user_id" 和 "token" 必须和前端 JS 中 body: JSON.stringify({...}) 里的保持完全一致！
    rpc_req.set_user_id(req_json.value("user_id", 0LL));
    rpc_req.set_token(req_json.value("token", ""));

    auto rpc_resp = std::make_shared<HeartbeatResponse>();
    auto controller = std::make_shared<MyController>();

    // 5. 定义 RPC 成功后的回调：将服务端处理结果封装成 HTTP JSON 返回给前端
    std::function<void(const TcpConnectionPtr&, const std::shared_ptr<HeartbeatResponse>&)> success_callback =
        [](const TcpConnectionPtr& conn, const std::shared_ptr<HeartbeatResponse>& resp)
        {
            json res_json;
            res_json["success"] = resp->success();

            // 如果心跳成功，顺便把服务端的时间戳下发给前端（前端可以用作本地时间校准）
            if (resp->success()) {
                res_json["server_time"] = resp->server_time();
            }

            std::string body = res_json.dump();
            std::string http_response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json; charset=utf-8\r\n"
                "Content-Length: " + std::to_string(body.length()) + "\r\n"
                "Connection: keep-alive\r\n\r\n" + body;

            conn->Send(http_response);
        };

    auto done = new HttpRpcClosure<HeartbeatResponse>(conn, controller, rpc_resp, success_callback);

    // 6. 通过 LoginService 的专属 Channel 发送 RPC 请求
    // 假设你在网关初始化时，给登录服务建的通道叫 login_channel_
    LOG_INFO << "dis heart request";
    LoginService_Stub stub(login_channel_.get());
    stub.Heartbeat(controller.get(), &rpc_req, rpc_resp.get(), done);
}

// ==============================================================================
/* 💡 真实 TCP 字节流形态 (架构 2.0 切片上传)：

   POST /upload_chunk HTTP/1.1\r\n
   Host: 127.0.0.1:8080\r\n
   Content-Length: 1048576\r\n                  👈 我们必须抓取这个数字！
   Content-Type: application/octet-stream\r\n
   X-File-Hash: 1073741824_abc123...\r\n        👈 提取文件总哈希
   X-Block-Hash: def456...\r\n                  👈 提取切片哈希
   X-Chunk-Index: 0\r\n                         👈 提取块序号
   X-File-Name: data.bin\r\n
   X-File-Offset: 0\r\n
   X-File-Eof: 0\r\n
   X-File-Size: 1073741824\r\n
   \r\n
   <这里紧紧跟着 1048576 个纯二进制的乱码字节 (Body)>
*/
// ==============================================================================
void GatewayHttpServer::HandleUploadChunk(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer, const std::string& method, const std::string& uri, const std::string& headers, size_t header_end_pos)
{
    // 1. 解析Content-Length
    size_t cl_pos = headers.find("Content-Length: ");
    if (cl_pos == std::string::npos) return;

    size_t cl_end = headers.find("\r\n", cl_pos);
    int content_length = std::stoi(headers.substr(cl_pos + 16, cl_end - cl_pos - 16));

    // 2. 判断Body数据是否全部到达网卡 (TCP 半包粘包核心防线)
    size_t total_expected_bytes = header_end_pos + 4 + content_length;
    if (buffer->ReadableBytes() < total_expected_bytes)
    {
        // 数据还没收全，直接 return 等待下一次 read 触发
        return;
    }

    // 3. 从header中提取键值 (Lambda 辅助函数)
    auto get_header_value = [&](const std::string& key) ->std::string {
        size_t pos = headers.find(key + ": ");
        if (pos == std::string::npos) return "";
        size_t end = headers.find("\r\n", pos);
        return headers.substr(pos + key.length() + 2, end - pos - key.length() - 2);
        };

    std::string file_hash = get_header_value("X-File-Hash");
    std::string block_hash = get_header_value("X-Block-Hash");
    std::string chunk_index_str = get_header_value("X-Chunk-Index");
    std::string file_name = get_header_value("X-File-Name");
    int64_t offset = std::stoll(get_header_value("X-File-Offset"));
    bool is_eof = (get_header_value("X-File-Eof") == "1");
    int64_t file_size = std::stoll(get_header_value("X-File-Size"));

    // 4. 提取二进制文件包体
    std::string full_request = buffer->RetrieveAsString(total_expected_bytes);
    std::string pure_chunk = full_request.substr(header_end_pos + 4);

    LOG_INFO << "🔪 [透传引擎] 收到碎片 | 文件: " << file_name
        << " | 块序号: " << chunk_index_str
        << " | Size: " << pure_chunk.size() << " bytes";

    // 5. 转交给文件服务进行处理 (RPC 调用)
    omnibox::FileChunkUploadRequest rpc_req;
    // 逻辑层信息赋值
    rpc_req.set_file_hash(file_hash);
    rpc_req.set_file_name(file_name);
    rpc_req.set_total_size(file_size);

    // 物理层切片赋值
    rpc_req.set_block_hash(block_hash);
    rpc_req.set_chunk_index(std::stoi(chunk_index_str)); // 注意把字符串转成整型
    rpc_req.set_data(pure_chunk);
    rpc_req.set_offset(offset);

    auto rpc_resp = std::make_shared<omnibox::FileChunkUploadResponse>();
    auto controller = std::make_shared<MyController>();

    // 成功回调闭包
    std::function<void(const TcpConnectionPtr&, const std::shared_ptr<omnibox::FileChunkUploadResponse>&)> success_callback =
        [](const TcpConnectionPtr& conn, const std::shared_ptr<omnibox::FileChunkUploadResponse>& resp)
        {
            if (resp->success())
            {
                // 如果触发了物理层块级去重，打印一下炫技日志
                if (resp->is_duplicate())
                {
                    LOG_INFO << "🎯 [块级秒传] 碎片复用成功，未产生磁盘 I/O: " << resp->message();
                }
                else
                {
                    LOG_INFO << "💾 [落盘成功] 新碎片已写入物理磁盘: " << resp->message();
                }

                // 必须给前端的 fetch 返回一个 HTTP 200 OK，前端的 Promise 才会 resolve，滑动窗口才会继续往前走！
                conn->Send("HTTP/1.1 200 OK\r\nConnection: keep-alive\r\n\r\n");
            }
            else {
                LOG_ERROR << "❌ [RPC 失败] TransferServer 拒绝了碎片: " << resp->message();
                // 通知前端出错，让前端重试
                conn->Send("HTTP/1.1 500 Internal Server Error\r\nConnection: keep-alive\r\n\r\n");
            }
        };
    auto done = new HttpRpcClosure<omnibox::FileChunkUploadResponse>(conn, controller, rpc_resp, success_callback);

    omnibox::TransferService_Stub stub(transfer_channel_.get());
    stub.UploadChunk(controller.get(), &rpc_req, rpc_resp.get(), done);
}

static std::string UrlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%') {
            int value;
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &value);
            result += static_cast<char>(value);
            i += 2;
        }
        else if (str[i] == '+') {
            result += ' ';
        }
        else {
            result += str[i];
        }
    }
    return result;
}

// ==============================================================================
/* 💡 真实 TCP 字节流形态 (网关查岗 /check_file)：

   GET /check_file?hash=1073741824_abc123&name=data.bin HTTP/1.1\r\n   👈 参数全塞在第一行的 URI 里
   Host: 127.0.0.1:8080\r\n
   Connection: keep-alive\r\n
   \r\n
*/
// ==============================================================================
void GatewayHttpServer::HandleCheckFile(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer, const std::string& method, const std::string& uri, const std::string& headers, size_t header_end_pos)
{
    // 1. 简易的 URL 参数解析 (提取 ?hash=xxx&name=yyy)
    auto get_uri_param = [&](const std::string& key) -> std::string {
        size_t pos = uri.find(key + "=");
        if (pos == std::string::npos) return "";
        size_t end_pos = uri.find("&", pos);
        if (end_pos == std::string::npos) end_pos = uri.length();
        return uri.substr(pos + key.length() + 1, end_pos - pos - key.length() - 1);
        };

    std::string file_hash = get_uri_param("hash");
    std::string file_name = UrlDecode(get_uri_param("name"));

    // 提前清理 Buffer
    buffer->RetrieveAsString(header_end_pos + 4);

    if (file_hash.empty() || file_name.empty()) {
        conn->Send("HTTP/1.1 400 Bad Request\r\n\r\n");
        return;
    }

    LOG_INFO << "📡 [秒传查岗] 用户试图上传文件: " << file_name << " | 哈希: " << file_hash;

    // 🚨 架构师技巧：我们之前的急速哈希是 "文件大小_哈希串" (如 "1048576_abc123")
    // 这里顺手把文件大小切出来，建表的时候要用！
    int64_t file_size = 0;
    size_t underscore_pos = file_hash.find('_');
    if (underscore_pos != std::string::npos) {
        file_size = std::stoll(file_hash.substr(0, underscore_pos));
    }

    // [模拟用户环境] 实际项目中，这两个值应该从 HTTP 请求头的 Token (如 JWT) 里解析出来
    int64_t current_user_id = 1001;
    int64_t current_parent_id = 0;  // 假设上传到用户的根目录

    // ==========================================
    // 2. 第一步：异步去库里查这个 Hash 存不存在
    // ==========================================
    std::string query_sql = "SELECT 1 FROM virtual_file_node WHERE file_hash = ? AND status = 1 LIMIT 1";
    DbParams query_params = { file_hash };

    // 注意：Lambda 捕获列表中，要把接下来需要用到的变量全部按值传进去 [=] 或者显式捕获
    DbExecutor::AsyncQuery(loop_, thread_pool_.get(), query_sql, query_params,
        [this, conn, file_hash, file_name, file_size, current_user_id, current_parent_id](const DbResultSet& results)
        {
            bool is_exist = !results.empty();

            if (is_exist)
            {
                LOG_INFO << "🎉 [秒传命中] 全网数据库中已存在该文件: " << file_hash << "，准备执行虚拟挂载...";

                // ==========================================
                // 3. 第二步：嵌套发起异步 INSERT 操作，完成“伪装建房”
                // ==========================================
                std::string insert_sql =
                    "INSERT INTO virtual_file_node "
                    "(user_id, parent_id, node_name, is_dir, file_hash, file_size, status) "
                    "VALUES (?, ?, ?, 0, ?, ?, 1)";

                DbParams insert_params = { current_user_id, current_parent_id, file_name, file_hash, file_size };

                // 继续向线程池派发写库任务
                DbExecutor::AsyncUpdate(loop_, thread_pool_.get(), insert_sql, insert_params,
                    [conn](int affectedRows, int lastInsertId)
                    {
                        if (affectedRows > 0) {
                            LOG_INFO << "✅ [秒传完成] 虚拟文件树挂载成功！(无物理 I/O 产生)";
                        }
                        else {
                            LOG_ERROR << "❌ [秒传失败] 写入 virtual_file_node 发生异常！";
                        }

                        // 无论如何，只要到了这一步，就告诉前端秒传成功
                        std::string response_body = "{\"status\": \"exists\"}";
                        std::string http_response =
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: application/json\r\n"
                            "Connection: keep-alive\r\n"
                            "Content-Length: " + std::to_string(response_body.length()) + "\r\n\r\n"
                            + response_body;

                        conn->Send(http_response);
                    });
            }
            else
            {
                LOG_INFO << "🧱 [秒传未命中] 数据库无此文件，准备迎接切片洪流...";

                // 库里没查到，直接回绝，让前端开始发切片
                std::string response_body = "{\"status\": \"not_found\"}";
                std::string http_response =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: application/json\r\n"
                    "Connection: keep-alive\r\n"
                    "Content-Length: " + std::to_string(response_body.length()) + "\r\n\r\n"
                    + response_body;

                conn->Send(http_response);
            }
        });
}

void GatewayHttpServer::HandleListDirectory(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer, const std::string& method, const std::string& uri, const std::string& headers, size_t header_end_pos)
{
    // 1. 提取 URI 参数
    auto get_uri_param = [&](const std::string& key) -> std::string {
        size_t pos = uri.find(key + "=");
        if (pos == std::string::npos) return "";
        size_t end = uri.find("&", pos);
        if (end == std::string::npos) end = uri.length();
        return uri.substr(pos + key.length() + 1, end - pos - key.length() - 1);
        };

    std::string parent_id_str = get_uri_param("parent_id");
    int64_t parent_id = parent_id_str.empty() ? 0 : std::stoll(parent_id_str);

    // GET 请求没有 Body，直接清空 buffer 即可
    buffer->RetrieveAsString(header_end_pos + 4);

    // 2. 组装 RPC 请求
    omnibox::ListDirectoryRequest rpc_req;
    rpc_req.set_user_id(1001); // 模拟当前登录用户
    rpc_req.set_parent_id(parent_id);

    auto rpc_resp = std::make_shared<omnibox::ListDirectoryResponse>();
    auto controller = std::make_shared<MyController>();

    // 3. RPC 成功回调：将 Protobuf 数组转成 JSON 返回给前端
    std::function<void(const TcpConnectionPtr&, const std::shared_ptr<omnibox::ListDirectoryResponse>&)> success_callback =
        [](const TcpConnectionPtr& conn, const std::shared_ptr<omnibox::ListDirectoryResponse>& resp)
        {
            json response_json;
            response_json["success"] = resp->success();
            response_json["message"] = resp->message();
            response_json["nodes"] = json::array(); // 初始化为空数组

            for (int i = 0; i < resp->nodes_size(); ++i) {
                const auto& node = resp->nodes(i);
                json node_json;
                node_json["node_id"] = node.node_id();
                node_json["node_name"] = node.node_name();
                node_json["is_dir"] = node.is_dir();
                node_json["file_size"] = node.file_size();
                node_json["update_time"] = node.update_time();
                node_json["file_hash"] = node.file_hash();
                response_json["nodes"].push_back(node_json);
            }

            std::string body = response_json.dump();
            std::string http_response = "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: " + std::to_string(body.length()) + "\r\nConnection: keep-alive\r\n\r\n" + body;
            conn->Send(http_response);
        };

    auto done = new HttpRpcClosure<omnibox::ListDirectoryResponse>(conn, controller, rpc_resp, success_callback);
    omnibox::MetaService_Stub stub(meta_channel_.get());
    stub.ListDirectory(controller.get(), &rpc_req, rpc_resp.get(), done);
}

void GatewayHttpServer::HandleCreateFolder(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer, const std::string& method, const std::string& uri, const std::string& headers, size_t header_end_pos)
{

}

void GatewayHttpServer::HandleDeleteNode(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer, const std::string& method, const std::string& uri, const std::string& headers, size_t header_end_pos)
{

}

void GatewayHttpServer::HandleRenameNode(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer, const std::string& method, const std::string& uri, const std::string& headers, size_t header_end_pos)
{

}

void GatewayHttpServer::HandleMoveNode(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer, const std::string& method, const std::string& uri, const std::string& headers, size_t header_end_pos)
{

}