#include <iostream>
#include "RPCServer.h"
#include "MyLoginService.h"

int main(int argc, char** argv) 
{
    EventLoop main_loop;
    RPCServer server(&main_loop, "127.0.0.1", 9090);
    MyLoginService Login;
    server.RegisterService(&Login);

    server.Run();

    return 0;
}