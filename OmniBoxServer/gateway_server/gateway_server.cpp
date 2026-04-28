#include <iostream>
#include <thread>
#include <chrono>
#include <mymuduo/db/ConnectionPool.h>
#include <mymuduo/base/ThreadPool.h>
#include "RPCServer.h"
#include "MyGatewayService.h"
#include "GatewayTcpServer.h"
#include "GatewayHttpServer.h"


int main(int argc, char** argv)
{
    EventLoop main_loop;                                                                        // 整个进程唯一的超级心脏

    // 1. 启动外网 TCP 网关 (挂载到 main_loop，处理长连接与 Protobuf)
    GatewayTcpServer tcp_server(&main_loop, "0.0.0.0", 8001);

    // 2. 启动外网 HTTP 网关 (挂载到 main_loop，处理短连接、大文件上传)
    // GatewayHttpServer http_server(&main_loop, "0.0.0.0", 8002);

    // 3. 启动内网 RPC (也挂载到 main_loop！)
    RPCServer gateway_rpc_server(&main_loop, "127.0.0.1", 8080);
    MyGatewayService gateway_service(&tcp_server);
    gateway_rpc_server.RegisterService(&gateway_service);

    // 初始化数据库
    ConnectionPool::Instance().Init("127.0.0.1", "root", "123456", "omni_box");

    // 启动监听 (它们内部的 acceptor 开始工作)
    tcp_server.Start(4);                                                                        // TCP 网关分配 4 个 Worker 线程
    // http_server.Start(4);                                                                       // HTTP 网关也分配 4 个 Worker 线程
    gateway_rpc_server.Run(2);                                                                  // RPC 分配 2 个 Worker 线程

    LOG_INFO << "Server is fully started. TCP: 8001, HTTP: 8002, RPC: 8080";

    // 4. 开启超级心脏！(同时接管外网 8001, 8002 和内网 8080 的新连接)
    main_loop.Loop();

    return 0;
}