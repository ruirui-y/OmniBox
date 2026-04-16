#include <iostream>
#include <thread>
#include <chrono>
#include <mymuduo/db/ConnectionPool.h>
#include "RPCServer.h"
#include "MyGatewayService.h"
#include "GatewayTcpServer.h"

int main(int argc, char** argv)
{
    EventLoop main_loop;                                                                        // 整个进程唯一的超级心脏

    // 1. 启动外网网关 (挂载到 main_loop)
    GatewayTcpServer tcp_server(&main_loop, "0.0.0.0", 8001);

    // 2. 启动内网 RPC (也挂载到 main_loop！)
    RPCServer gateway_rpc_server(&main_loop, "127.0.0.1", 8080);
    MyGatewayService gateway_service(&tcp_server);
    gateway_rpc_server.RegisterService(&gateway_service);

    // 初始化数据库
    ConnectionPool::Instance().Init("127.0.0.1", "root", "123456", "omni_box");

    // 启动监听 (它们内部的 acceptor 开始工作)
    tcp_server.Start(4);                                                                        // 网关分配 4 个 Worker 线程
    gateway_rpc_server.Run(2);                                                                  // RPC 分配 2 个 Worker 线程

    LOG_INFO << "Server is fully started. Gateway: 8001, RPC: 8080";

    // 3. 开启超级心脏！(同时接管外网 8001 和内网 8080 的新连接)
    main_loop.Loop();

    return 0;
}