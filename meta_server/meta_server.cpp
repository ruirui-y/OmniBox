#include <iostream>
#include <mymuduo/db/ConnectionPool.h>
#include "RPCServer.h"
#include "MetaServiceImpl.cpp"

int main(int argc, char** argv)
{
    EventLoop main_loop;

    RPCServer server(&main_loop, "127.0.0.1", 8090);
    MetaServiceImpl meta(&main_loop);
    server.RegisterService(&meta);

    // 初始化数据库
    ConnectionPool::Instance().Init("127.0.0.1", "root", "123456", "omni_box");

    server.Run(4);
    main_loop.Loop();
    return 0;
}