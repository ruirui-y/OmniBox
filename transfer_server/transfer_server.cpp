#include "RPCServer.h"
#include "TransferServiceImpl.h"
#include <mymuduo/Log/Logger.h>
#include <mymuduo/db/ConnectionPool.h>

int main(int argc, char** argv)
{
    // 假设你的内网环境，TransferServer 坚守在 8082 端口
    std::string ip = "127.0.0.1";
    uint16_t port = 8082;
    std::cout << "[INFO] === OmniBox TransferServer is starting on " << ip << ":" << port << " ===" << std::endl;
    
    // 初始化数据库连接池
    ConnectionPool::Instance().Init("127.0.0.1", "root", "123456", "omni_box");
    
    // 初始化你的 RPC 服务器 (基于 MyMuduo)
    RPCServer transfer_rpc_server(ip, port);

    // 注册刚刚写好的文件传输服务
    TransferServiceImpl transfer_service;
    transfer_rpc_server.RegisterService(&transfer_service);

    // 阻塞运行，静静等待 Gateway 发来的 RPC 碎片
    transfer_rpc_server.Run(4);

    return 0;
}