#ifndef GATEWAY_TCP_SERVER_H
#define GATEWAY_TCP_SERVER_H

#include <mymuduo/net/TcpServer.h>
#include <mymuduo/net/EventLoop.h>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <string>
#include <functional>
#include "MyChannel.h"

// 定义消息处理函数签名：入参是 (当前物理连接, 尚未反序列化的 Protobuf 纯二进制流)
using MsgHandler = std::function<void(const std::shared_ptr<TcpConnection>&, const std::string&)>;

class GatewayTcpServer
{
public:
	GatewayTcpServer(EventLoop* loop, const std::string& ip, uint16_t port);

    void Start(int thread_num);                                                                                         // 启动服务
    bool PushMessageToClient(int32_t uid, uint32_t msg_type, const std::string& content);                               // 提供给内部 RPC 调用的核心接口：精准推送消息给某个物理客户端

private:
    void OnConnection(const std::shared_ptr<TcpConnection>& conn);                                                      // 底层网络回调
    void OnMessage(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer);

private:    
    void RegisterHandler(uint32_t msg_id, MsgHandler handler);                                                          // 注册路由的回调函数
    
    // 处理HTTP请求
    void HandleHttpRequest(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer);

    // 具体的业务处理函数 (登录)
    void HandleLoginReq(const std::shared_ptr<TcpConnection>& conn, const std::string& pb_data);                        

    void SendToConn(const std::shared_ptr<TcpConnection>& conn, uint32_t msg_id, const std::string& pb_data);           // 发送数据
private:
    TcpServer server_;

    std::string ip_;
    uint16_t port_;

    // 会话管理器 (封装在类内部，绝对安全)
    std::mutex session_mutex_;
    std::unordered_map<int32_t, std::shared_ptr<TcpConnection>> user_sessions_;
    std::unordered_map<uint32_t, MsgHandler> msg_dispatcher_;                                                           // 事件分发

    // 网关全局共享的rpc通道
    std::shared_ptr<MyChannel> login_channel_;
};

#endif