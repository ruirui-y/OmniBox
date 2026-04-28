#ifndef GATEWAY_HTTP_SERVER_H
#define GATEWAY_HTTP_SERVER_H

#include <mymuduo/net/TcpServer.h>
#include <mymuduo/net/EventLoop.h>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <string>
#include <functional>
#include <mymuduo/base/ThreadPool.h>
#include "MyChannel.h"

// HTTP 路由处理函数签名
using HttpHandler = std::function<void(
    const std::shared_ptr<TcpConnection>& conn,
    Buffer* buffer,
    const std::string& method,
    const std::string& uri,
    const std::string& headers,
    size_t header_end_pos
    )>;

class GatewayHttpServer
{
public:
    // 现在它自己就是一个服务器，需要传入 Loop、IP 和独立端口
    GatewayHttpServer(EventLoop* loop, const std::string& ip, uint16_t port);
    ~GatewayHttpServer() = default;

    // 启动 HTTP 服务，分配工作线程
    void Start(int thread_num);

private:
    // 拥有独立的网络事件回调
    void OnConnection(const std::shared_ptr<TcpConnection>& conn);
    void OnMessage(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer);

    // 注册路由
    void RegisterHttpHandler();

    // ==========================================
    // 具体的 HTTP 业务处理函数 (仅声明，业务逻辑在 cpp 中实现)
    // ==========================================
    void HandleStaticResource(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer, const std::string& method,
        const std::string& uri, const std::string& headers, size_t header_end_pos);

    // 鉴权与心跳
    void HandleLogin(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer, const std::string& method,
        const std::string& uri, const std::string& headers, size_t header_end_pos);
    void HandleHeartbeat(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer, const std::string& method,
        const std::string& uri, const std::string& headers, size_t header_end_pos);

    // 文件传输
    void HandleUploadChunk(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer, const std::string& method,
        const std::string& uri, const std::string& headers, size_t header_end_pos);
    void HandleCheckFile(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer, const std::string& method,
        const std::string& uri, const std::string& headers, size_t header_end_pos);

    // 文件树操作
    void HandleListDirectory(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer, const std::string& method,
        const std::string& uri, const std::string& headers, size_t header_end_pos);
    void HandleCreateFolder(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer, const std::string& method,
        const std::string& uri, const std::string& headers, size_t header_end_pos);
    void HandleDeleteNode(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer, const std::string& method,
        const std::string& uri, const std::string& headers, size_t header_end_pos);
    void HandleRenameNode(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer, const std::string& method,
        const std::string& uri, const std::string& headers, size_t header_end_pos);
    void HandleMoveNode(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer, const std::string& method,
        const std::string& uri, const std::string& headers, size_t header_end_pos);

private:
    TcpServer server_;                                                                                                  // 专属底层网络引擎
    EventLoop* loop_;                                                                                                   // 全局事件循环

    std::unordered_map<std::string, HttpHandler> router_;                                                               // HTTP 路由表

    // 依赖的全局资源
    std::shared_ptr<MyChannel> login_channel_;
    std::shared_ptr<MyChannel> transfer_channel_;
    std::shared_ptr<MyChannel> meta_channel_;
    std::shared_ptr<ThreadPool> thread_pool_;
};

#endif