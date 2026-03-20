#include <iostream>
#include <thread>
#include <chrono>
#include "RPCServer.h"
#include "MyGatewayService.h"
#include "GatewayTcpServer.h"

int main(int argc, char** argv)
{
    // 1. 启动外网网关
    EventLoop client_loop;
    GatewayTcpServer tcp_server(&client_loop, "0.0.0.0", 8001);

    // 2. 内网RPC
    std::thread rpc_thread([&tcp_server]()
        {
            LOG_INFO << "=== Gateway Server is starting on port 8080 ===";
            RPCServer gateway_rpc_server("127.0.0.1", 8080);
            MyGatewayService gateway_service(&tcp_server);
            gateway_rpc_server.RegisterService(&gateway_service);
            gateway_rpc_server.Run();
        });

    rpc_thread.detach();

    tcp_server.Start(4);
    client_loop.Loop();

    return 0;
}