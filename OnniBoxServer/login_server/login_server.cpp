#include <iostream>
#include <mymuduo/db/ConnectionPool.h>
#include <mymuduo/base/ThreadPool.h>
#include "RPCServer.h"
#include "MyLoginService.h"

int main(int argc, char** argv) 
{
    EventLoop main_loop;

    std::shared_ptr<ThreadPool> thread_pool = std::make_shared<ThreadPool>("Login_Pool");
    thread_pool->start(4);
    
    RPCServer server(&main_loop, "127.0.0.1", 9090);
    MyLoginService Login(&main_loop, thread_pool);
    server.RegisterService(&Login);

    // 初始化数据库
    ConnectionPool::Instance().Init("127.0.0.1", "root", "123456", "omni_box");

    server.Run();
    main_loop.Loop();
    return 0;
}